#pragma once

#include "utils/transform.h"
#include "utils/bounding_volumes.h"
#include "math/geometry.h"
#include "math/vector.h"
#include "math/matrix.h"

// IMPORTANT NOTE(soimn): Goal: a framework to build upon

// TODO(soimn): Change this to a double indirection to enable more efficient memory management and limit the size 
//              of the buffer handle
struct GPU_Buffer
{
    U64 handle;
    U32 offset;
};

typedef GPU_Buffer Vertex_Buffer;
typedef GPU_Buffer Index_Buffer;

struct Sub_Mesh
{
    U32 first_vertex;
    U32 first_index;
    U32 index_count;
};

struct Mesh
{
    Vertex_Buffer vertex_buffer;
    Index_Buffer index_buffer;
    
    Sub_Mesh* submeshes;
    U32 submesh_count;
    
    U32 material_count;
    U32* materials;
};

enum TEXTURE_TYPE
{
    TextureType_2D,
};

struct Texture
{
    U64 handle;
    V2 dimensions;
    Enum8(TEXTURE_TYPE) type;
    U8 mip_count;
};

enum TEXTURE_FORMAT
{
    TEXTURE_FORMAT_COUNT
};

enum TEXTURE_FILTER
{
    TEXTURE_FILTER_COUNT
};

enum TEXTURE_WRAP
{
    TEXTURE_WRAP_COUNT
};

enum TEXTURE_USAGE
{
    TextureUsage_Read,
    TextureUsage_Write,
};

struct Texture_View
{
    U64 handle;
    Rect view;
    Enum8(TEXTURE_FORMAT) format;
    Enum8(TEXTURE_FILTER) min_filter;
    Enum8(TEXTURE_FILTER) mag_filter;
    Enum8(TEXTURE_WRAP) u_wrap;
    Enum8(TEXTURE_WRAP) v_wrap;
    Flag8(TEXTURE_USAGE) usage;
};

typedef U32 Shader_ID;

struct Material
{
    void* data;
    U32 size;
    B32 is_dirty;
    
    Shader_ID shader;
    
    U32 texture_count;
    Asset_ID* textures;
};

enum RENDERER_LIGHT_TYPE
{
    Light_Point,
    Light_Spot,
    Light_Directional,
    Light_Area,
};

struct Light
{
    V3 position;
    F32 intensity;
    B8 casts_shadows;
    
    Enum8(RENDERER_LIGHT_TYPE) type;
    
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

#define RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS 8
#define RENDERER_MAX_FRAMEBUFFER_UTIL_ATTACHMENTS 2
struct Framebuffer
{
    Texture_View color_attachments[RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
    Texture_View util_attachments[RENDERER_MAX_FRAMEBUFFER_UTIL_ATTACHMENTS];
};

typedef U64 Camera_Filter;

struct Render_Request
{
    Camera_Filter filter;
    Bounding_Sphere bounding_sphere;
    Transform transform;
    
    Asset_ID asset;
    
    void* dynamic_material_data;
    UMM dynamic_material_data_size;
};

enum RENDERER_CAMERA_PROJECTION_MODE
{
    Camera_Perspective,
    Camera_Orthographic,
};

#define RENDERER_MAX_CAMERA_FOV 170

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

struct Render_Batch;

#define RENDERER_PUSH_BATCH_FUNCTION(name) Render_Batch*name (Camera camera, Light* lights, U32 light_count, U32 expected_max_request_count)
typedef RENDERER_PUSH_BATCH_FUNCTION(renderer_push_batch_function);

#define RENDERER_PUSH_MESHES_FUNCTION(name) void name (Render_Batch* batch, Render_Request* requests, U32 request_count)
typedef RENDERER_PUSH_MESHES_FUNCTION(renderer_push_meshes_function);

#define RENDERER_SORT_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_SORT_BATCH_FUNCTION(renderer_sort_batch_function);

#define RENDERER_RENDER_FUNCTION(name) void name (Render_Batch* batch, Framebuffer* framebuffer, struct GBuffer* gbuffer)
typedef RENDERER_RENDER_FUNCTION(renderer_render_function);

#define RENDERER_CLEAN_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_CLEAN_BATCH_FUNCTION(renderer_clean_batch_function);