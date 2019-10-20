#include "ant_renderer.h"

/// Memory management structures
/// ////////////////////////////

enum RENDERER_MEMORY_USAGE
{
    RendererMemoryUsage_MeshData,
    RendererMemoryUsage_TextureData,
};

struct GPU_Free_List
{
    GPU_Free_List* next;
    U32 offset;
    U32 size;
};

struct GPU_Memory_Block
{
    Enum64(RENDERER_MEMORY_USAGE) usage;
    U64 memory_handle;
    U64 object_handle;
    
    U32 offset;
    U32 space;
    
    U32 largest_free;
    U32 smallest_free;
    
    Free_List_Bucket_Array free_list;
    GPU_Free_List* first_free;
};

struct GPU_Buffer
{
    U64 handle;
    U32 block_index;
    U32 block_offset;
};

struct Sub_Mesh
{
    U32 first_vertex;
    U32 first_index;
    U32 index_count;
};

struct Mesh
{
    GPU_Buffer vertex_buffer;
    GPU_Buffer index_buffer;
    
    Sub_Mesh* submeshes;
    U32 submesh_count;
    
    U32 memory_footprint;
    B32 is_loaded;
};

struct Texture
{
    GPU_Buffer texture_buffer;
    
    U32 memory_footprint;
    B32 is_loaded;
    
    U16 width;
    U16 height;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
    U8 layer_count;
    
    Enum8(TEXTURE_USAGE) current_usage;
};

struct Texture_View
{
    U64 sampler_handle;
    Texture_View_Info info;
};

struct Shader
{
    U64 handle;
    
    Buffer shader_code[RENDERER_SHADER_STAGE_COUNT];
    
    B32 is_post_process_shader;
    Enum8(RENDERER_SHADER_FIELD_TYPE) input_data[RENDERER_MAX_SHADER_INPUT_FIELD_COUNT];
    U32 output_framebuffer_count;
};

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

enum RENDER_COMMAND_TYPE
{
    RenderCommand_RenderBatch,
    RenderCommand_LoadOp,
    RenderCommand_UnloadOp,
    RenderCommand_UpateMaterialCache,
    RenderCommand_UpdateMaterial,
    RenderCommand_AddShader,
    RenderCommand_RemoveShader,
};

struct Render_Command
{
    Enum64(RENDER_COMMAND_TYPE) type;
    
    union
    {
        // Render batch
        struct
        {
            U32 render_batch;
            U32 light_batch;
            Material_ID override_material;
            Texture_Bundle output_textures;
        } render_batch_op;
        
        // Unload mesh/texture
        struct
        {
            GPU_Buffer buffers[2];
        } unload_op;
        
        // Load mesh/texture
        struct
        {
            U32 id;
            B32 type;
        } load_op;
        
        // Update material
        Material_ID material_to_update;
        
        // Add/remove shader
        struct
        {
            Shader* shader;
            B64 should_add;
        } shader_op;
    };
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
    
    // TODO(soimn): Add settings for built in post processing, texture handling and optional functionality
};

// TODO(soimn): Is this really a good way of storing this information?
// TODO(soimn): Profile and see if there is a problem with everything being stored in interleaved blocks
global struct
{
    Renderer_Settings settings;
    
    struct Memory_Arena* state_arena;
    struct Memory_Arena* work_arena;
    Bucket_Array memory_blocks;
    
    /// Stored on state arena
    Free_List_Bucket_Array mesh_storage;
    Free_List_Variable_Bucket_Array sub_mesh_storage;
    
    Free_List_Bucket_Array texture_storage;
    Free_List_Bucket_Array texture_view_storage;
    
    Bucket_Array shader_storage;
    
    Bucket_Array material_storage;
    B32 material_cache_outdated;
    U32 max_dynamic_material_buffer_size;
    
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
    
    result = (Mesh*)ElementAt(&RendererGlobals.mesh_storage, id);
    
    return (result && result->memory_footprint != 0 ? result : 0);
}

inline Texture*
RendererGetTexture(Texture_ID id)
{
    Texture* result = 0;
    
    result = (Texture*)ElementAt(&RendererGlobals.texture_storage, id);
    
    return (result && result->memory_footprint != 0 ? result : 0);
}

inline Texture_View*
RendererGetTextureView(Texture_View_ID id)
{
    Texture_View* result = 0;
    
    result = (Texture_View*)ElementAt(&RendererGlobals.texture_view_storage, id);
    
    return (result && result->sampler_handle != 0 ? result : 0);
}

inline Shader*
RendererGetShader(Shader_ID id)
{
    return (Shader*)ElementAt(&RendererGlobals.shader_storage, id);
}

inline Material_Info*
RendererGetMaterialInfo(Material_ID id)
{
    return (Material_Info*)ElementAt(&RendererGlobals.material_storage, id);
}