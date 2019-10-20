#pragma once

#include "utils/transform.h"
#include "utils/bounding_volumes.h"
#include "math/common.h"
#include "math/geometry.h"
#include "math/vector.h"
#include "math/matrix.h"

struct Render_Batch;
struct Light_Batch;
typedef U32 Mesh_ID;
typedef U32 Texture_ID;
typedef U32 Texture_View_ID;
typedef U32 Material_ID;
typedef U32 Shader_ID;
typedef U64 Camera_Filter;

/// STRUCTURES RELATED TO THE TRANSFER OF OBJECT DATA TO THE RENDERER BACKEND
/// /////////////////////////////////////////////////////////////////////////

/// Meshes
struct Mesh_Info
{
    UMM memory_footprint;
    
    Buffer mesh_data;
    
    struct
    {
        U32 first_vertex;
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
    UMM memory_footprint;
    
    Buffer texture_data;
    
    U16 width;
    U16 height;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
    U8 layer_count;
};

struct Texture_View_Info
{
    Texture_ID texture;
    
    Enum8(TEXTURE_USAGE) usage;
    
    Enum8(TEXTURE_TYPE) type;
    Enum8(TEXTURE_FORMAT) format;
    
    Enum8(TEXTURE_FILTER) min_filter;
    Enum8(TEXTURE_FILTER) mag_filter;
    Enum8(TEXTURE_MIP_MAP_MODE) mip_map_mode;
    Enum8(TEXTURE_WRAP) u_wrap;
    Enum8(TEXTURE_WRAP) v_wrap;
    
    U8 mip_lowest;
    U8 mip_highest;
    
    U8 level_lowest;
    U8 level_highest;
};

/// Materials
struct Material_Info
{
    Buffer data;
    Buffer default_dynamic_data;
    Texture_View_Info* textures;
    U32 texture_count;
};

struct Material_Palette
{
    Material_ID* materials;
    U32 material_count;
};

/// Shaders

enum RENDERER_SHADER_ENCODING
{
    ShaderEncoding_GLSL,
    ShaderEncoding_HLSL,
    
    RENDERER_SHADER_ENCODING_COUNT
};

enum RENDERER_SHADER_STAGE
{
    ShaderStage_Vertex,
    ShaderStage_Geometry,
    ShaderStage_Fragment,
    
    RENDERER_SHADER_STAGE_COUNT
};

#define RENDERER_MAX_SHADER_INPUT_FIELD_COUNT 16

enum RENDERER_SHADER_FIELD_TYPE
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
    
    RendererShaderField_Sampler,
};

struct Shader_Info
{
    Buffer shader_stage_data[RENDERER_SHADER_ENCODING_COUNT][RENDERER_SHADER_STAGE_COUNT];
    
