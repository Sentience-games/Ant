#include "ant_renderer.h"

/// Render commands and batches
struct Camera_Render_Info
{
    V3 culling_vectors[3];
    M4 view_projection_matrix;
};

struct Render_Batch
{
    Render_Request* first;
    U32 push_index;
    U32 capacity;
    Camera_Render_Info camera_info;
};

struct Light_Batch
{
    // TODO(soimn): Find out how to deal with view frustum light clusters
    Light* first;
    U32 push_index;
    U32 capacity;
};

struct Render_Command
{
    // TODO(soimn): What should this contain?
};

/// Memory management structures

#define RENDERER_MAX_GPU_MEMORY_BLOCK_COUNT 24
#define RENDERER_GPU_MEMORY_BLOCK_SIZE MEGABYTES(256)
#define RENDERER_MIN_ALLOCATION_SIZE KILOBYTES(16)

struct GPU_Free_List
{
    GPU_Free_List* next;
    U32 offset;
    U32 size;
};

struct GPU_Memory_Block
{
    U64 handle;
    U32 push_offset;
    
    // NOTE(soimn): The free list has a capacity of BLOCK_SIZE / MIN_ALLOCATION_SIZE, and each element is indexed //              by the integer result of the offset divided by the MIN_ALLOCATION_SIZE
    U32 free_list_size;
    GPU_Free_List* free_list;
    GPU_Free_List* first_free;
    U32 largest_free;
    U32 smallest_free;
};

union GPU_Buffer
{
    struct
    {
        U32 block_index;
        U32 block_offset;
    };
    
    U64 handle;
};

struct Sub_Mesh
{
    U32 first_vertex;
    U32 first_index;
    U32 index_count;
};

struct Mesh
{
    Sub_Mesh* submeshes;
    U32 submesh_count;
    
    GPU_Buffer vertex_buffer;
    GPU_Buffer index_buffer;
};

struct Texture
{
    GPU_Buffer texture_buffer;
    U32 width;
    U32 height;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
};

// TODO(soimn): Every material has a dynamic input buffer associated with it, which is present on a per mesh 
//              instance basis
struct Material
{
    Shader_ID shader;
    U32 static_buffer_size;
    U32 dynamic_buffer_size;
    
    // TODO(soimn): Texture bindings
};


struct Shader
{
    U64 handle;
    bool is_postprocess_shader;
    // TODO(soimn): Describe the input and outputs of this shader
};

/// Global data

enum RENDERER_API
{
    RendererAPI_None,
    
    RendererAPI_OpenGL,
    RendererAPI_Vulkan,
    RendererAPI_DirectX,
};

// TODO(soimn): Is this really a good way of storing this information?
global struct
{
    struct Memory_Arena* state_arena;
    struct Memory_Arena* work_arena;
    GPU_Memory_Block memory_blocks[RENDERER_MAX_GPU_MEMORY_BLOCK_COUNT];
    
    Bucket_Array commands;
    
    Bucket_Array mesh_array;
    Bucket_Array sub_mesh_array;
    Bucket_Array texture_array;
    
    Shader* shaders;
    U32 shader_count;
    
    U32 material_count;
    Material* materials;
    
    Enum8(RENDERER_API) api;
    
    // Functions
} RendererGlobals;

/// Camera related utility functions
internal inline void
GetFrustumVectors(Camera camera, V3* result)
{
    if (camera.projection_mode == Camera_Perspective)
    {
        F32 half_width = Tan(camera.fov / 2.0f) * camera.near;
        
        V3 upper_near_to_far = Vec3(-half_width, half_width / camera.aspect_ratio, -1.0f);
        
        result[0] = Normalized(Cross(Vec3(1, 0, 0), upper_near_to_far));
        result[2] = Normalized(Cross(upper_near_to_far, Vec3(0, -1, 0)));
        
        // NOTE(soimn): Rotate the previous vectors to produce the remaining ones
        result[1] = -result[0];
        result[3] = -result[2];
        
        result[1].z *= -1.0f;
        result[3].z *= -1.0f;
    }
    
    else
    {
        result[0] = Vec3(1, 0, 0);
        result[1] = Vec3(0, 1, 0);
    }
}

internal inline Camera_Render_Info
GetCameraRenderInfo(Camera camera)
{
    Assert(camera.fov && camera.fov <= RENDERER_MAX_CAMERA_FOV);
    Assert(camera.near && camera.far > camera.near);
    Assert(camera.rotation.x || camera.rotation.y || camera.rotation.z || camera.rotation.w);
    Assert(camera.aspect_ratio);
    
    Camera_Render_Info result = {0};
    
    M4 projection_matrix;
    M4 view_matrix = ViewMatrix(camera.position, camera.rotation).m;
    
    if (camera.projection_mode == Camera_Perspective)
    {
        projection_matrix = PerspectiveMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m;
    }
    
    else
    {
        projection_matrix  = OrthographicMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m;
    }
    
    result.view_projection_matrix = projection_matrix * view_matrix;
    GetFrustumVectors(camera, result.culling_vectors);
    
    // NOTE(soimn): Correction for Vulkan/OpenGL/DirectX/Metal clip space coords crazyness
    switch (RendererGlobals.api)
    {
        case RendererAPI_OpenGL:
        {
            // NOTE(soimn): Remapping Z from [0, 1] -> [-1, 1]
            result.view_projection_matrix.k.z = (camera.far - camera.near) / (camera.near - camera.far);
            result.view_projection_matrix.w.z = (2 * camera.far * camera.near) / (camera.near - camera.far);
        } break;
        
        case RendererAPI_Vulkan:
        {
            // NOTE(soimn): Flipping the y-axis
            result.view_projection_matrix.j.y *= -1;
        } break;
    }
    
    return result;
};