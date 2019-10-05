#pragma once

#include "utils/transform.h"
#include "utils/bounding_volumes.h"
#include "math/common.h"
#include "math/geometry.h"
#include "math/vector.h"
#include "math/matrix.h"

struct Render_Batch;
struct Light_Batch;
typedef U64 Mesh_ID;
typedef U64 Texture_ID;
typedef U64 Material_ID;
typedef U64 Shader_ID;
typedef U64 Camera_Filter;

/// Meshes
struct Mesh_Info
{
    UMM total_memory_footprint;
    Buffer mesh_data;
    
    struct
    {
        U32 first_vertex;
        U32 vertex_count;
        U32 first_index;
        U32 index_count;
    }* submeshes;
    U32 submesh_count;
};

/// Textures
enum CHANNEL_MASK
{
    Channel_R    = 0x1,
    Channel_G    = 0x2,
    Channel_B    = 0x4,
    Channel_A    = 0x8,
    Channel_RGB  = 0x7,
    Channel_RGBA = 0xF,
};

enum TEXTURE_TYPE
{
    TextureType_2D,
};

enum TEXTURE_FORMAT
{
    TextureFormat_1D,
    TextureFormat_2D,
    TextureFormat_3D,
};

enum TEXTURE_FILTER
{
    TextureFilter_Nearest,
    TextureFilter_Linear,
    TextureFilter_Cubic,
};

enum TEXTURE_MIP_MAP_MODE
{
    TextureMipMapMode_Nearest,
    TextureMipMapMode_Linear,
};

enum TEXTURE_WRAP
{
    TextureWrap_u,
    TextureWrap_v,
    TextureWrap_uv,
};

enum TEXTURE_USAGE
{
    TextureUsage_Read,
    TextureUsage_Write,
    TextureUsage_ReadWrite,
};

struct Texture_Info
{
    UMM total_memory_footprint;
    Buffer texture_data;
    
    U32 width;
    U32 height;
    Enum8(TEXTURE_FORMAT) format;
    Enum8(TEXTURE_FILTER) min_filter;
    Enum8(TEXTURE_FILTER) mag_filter;
    Enum8(TEXTURE_MIP_MAP_MODE) mip_map_mode;
    Enum8(TEXTURE_WRAP) u_wrap;
    Enum8(TEXTURE_WRAP) v_wrap;
    Enum8(TEXTURE_USAGE) usage;
    U8 mip_levels;
};

/// Materials
struct Material_Info
{
    Buffer data;
    Buffer default_dynamic_data;
    Texture_ID* textures;
    U32 texture_count;
};

struct Material_Palette
{
    Material_ID* materials;
    U32 material_count;
};

/// Shaders

enum RENDERER_SHADER_STAGE
{
    ShaderStage_Vertex,
    ShaderStage_Geometry,
    ShaderStage_Fragment,
};

struct Shader_Info
{
    Buffer shader_data;
};

/// Lighting
enum RENDERER_LIGHT_TYPE
{
    Light_Point,
    Light_Spot,
    Light_Directional,
    Light_Area,
};

// TODO(soimn): This needs to be revised
// TODO(soimn): Figure out how area lights should function
struct Light
{
    V3 position;
    F32 intensity;
    B8 casts_shadows;
    
    Enum8(RENDERER_LIGHT_TYPE) type;
    
    V3 ambient_color;
    V3 diffuse_color;
    V3 specular_color;
    
    union
    {
        V3 direction;
        V3 attenuation_coefficients;
        
        struct
        {
            Rect dimensions;
            V3 normal;
        };
    };
};

/// Camera 
enum RENDERER_CAMERA_PROJECTION_MODE
{
    Camera_Perspective,
    Camera_Orthographic,
};

#define RENDERER_MAX_CAMERA_FOV DEGREES(170)

struct Camera
{
    Camera_Filter filter;
    Enum32(RENDERER_CAMERA_PROJECTION_MODE) projection_mode;
    
    V3 position;
    Quat rotation;
    
    F32 fov;
    F32 near;
    F32 far;
    F32 aspect_ratio;
};

/// Texture handling for rendering
#define RENDERER_MAX_TEXTURE_BUNDLE_TEXTURE_COUNT 4
struct Texture_Bundle
{
    Texture_ID attachments[RENDERER_MAX_TEXTURE_BUNDLE_TEXTURE_COUNT];
};

/// Render request data
struct Render_Request
{
    Camera_Filter filter;
    Bounding_Sphere bounding_sphere;
    Transform transform;
    
    Mesh_ID mesh;
    Material_Palette* materials;
};

//// COMMANDS
// Commands are recorded in between the BeginFrame and EndFrame functions. The commands are recorded if their 
// input criteria are met and the recording function is called, e.g. calling PushRequest with invalid parameters 
// will result in the command not being recorded and an error code returned. Another example of such a failure 
// state is calling UploadMeshes when there is no gpu side memory left, the command will not be recorded and an 
// error code indicating that the there is not enough memory is returned.
//
// Pointers returned by command recording functions are only valid util EndFrame is called.

//// COMMANDS START

#define RENDERER_BEGIN_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_BEGIN_FRAME_FUNCTION(renderer_begin_frame_function);

/// Batching and rendering
#define RENDERER_GET_RENDER_BATCH_FUNCTION(name) Render_Batch* name (Camera camera, U32 max_request_count)
typedef RENDERER_GET_RENDER_BATCH_FUNCTION(renderer_get_render_batch_function);

#define RENDERER_PUSH_REQUEST_FUNCTION(name) U8 name (Render_Batch* batch, Render_Request* requests, U32 request_count)
typedef RENDERER_PUSH_REQUEST_FUNCTION(renderer_push_request_function);

