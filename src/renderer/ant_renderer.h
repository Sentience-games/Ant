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

#define RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS 8
#define RENDERER_MAX_FRAMEBUFFER_UTIL_ATTACHMENTS 2
struct Framebuffer
{
    Texture_View color_attachments[RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
    Texture_View util_attachments[RENDERER_MAX_FRAMEBUFFER_UTIL_ATTACHMENTS];
};

// NOTE(soimn): Batching
////////////////////////////////

typedef U64 Camera_Filter;

struct Render_Request
{
    Camera_Filter filter;
    Bounding_Sphere bounding_sphere;
    Transform transform;
    
    Mesh* mesh;
    
    // void* dynamic_material_data;
    // UMM dynamic_material_data_size;
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

struct Render_Batch;
struct Light_Batch;

#define RENDERER_PUSH_BATCH_FUNCTION(name) Render_Batch* name (Camera camera, Light* lights, U32 light_count, U32 expected_max_request_count)
typedef RENDERER_PUSH_BATCH_FUNCTION(renderer_push_batch_function);

#define RENDERER_PUSH_RENDER_REQUEST_FUNCTION(name) void name (Render_Batch* batch, Render_Request* requests, U32 request_count)
typedef RENDERER_PUSH_RENDER_REQUEST_FUNCTION(renderer_push_render_request_function);

// TODO(soimn): This step may need to happen during the "RenderBatch" 
//              function to enable synchronization between the 
//              rendering of separate batches.
#define RENDERER_SORT_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_SORT_BATCH_FUNCTION(renderer_sort_batch_function);

#define RENDERER_CLEAN_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_CLEAN_BATCH_FUNCTION(renderer_clean_batch_function);

// NOTE(soimn): Render Commands
///////////////////////////////

// TODO(soimn): Render commands a submitted by calling a "start recording" function, calling the respective 
//              command functions and ending the recording with a call to a "end recording" function.
#define RENDERER_BEGIN_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_BEGIN_FRAME_FUNCTION(renderer_begin_frame_function);

// TODO(soimn):
// RenderBatch
// CopyFramebuffer
// ClearFramebuffer
// ApplyShader
// Immediate drawing
// Compute shader management
// Flush all allocations and objects
// Create / destroy framebuffer
// Flush materials, add material, remove material(?)
// GetShader, add shader, remove shader (?), update shader
// Add / remove mesh, update mesh
// Add / remove texture, update texture

#define RENDERER_END_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_END_FRAME_FUNCTION(renderer_end_frame_function);

// IMPORTANT NOTE(soimn):
// Solution to synchronization problems regariding render textures:
// Split render batches up into normal texture/material calls and 
// requests that use render textures during render batch sorting. Then 
// check if the given render texture is renderered to earlier in the 
// pipeline, if so: move the request to the end of the command sequence 
// and insert a semaphore that waits on the render texture to be 
// renderered properly before using it for sampling, else: move the 
// request back into the normal batch.

//// DRAFT
// 0. The renderer allocates a fixed chunk (could be several pools) of memory on the gpu side and suballocates 
//    every time a resource is uploaded
// 1. The renderer does *not* care about which mesh/texture/material is where or if it is resident at all,
//    is views all objects as generic blocks of data and only "converts" the data to the required format when it
//    is used. This is done to enable defragmenting of the gpu memory and easy handling of resources.
// 2. Either the renderer, or the game, asks the asset system for the mesh/texture/material to be used and the 
//    asset system then decides what to do. (e.g. queue an asset for loading and returning the default data for 
//    that asset, or returning visually similar assets or lower quality versions when the asset is not present)
// 3. Requests for allocations or transfer of data from cpu side to gpu side are accumulated and executed once 
//    rendering work is done. This also gives the renderer a chance to batch allocations and reduce fragmentations
// 4. Commands are recorded between BeginFrame and EndFrame. These commands are pooled and sorted for future 
//    issuing. Commands related to resource management are batched and executed when the time is right. Commands 
//    related to drwaing are sorted and split into several sequences of serialized commands which are synchronized 
//    when needed. This is done to enable drawing of independent objects while waiting on dependent objects (e.g. 
//    Drawing the enitre scene and drawing the scene from a mirrors perspective at the same time, and then waiting 
//    on both operations to finish before applying the mirrors reflection in the main image).