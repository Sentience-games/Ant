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
    Asset_ID asset;
    
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
    Asset_ID asset;
    
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
    struct
    {
        Buffer binary;
        Enum32(RENDERER_SHADER_STAGE) stage;
    }* shader_stages;
    
    U32 shader_stage_count;
    
    // TODO(soimn): Input requirements
    // TODO(soimn): Output requirements
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

#define RENDERER_UPLOAD_MESHES_FUNCTION(name) U8 name (Mesh_Info* meshes, U32 count, Mesh_ID* mesh_ids)
typedef RENDERER_UPLOAD_MESHES_FUNCTION(renderer_upload_meshes_function);

#define RENDERER_REMOVE_MESHES_FUNCTION(name) void name (Mesh_ID mesh_id)
typedef RENDERER_REMOVE_MESHES_FUNCTION(renderer_remove_meshes_function);

#define RENDERER_UPLOAD_TEXTURES_FUNCTION(name) U8 name (Texture_Info* textures, U32 count, Texture_ID* texture_ids)
typedef RENDERER_UPLOAD_TEXTURES_FUNCTION(renderer_upload_textures_function);

#define RENDERER_REMOVE_TEXTURES_FUNCTION(name) void name (Texture_ID texture_id)
typedef RENDERER_REMOVE_TEXTURES_FUNCTION(renderer_remove_textures_function);

// TODO(soimn): Compute and immediate mode

#define RENDERER_END_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_END_FRAME_FUNCTION(renderer_end_frame_function);

//// COMMANDS END

//// DRAFT
// 0. The renderer allocates a fixed chunk (could be several pools) of memory on the gpu side and suballocates 
//    every time a resource is uploaded
// 1. The renderer does *not* care about which mesh/texture/material is where or if it is resident at all,
//    it views all objects as generic blocks of data and only "converts" the data to the required format when it
//    is used. This is done to enable defragmenting of the gpu memory and easy handling of resources.
// 2. Either the renderer, or the game, asks the asset system for the mesh/texture/material to be used and the 
//    asset system then decides what to do. (e.g. queue an asset for loading and returning the default data for 
//    that asset, or returning visually similar assets or lower quality versions when the asset is not present)
// 3. Requests for allocations or transfer of data from cpu side to gpu side are accumulated and executed once 
//    rendering work is done. This also gives the renderer a chance to batch allocations and reduce fragmentation
// 4. Commands are recorded between BeginFrame and EndFrame. These commands are pooled and sorted for future 
//    issuing. Commands related to resource management are batched and executed when the time is right. Commands 
//    related to drawing are sorted and split into several sequences of serialized commands which are synchronized 
//    when needed. This is done to enable drawing of independent objects while waiting on dependent objects (e.g. 
//    Drawing the enitre scene and drawing the scene from a mirrors perspective at the same time, and then waiting 
//    on both operations to finish before applying the mirrors reflection in the main image).