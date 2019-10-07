#include "ant_renderer.h"

/// Render commands and batches
/// ///////////////////////////

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
    
    // TODO(soimn): Dynamic material data storage
    void* dynamic_material_data;
    U32 dynamic_material_data_size;
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
/// ////////////////////////////

#define RENDERER_MAX_GPU_MEMORY_BLOCK_COUNT 32
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
    
    // TODO(soimn): Find a better way of storing the free list that does not consume as much space
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
    U64 sampler_handle;
    U16 width;
    U16 height;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
};

enum RENDERER_SHADER_FIELD_FORMAT
{
    RendererShaderField_Float,
    RendererShaderField_Vec2,
    RendererShaderField_Vec3,
    RendererShaderField_Vec4,
    
    RendererShaderField_Float_16,
    RendererShaderField_Vec2_16,
    RendererShaderField_Vec3_16,
    RendererShaderField_Vec4_16,
    
    RendererShaderField_Int,
    RendererShaderField_IVec2,
    RendererShaderField_IVec3,
    RendererShaderField_IVec4,
    
    RendererShaderField_Uint,
    RendererShaderField_UVec2,
    RendererShaderField_UVec3,
    RendererShaderField_UVec4,
};

#define RENDERER_MAX_SHADER_INPUT_FIELD_COUNT 16
#define RENDERER_MAX_SHADER_OUTPUT_TEXTURE_COUNT 8
struct Shader
{
    U64 handle;
    bool is_postprocess_shader;
    
    Enum8(RENDERER_SHADER_FIELD_FORMAT) input[RENDERER_MAX_SHADER_INPUT_FIELD_COUNT];
    
    Texture_ID output[RENDERER_MAX_SHADER_OUTPUT_TEXTURE_COUNT];
};

struct Material
{
    Buffer static_data;
    Texture_ID* bound_textures;
    U32 bound_texture_count;
    U32 dynamic_buffer_size;
    
    Shader_ID shader;
    Enum8(RENDERER_SHADER_FIELD_FORMAT) format[RENDERER_MAX_SHADER_INPUT_FIELD_COUNT];
};


/// Global data accessible by all rendering backends
/// ////////////////////////////////////////////////
enum RENDERER_API
{
    RendererAPI_None,
    
    RendererAPI_OpenGL,
    RendererAPI_Vulkan,
    RendererAPI_DirectX,
};

struct Renderer_Settings
{
    U16 backbuffer_width;
    U16 backbuffer_height;
    bool enable_vsync;
    
    // TODO(soimn): Add settings for built in post processing
};

// TODO(soimn): Is this really a good way of storing this information?
global struct
{
    Renderer_Settings settings;
    
    struct Memory_Arena* state_arena;
    struct Memory_Arena* work_arena;
    GPU_Memory_Block memory_blocks[RENDERER_MAX_GPU_MEMORY_BLOCK_COUNT];
    
    Bucket_Array commands;
    Bucket_Array render_batch_array;
    Bucket_Array light_batch_array;
    
    U32 max_dynamic_material_storage_size;
    
    Mesh* static_mesh_array;
    U32 static_mesh_array_size;
    U32 static_texture_array_size;
    Texture* static_texture_array;
    
    Free_List_Variable_Bucket_Array sub_mesh_storage;
    
    // TODO(soimn): runtime generated mesh storage and handling
    
    Shader* shaders;
    U32 shader_count;
    
    U32 material_count;
    Material* materials;
    
    Enum8(RENDERER_API) api;
    
    // Functions
} RendererGlobals;



/// UTILITY FUNCTIONS
/// /////////////////

internal inline Camera_Render_Info
GetCameraRenderInfo(Camera camera)
{
    Camera_Render_Info result = {0};
    
    if (camera.projection_mode == Camera_Perspective)
    {
        { /// Compute view frustum vectors
            F32 half_width = Tan(camera.fov / 2.0f) * camera.near;
            
            V3 upper_near_to_far = Vec3(-half_width, half_width / camera.aspect_ratio, -1.0f);
            
            result.culling_vectors[0] = Normalized(Cross(Vec3(1, 0, 0), upper_near_to_far));
            result.culling_vectors[2] = Normalized(Cross(upper_near_to_far, Vec3(0, -1, 0)));
            
            // NOTE(soimn): Rotate the previous vectors to produce the remaining ones
            result.culling_vectors[1] = -result.culling_vectors[0];
            result.culling_vectors[3] = -result.culling_vectors[2];
            
            result.culling_vectors[1].z *= -1.0f;
            result.culling_vectors[3].z *= -1.0f;
        }
        
        result.view_projection_matrix = PerspectiveMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m * ViewMatrix(camera.position, camera.rotation).m;
    }
    
    else
    {
        result.culling_vectors[0] = Vec3(1, 0, 0);
        result.culling_vectors[1] = Vec3(0, 1, 0);
        
        
        result.view_projection_matrix = OrthographicMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m * ViewMatrix(camera.position, camera.rotation).m;
    }
    
    
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