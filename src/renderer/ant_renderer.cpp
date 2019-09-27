#include "ant_renderer.h"

enum RENDERER_API
{
    RendererAPI_None,
    
    RendererAPI_OpenGL,
    RendererAPI_Vulkan,
    RendererAPI_DirectX,
};

// TODO(soimn): How much global data is acceptable?
global struct
{
    Enum8(RENDERER_API) api;
    
    // Functions
    
    struct GPU_Memory_Block* memory_blocks;
    UMM memory_block_count;
} RendererGlobals;

// TODO(soimn): Store commands somehow

/// Render commands and batches
struct Camera_Render_Info
{
    V3 culling_vectors[3];
    M4 view_projection_matrix;
};

struct Render_Batch_Block
{
    void* data;
    Render_Batch_Block* next;
    U32 size;
    U32 capacity;
};

struct Render_Batch
{
    Render_Batch_Block* first;
    U32 block_count;
    U32 total_request_count;
    
    Camera_Render_Info camera_info;
    
    void* light_data;
    UMM light_data_size;
};

/// Memory management structures

struct GPU_Memory_Free_List_Entry
{
    GPU_Memory_Free_List_Entry* next;
    U32 offset;
    U32 size;
};

#define RENDERER_GPU_MEMORY_BLOCK_SIZE MEGABYTES(256)
#define RENDERER_MIN_ALLOCATION_SIZE KILOBYTES(16)
struct GPU_Memory_Block
{
    U64 handle;
    
    // TODO(soimn): How should the free list be stored
    // NOTE(soimn): When the free list is empty there are no gaps between allocations, and the leading edge should 
    //              be consolted when allocating.
    GPU_Memory_Free_List_Entry* free_list;
    U32 leading_edge;
    
    // NOTE(soimn): This is referenced when searching for a suitable memory block
    U32 largest_free;
    U32 smallest_free;
};


struct Texture
{
    U64 handle;
    V2 dimensions;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
};


// TODO(soimn): This needs to be revised
struct Material
{
    void* data;
    U32 size;
    B32 is_dirty;
    
    U32 shader;
    
    U32 texture_count;
    Texture_View* textures;
};

// TODO(soimn): Should framebuffer be nuked and raw textures be used instead?
struct Framebuffer
{
    Texture_View texture;
};

internal U64
RendererAllocateMemory(U32 size, U8 alignment)
{
    U64 result = 0;
    
    // TODO(soimn): Check all memory blocks
    // TODO(soimn): if none exist, or there is no suitable space for allocation, allocate new block
    // TODO(soimn): If the memory block limit has been reached, return a special value
    
    return result;
}

internal void
RendererFreeMemory(U64 handle)
{
    // TODO(soimn): Free the passed memory and defragmen the block, if possible
}

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