#define RENDERER_GET_LIGHT_BATCH_FUNCTION(name) Light_Batch* name (Camera camera, U32 max_request_count)
typedef RENDERER_GET_LIGHT_BATCH_FUNCTION(renderer_get_light_batch_function);

#define RENDERER_PUSH_LIGHT_FUNCTION(name) U8 name (Light_Batch* batch, Light* lights, U32 light_count)
typedef RENDERER_PUSH_LIGHT_FUNCTION(renderer_push_light_function);

#define RENDERER_RENDER_BATCH_FUNCTION(name) void name (Render_Batch* render_batch, Light_Batch* light_batch, Material_ID override_material, Texture_Bundle gbuffer)
typedef RENDERER_RENDER_BATCH_FUNCTION(renderer_render_batch_function);

/// Render texture management
#define RENDERER_COPY_TEXTURE_CONTENT_FUNCTION(name) void name (Texture_ID source_texture, Texture_ID dest_texture, Flag8(CHANNEL_MASK) channel_mask)
typedef RENDERER_COPY_TEXTURE_CONTENT_FUNCTION(renderer_copy_texture_content_function);

#define RENDERER_CLEAR_TEXTURE_CONTENT_FUNCTION(name) void name (Texture_ID texture, Flag8(CHANNEL_MASK) channel_mask, U32 clear_value)
typedef RENDERER_CLEAR_TEXTURE_CONTENT_FUNCTION(renderer_clear_texture_content_function);

#define RENDERER_APPLY_SHADER_FUNCTION(name) void name (Shader_ID shader, Texture_Bundle source, Texture_Bundle dest)
typedef RENDERER_APPLY_SHADER_FUNCTION(renderer_apply_shader_function);

/// Object management
#define RENDERER_FLUSH_ALL_OBJECTS_FUNCTION(name) void name (void)
typedef RENDERER_FLUSH_ALL_OBJECTS_FUNCTION(renderer_flush_all_objects_function);


#define RENDERER_UPLOAD_MATERIALS_FUNCTION(name) U8 name (Material_Info* materials, U32 count)
typedef RENDERER_UPLOAD_MATERIALS_FUNCTION(renderer_upload_materials_function);

#define RENDERER_UPDATE_MATERIAL_FUNCTION(name) U8 name (Material_ID material, Material_Info data)
typedef RENDERER_UPDATE_MATERIAL_FUNCTION(renderer_update_material_function);


#define RENDERER_UPLOAD_ALL_SHADERS_FUNCTION(name) U32 name (Shader_Info* shaders, U32 count)
typedef RENDERER_UPLOAD_ALL_SHADERS_FUNCTION(renderer_upload_all_shaders_function);

#define RENDERER_UPDATE_SHADER_FUNCTION(name) U32 name (Shader_ID shader, Shader_Info data)
typedef RENDERER_UPDATE_SHADER_FUNCTION(renderer_update_shader_function);

// TODO(soimn): Is there a smarter way to do this?
// NOTE(soimn):
// Add*    - adds the object to the appropriate object table
// Remove* - unloads the object data and removes the object from the object table
// Load*   - loads the object data into gpu memory
// Unload* - unloads the object data from gpu memory

#define RENDERER_ADD_MESHES_FUNCTION(name) Mesh_ID name (Mesh_Info* meshes, U32 count)
typedef RENDERER_ADD_MESHES_FUNCTION(renderer_add_meshes_function);

#define RENDERER_REMOVE_MESHES_FUNCTION(name) void name (Mesh_ID mesh_id)
typedef RENDERER_REMOVE_MESHES_FUNCTION(renderer_remove_meshes_function);

#define RENDERER_LOAD_MESHES_FUNCTION(name) U8 name (Mesh_ID* mesh_ids, U32 count)
typedef RENDERER_LOAD_MESHES_FUNCTION(renderer_load_meshes_function);

#define RENDERER_UNLOAD_MESHES_FUNCTION(name) U8 name (Mesh_ID* mesh_ids, U32 count)
typedef RENDERER_UNLOAD_MESHES_FUNCTION(renderer_unload_meshes_function);


#define RENDERER_ADD_TEXTURES_FUNCTION(name) Texture_ID name (Texture_Info* textures, U32 count)
typedef RENDERER_ADD_TEXTURES_FUNCTION(renderer_add_textures_function);

#define RENDERER_REMOVE_TEXTURES_FUNCTION(name) void name (Texture_ID* texture_ids, U32 count)
typedef RENDERER_REMOVE_TEXTURES_FUNCTION(renderer_remove_textures_function);

#define RENDERER_LOAD_TEXTURES_FUNCTION(name) U8 name (Texture_Info* textures, U32 count)
typedef RENDERER_LOAD_TEXTURES_FUNCTION(renderer_load_textures_function);

#define RENDERER_UNLOAD_TEXTURES_FUNCTION(name) U8 name (Texture_ID* texture_ids, U32 count)
typedef RENDERER_UNLOAD_TEXTURES_FUNCTION(renderer_unload_textures_function);

// TODO(soimn): Compute and immediate mode

#define RENDERER_END_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_END_FRAME_FUNCTION(renderer_end_frame_function);

//// COMMANDS END

//// COMMAND BUFFER SYNCHRONIZATION
// Command buffers are only synchronized when necessary, e.g. rendering to a texture that is used in a render 
// request in another command buffer that succeeds the command buffer in the intended order of execution. The 
// intended execution order of the command buffers is determined by the submission order, and may be altered by 
// the rendering backend when optimizing to maximize throughput.