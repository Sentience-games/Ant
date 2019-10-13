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
    Bucket_Array requests;
    Camera_Render_Info camera_info;
    Bucket_Array dynamic_material_data;
};

struct Light_Batch
{
    // TODO(soimn): Find out how to deal with view frustum light clusters
    Bucket_Array lights;
};

struct Render_Command
{
    // TODO(soimn): What should this contain?
};


/// Memory management structures
/// ////////////////////////////

struct GPU_Free_List
{
    GPU_Free_List* next;
    U32 offset;
    U32 size;
};

struct GPU_Memory_Block
{
    U64 handle;
    
    U32 offset;
    U32 space;
    
    U32 largest_free;
    U32 smallest_free;
    
    Free_List_Bucket_Array free_list;
    GPU_Free_List* first_free;
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
    U32 memory_footprint;
    B32 is_loaded;
    
    Sub_Mesh* submeshes;
    U32 submesh_count;
    
    GPU_Buffer vertex_buffer;
    GPU_Buffer index_buffer;
};

struct Transient_Mesh
{
    Mesh mesh;
    U64 marker;
};

struct Texture
{
    U32 memory_footprint;
    B32 is_loaded;
    
    GPU_Buffer texture_buffer;
    
    U16 width;
    U16 height;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
    U8 layer_count;
    
    Enum8(TEXTURE_USAGE) current_usage;
};

struct Transient_Texture
{
    Texture texture;
    U64 marker;
};

struct Texture_View
{
    U64 sampler_handle;
    Texture_View_Info info;
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

#define RENDERER_MAX_MATERIAL_BOUND_TEXTURE_COUNT 8
struct Material
{
    Buffer static_data;
    Texture_View bound_textures[RENDERER_MAX_MATERIAL_BOUND_TEXTURE_COUNT];
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
    bool allow_partial_push_to_batches;
    
    // TODO(soimn): Add settings for built in post processing, texture handling and other optional functionality
};

// TODO(soimn): Is this really a good way of storing this information?
// TODO(soimn): Profile and see if there is a problem with everything being stored in interleaved blocks
// TODO(soimn): Should the transient/persistent notion be removed and all objects treated equally, despite 
//              lifetime differences?
global struct
{
    Renderer_Settings settings;
    
    struct Memory_Arena* state_arena;
    struct Memory_Arena* work_arena;
    Bucket_Array memory_blocks;
    
    /// Stored on state arena
    Bucket_Array mesh_storage;
    Free_List_Variable_Bucket_Array sub_mesh_storage;
    
    Free_List_Bucket_Array transient_mesh_storage;
    Free_List_Variable_Bucket_Array transient_sub_mesh_storage;
    
    Bucket_Array texture_storage;
    Bucket_Array texture_view_storage;
    
    Free_List_Bucket_Array transient_texture_storage;
    Free_List_Bucket_Array transient_texture_view_storage;
    
    Bucket_Array shader_storage;
    
    Bucket_Array material_storage;
    B32 material_cache_outdated;
    U32 max_dynamic_material_buffer_size;
    
    Free_List_Bucket_Array transient_material_storage;
    B32 transient_material_cache_outdated;
    U32 max_transient_dynamic_material_buffer_size;
    
    /// Stored on work arena
    Bucket_Array commands;
    Bucket_Array render_batch_array;
    Bucket_Array light_batch_array;
    
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

inline Mesh*
RendererGetMesh(Mesh_ID id)
{
    Mesh* result = 0;
    
    if ((id & 0x7FFFFFFF) == id)
    {
        result = (Mesh*)ElementAt(&RendererGlobals.mesh_storage, id);
    }
    
    else
    {
        id &= 0x7FFFFFFF;
        
        Free_List_Bucket_Array* transient_mesh_storage = &RendererGlobals.transient_mesh_storage;
        
        if (transient_mesh_storage->block_count)
        {
            UMM element_count = (transient_mesh_storage->block_count - 1) * transient_mesh_storage->block_size + transient_mesh_storage->current_block->offset;
            
            if (id < element_count)
            {
                UMM block_index  = element_count / transient_mesh_storage->block_size;
                U32 block_offset = element_count % transient_mesh_storage->block_size;
                
                Free_List_Bucket_Array_Block* scan = transient_mesh_storage->first_block;
                for (U32 i = 0; i < block_index; ++i)
                {
                    scan = scan->next;
                }
                
                Transient_Mesh* entry = (Transient_Mesh*)((U8*)(scan + 1) + transient_mesh_storage->element_size * block_offset);
                
                if (entry->marker == U64_MAX)
                {
                    result = &entry->mesh;
                }
            }
        }
    }
    
    return result;
}

inline Texture*
RendererGetTexture(Texture_ID id)
{
    Texture* result = 0;
    
    if ((id & 0x7FFFFFFF) == id)
    {
        result = (Texture*)ElementAt(&RendererGlobals.texture_storage, id);
    }
    
    else
    {
        id &= 0x7FFFFFFF;
        
        Free_List_Bucket_Array* transient_texture_storage = &RendererGlobals.transient_texture_storage;
        
        if (transient_texture_storage->block_count)
        {
            UMM element_count = (transient_texture_storage->block_count - 1) * transient_texture_storage->block_size + transient_texture_storage->current_block->offset;
            
            if (id < element_count)
            {
                UMM block_index  = element_count / transient_texture_storage->block_size;
                U32 block_offset = element_count % transient_texture_storage->block_size;
                
                Free_List_Bucket_Array_Block* scan = transient_texture_storage->first_block;
                for (U32 i = 0; i < block_index; ++i)
                {
                    scan = scan->next;
                }
                
                Transient_Texture* entry = (Transient_Texture*)((U8*)(scan + 1) + transient_texture_storage->element_size * block_offset);
                
                if (entry->marker == U64_MAX)
                {
                    result = &entry->texture;
                }
            }
        }
    }
    
    return result;
}