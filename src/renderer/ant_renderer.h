#pragma once

#include "utils/transform.h"
#include "utils/bounding_volumes.h"
#include "math/common.h"
#include "math/geometry.h"
#include "math/vector.h"
#include "math/matrix.h"

typedef U32 Vertex_Buffer;
typedef U32 Index_Buffer;

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
    
    Vertex_Buffer vertex_buffer;
    Index_Buffer index_buffer;
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

struct Texture_View
{
    U64 handle;
    Rect view;
    Enum8(TEXTURE_FORMAT) format;
    Enum8(TEXTURE_FILTER) min_filter;
    Enum8(TEXTURE_FILTER) mag_filter;
    Enum8(TEXTURE_MIP_MAP_MODE) mip_map_mode;
    Enum8(TEXTURE_WRAP) u_wrap;
    Enum8(TEXTURE_WRAP) v_wrap;
    Enum8(TEXTURE_USAGE) usage;
};

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

typedef U64 Framebuffer_ID;
typedef U64 Shader_ID;

#define RENDERER_MAX_GBUFFER_ATTACHMENTS 4
struct GBuffer
{
    Framebuffer_ID attachments[RENDERER_MAX_GBUFFER_ATTACHMENTS];
};

// NOTE(soimn): Batching
////////////////////////////////

typedef U64 Camera_Filter;

// TODO(soimn): Pass wanted materials somehow
struct Render_Request
{
    Camera_Filter filter;
    Bounding_Sphere bounding_sphere;
    Transform transform;
    
    Mesh* mesh;
};

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

//// COMMANDS START

struct Render_Batch;
struct Light_Batch;

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

#define RENDERER_RENDER_BATCH_FUNCTION(name) void name (Render_Batch* render_batch, Light_Batch* light_batch, Shader_ID override_shader, GBuffer gbuffer)
typedef RENDERER_RENDER_BATCH_FUNCTION(renderer_render_batch_function);

/// Framebuffer operations

enum CHANNEL_MASK
{
    Channel_R    = 0x1,
    Channel_G    = 0x2,
    Channel_B    = 0x4,
    Channel_A    = 0x8,
    Channel_RGBA = 0xF,
};

enum FRAMEBUFFER_COPY_MODE
{
    FramebufferCopy_Hard,
    FramebufferCopy_LinearBlend,
};

#define RENDERER_COPY_FRAMEBUFFER_CONTENT_FUNCTION(name) void name (Framebuffer_ID source, Framebuffer_ID dest, Flag8(CHANNEL_MASK) channel_mask, Enum8(FRAMEBUFFER_COPY_MODE) copy_mode)
typedef RENDERER_COPY_FRAMEBUFFER_CONTENT_FUNCTION(renderer_copy_framebuffer_content_function);

#define RENDERER_CLEAR_FRAMEBUFFER_CONTENT_FUNCTION(name) void name (Framebuffer_ID source, Flag8(CHANNEL_MASK) channel_mask, U32 clear_value)
typedef RENDERER_CLEAR_FRAMEBUFFER_CONTENT_FUNCTION(renderer_clear_framebuffer_content_function);

#define RENDERER_APPLY_SHADER_FUNCTION(name) void name (Shader_ID shader, GBuffer source, GBuffer dest)
typedef RENDERER_APPLY_SHADER_FUNCTION(renderer_apply_shader_function);

// TODO(soimn): Compute and immediate mode

/// Object management
#define RENDERER_FLUSH_ALL_OBJECTS_FUNCTION(name) void name (void)
typedef RENDERER_FLUSH_ALL_OBJECTS_FUNCTION(renderer_flush_all_objects_function);

#define RENDERER_CREATE_FRAMEBUFFER_FUNCTION(name) Framebuffer_ID name (/*FRAMEBUFFER STUFF*/)
typedef RENDERER_CREATE_FRAMEBUFFER_FUNCTION(renderer_create_framebuffer_function);

#define RENDERER_DESTROY_FRAMEBUFFER_FUNCTION(name) void name (Framebuffer_ID framebuffer)
typedef RENDERER_DESTROY_FRAMEBUFFER_FUNCTION(renderer_destroy_framebuffer_function);

#define RENDERER_RECREATE_MATERIAL_CACHE_FUNCTION(name) void name (U32 material_count)
typedef RENDERER_RECREATE_MATERIAL_CACHE_FUNCTION(renderer_recreate_material_cahce_function);

#define RENDERER_UPLOAD_MATERIALS_FUNCTION(name) U8 name (/*MATERIAL STUFF*/ U32 count)
typedef RENDERER_UPLOAD_MATERIALS_FUNCTION(renderer_upload_materials_function);

#define RENDERER_UPDATE_MATERIAL_CACHE_FUNCTION(name) U8 name (/*MATERIAL STUFF*/)
typedef RENDERER_UPDATE_MATERIAL_CACHE_FUNCTION(renderer_update_material_cache_function);

// TODO(soimn): Add / remove / update shaders
// TODO(soimn): Add / remove meshes and textures

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
//    rendering work is done. This also gives the renderer a chance to batch allocations and reduce fragmentations
// 4. Commands are recorded between BeginFrame and EndFrame. These commands are pooled and sorted for future 
//    issuing. Commands related to resource management are batched and executed when the time is right. Commands 
//    related to drawing are sorted and split into several sequences of serialized commands which are synchronized 
//    when needed. This is done to enable drawing of independent objects while waiting on dependent objects (e.g. 
//    Drawing the enitre scene and drawing the scene from a mirrors perspective at the same time, and then waiting 
//    on both operations to finish before applying the mirrors reflection in the main image).