    B32 is_post_process_shader;
    Enum8(RENDERER_SHADER_FIELD_TYPE) input_data[RENDERER_MAX_SHADER_INPUT_FIELD_COUNT];
    U32 output_framebuffer_count;
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

/// STRUCTURES USED TO MANAGE THE RENDERING OF OBJECTS
/// //////////////////////////////////////////////////

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

#define RENDERER_MAX_TEXTURE_BUNDLE_TEXTURE_COUNT 8
struct Texture_Bundle
{
    Texture_View_ID texture_views[RENDERER_MAX_TEXTURE_BUNDLE_TEXTURE_COUNT];
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

/// RENDERER COMMANDS
/// /////////////////

enum RENDERER_ERROR_CODE
{
    Renderer_UnknownError      = 0x0,
    Renderer_Success           = 0x1,
    Renderer_OutOfMemory       = 0x2,
    Renderer_InvalidObjectInfo = 0x4,
};

//// COMMANDS
// Commands are recorded in between the BeginFrame and EndFrame functions. The commands are recorded if their 
// input criteria are met and the recording function is called, e.g. calling PushRequest with invalid parameters 
// will result in the command not being recorded and an error code returned. Another example of such a failure 
// state is calling LoadMesh when there is no gpu side memory left, the command will not be recorded and an 
// error code indicating that the there is not enough memory is returned.
//
// Pointers returned by command recording functions are only valid util EndFrame is called.

//// COMMANDS START

#define RENDERER_BEGIN_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_BEGIN_FRAME_FUNCTION(renderer_begin_frame_function);

/// Batching and rendering
#define RENDERER_GET_RENDER_BATCH_FUNCTION(name) Render_Batch* name (Camera camera, U32 block_size)
typedef RENDERER_GET_RENDER_BATCH_FUNCTION(renderer_get_render_batch_function);

#define RENDERER_PUSH_REQUESTS_FUNCTION(name) U8 name (Render_Batch* batch, Render_Request* requests, U32 request_count)
typedef RENDERER_PUSH_REQUESTS_FUNCTION(renderer_push_requests_function);

#define RENDERER_GET_LIGHT_BATCH_FUNCTION(name) Light_Batch* name (Camera camera, U32 block_size)
typedef RENDERER_GET_LIGHT_BATCH_FUNCTION(renderer_get_light_batch_function);

#define RENDERER_PUSH_LIGHTS_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Light_Batch* batch, Light* lights, U32 light_count)
typedef RENDERER_PUSH_LIGHTS_FUNCTION(renderer_push_lights_function);

#define RENDERER_RENDER_BATCH_FUNCTION(name) void name (Render_Batch* render_batch, Light_Batch* light_batch, Material_ID override_material, Texture_Bundle output_textures)
typedef RENDERER_RENDER_BATCH_FUNCTION(renderer_render_batch_function);

/// Texture operations

// TODO(soimn):
// Apply post processing
// Clear / fill, copy

/// Object management

// Mesh management
#define RENDERER_ADD_MESHES_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Mesh_Info* mesh_infos, U32 count, Mesh_ID* out_first_id)
typedef RENDERER_ADD_MESHES_FUNCTION(renderer_add_meshes_function);

#define RENDERER_REMOVE_MESH_FUNCTION(name) void name (Mesh_ID mesh_id)
typedef RENDERER_REMOVE_MESH_FUNCTION(renderer_remove_mesh_function);

#define RENDERER_LOAD_MESH_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Mesh_ID mesh_id)
typedef RENDERER_LOAD_MESH_FUNCTION(renderer_load_mesh_function);

#define RENDERER_UNLOAD_MESH_FUNCTION(name) void name (Mesh_ID mesh_id)
typedef RENDERER_UNLOAD_MESH_FUNCTION(renderer_unload_mesh_function);

#define RENDERER_MODIFY_MESH_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Mesh_ID mesh_id, Mesh_Info* mesh_info, Enum8(GET_OR_SET) operation)
typedef RENDERER_MODIFY_MESH_FUNCTION(renderer_modify_mesh_function);

// Texture management
#define RENDERER_ADD_TEXTURES_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Texture_Info* texture_infos, U32 count, Texture_ID* out_first_id)
typedef RENDERER_ADD_TEXTURES_FUNCTION(renderer_add_textures_function);

#define RENDERER_REMOVE_TEXTURE_FUNCTION(name) void name (Texture_ID texture_id)
typedef RENDERER_REMOVE_TEXTURE_FUNCTION(renderer_remove_texture_function);

#define RENDERER_LOAD_TEXTURE_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Texture_ID texture_id)
typedef RENDERER_LOAD_TEXTURE_FUNCTION(renderer_load_texture_function);

#define RENDERER_UNLOAD_TEXTURE_FUNCTION(name) void name (Texture_ID texture_id)
typedef RENDERER_UNLOAD_TEXTURE_FUNCTION(renderer_unload_texture_function);

#define RENDERER_MODIFY_TEXTURE_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Texture_ID texture_id, Texture_Info* texture_info, Enum8(GET_OR_SET) operation)
typedef RENDERER_MODIFY_TEXTURE_FUNCTION(renderer_modify_texture_function);

// Material management
#define RENDERER_ADD_MATERIALS_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Material_Info* material_infos, U32 count, Material_ID* out_first_id)
typedef RENDERER_ADD_MATERIALS_FUNCTION(renderer_add_materials_function);

#define RENDERER_MODIFY_MATERIAL_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Material_ID material_id, Material_Info* material_info, Enum8(GET_OR_SET) operation)
typedef RENDERER_MODIFY_MATERIAL_FUNCTION(renderer_modify_material_function);

// Shader management
// TODO(soimn): Add error log messages
#define RENDERER_ADD_SHADERS_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Shader_Info* shader_infos, U32 count, Shader_ID* out_first_id)
typedef RENDERER_ADD_SHADERS_FUNCTION(renderer_add_shaders_function);

#define RENDERER_REMOVE_SHADER_FUNCTION(name) void name (Shader_ID shader_id)
typedef RENDERER_REMOVE_SHADER_FUNCTION(renderer_remove_shader_function);

#define RENDERER_MODIFY_SHADER_FUNCTION(name) Flag8(RENDERER_ERROR_CODE) name (Shader_ID shader_id, Shader_Info* shader_info, Enum8(GET_OR_SET) operation)
typedef RENDERER_MODIFY_SHADER_FUNCTION(renderer_modify_shader_function);

// TODO(soimn): Compute and immediate mode

#define RENDERER_END_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_END_FRAME_FUNCTION(renderer_end_frame_function);

//// COMMANDS END

//// COMMAND BUFFER SYNCHRONIZATION
// Command buffers are only synchronized when necessary, e.g. rendering to a texture that is used in a render 
// request in another command buffer that succeeds the command buffer in the intended order of execution. The 
// intended execution order of the command buffers is determined by the submission order, and may be altered by 
// the rendering backend when optimizing to maximize throughput.