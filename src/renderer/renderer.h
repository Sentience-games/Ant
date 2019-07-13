#pragma once

#include "ant_types.h"

#include "math/bounding_volumes.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/transform.h"

// NOTE(soimn):
// - Culling and layer filtering is performed on an per triangle mesh basis, this means a triangle mesh may not 
//   contain submeshes which exhibit different layer filtering tags or bounding volumes
// - Each camera has a layer filtering flag, and every object tagged with, at least, those layers are rendered by //   the camera
// - All gpu memory is handled in sufficiently sized blocks which are persistently bound and shared between 
//   several objects
// - When data is to be uploaded to gpu memory, the current blocks are inspected and the block with the least 
//   remaining contiguous space, still fitting the size of the data to be uploaded, is chosen.
// - When data is to be evicted from gpu memory, the data is removed and the block is scheduled for reordering 
//   in order to minimize fragmentation. This reordering is performed once the accumulated waste space has crossed 
//   a certain threshold, or the block is full, and is performed at the end of the frame, running in the 
//   background, and must finish before any other interaction with gpu memory, or state changes in the case of 
//   OpenGL.

//// SORTING
// 1. Vertex buffer
// 2. Shader program
// 3. Distance from camera (near -> far)

struct GPU_Buffer_Handle
{
    U32 index;
    U32 offset;
};

typedef GPU_Buffer_Handle Vertex_Buffer_Handle;
typedef GPU_Buffer_Handle Index_Buffer_Handle;

struct Vertex
{
    V3 position;
    V3 normal;
    
    // TODO(soimn): Investigate if it is beneficial to come up with a way of computing these instead
    V3 binormal;
    V3 tangent;
    
    V2 uv;
    
    // NOTE(soimn): joint_data.x: four U8 indices, one for each bone. joint_data.yzw: three of the four weights
    V4 joint_data; 
};

enum TEXTURE_TYPE
{
    TextureType_2D,
};

enum TEXTURE_FORMAT
{
    TextureFormat_RGB8,
    TextureFormat_RGBA8,
    TextureFormat_RGB16,
    TextureFormat_RGBA16,
    
    TextureFormat_RGB16F,
    TextureFormat_RGBA16F,
    TextureFormat_RGB32F,
    TextureFormat_RGBA32F,
    
    TextureFormat_SRGB,
    TextureFormat_SRGBA,
    
    TextureFormat_BC1_RGB,
    TextureFormat_BC3_RGBA,
    
    TextureFormat_DepthStencil,
};

enum TEXTURE_FILTERING
{
    TextureFiltering_Nearest,
    TextureFiltering_Linear,
    
    TextureFiltering_NearestBetweenMips_Nearest,
    TextureFiltering_NearestBetweenMips_Linear,
    TextureFiltering_LinearBetweenMips_Nearest,
    TextureFiltering_LinearBetweenMips_Linear,
};

enum TEXTURE_WRAPPING
{
    TextureWrapping_Repeat,
    TextureWrapping_MirroredRepeat,
    TextureWrapping_ClampToEdge,
};

struct Texture
{
    U64 handle;
    U32 id;
    Enum8(TEXTURE_TYPE) type;
    Enum8(TEXTURE_FORMAT) format;
    Enum8(TEXTURE_WRAPPING) u_wrapping;
    Enum8(TEXTURE_WRAPPING) v_wrapping;
    Enum8(TEXTURE_FILTERING) min_filtering;
    Enum8(TEXTURE_FILTERING) mag_filtering;
    U16 width;
    U16 height;
    U8 mip_levels;
    U8 is_valid;
};

struct Material
{
    // TODO(soimn): Fill this
    // Parameter pack sent to fragment shader describing the color and properties of a surface
};

typedef U16 Shader_ID;

// NOTE(soimn): This is a structure descibing the location of extra shader parameters. It is currently not 
//              determined whether or not skeletal transformation matrices should be stored in a shader parameter 
//              block, however what is determined is that all extra / dynamic / per object parameters should 
//              reside in a shader parameter block.
/*
struct Shader_Param
{
    U32 offset;
    U16 size;
    B16 is_used;
};
*/

struct Triangle_List
{
    U32 material;
    U32 offset;
    U32 vertex_count;
};

struct Triangle_Mesh
{
    Vertex_Buffer_Handle vertex_buffer;
    Index_Buffer_Handle index_buffer;
    
    Triangle_List* triangle_lists;
    U32 triangle_list_count;
};

typedef U64 Camera_Filter;

struct Render_Info
{
    V3 culling_vectors[4];
    M4 view_projection_matrix;
    GPU_Buffer_Handle command_buffer_handle;
    GPU_Buffer_Handle draw_param_buffer_handle;
};

struct Camera
{
    Camera_Filter filter;
    
    Render_Info render_info;
    
    V3 position;
    Quat rotation;
    
    F32 fov;
    F32 near;
    F32 far;
    F32 aspect_ratio;
};

struct Light
{
    // TODO(soimn): Fill this
};

struct Framebuffer
{
    // TODO(soimn): Fill this
};

struct G_Buffer
{
    // TODO(soimn): Fill this
};

struct Mesh_Rendering_Info
{
    Camera_Filter camera_filter;
    Triangle_Mesh* mesh;
    Transform* transform;
    // Skeleton* skeleton
    Bounding_Sphere bounding_sphere;
};

#define RENDERER_PREPARE_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PREPARE_FRAME_FUNCTION(renderer_prepare_frame_function);

#define RENDERER_PRESENT_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PRESENT_FRAME_FUNCTION(renderer_present_frame_function);

// NOTE(soimn): The functions skips over any Mesh_Rendering_Info which mesh is NULL, which means the info_count is 
//              more of a one_past_last_info_index. This, however, will likely change when the function is 
//              optimized
#define RENDERER_PREPARE_RENDER_FUNCTION(name) void name (Camera* camera, Mesh_Rendering_Info* infos, U32 info_count, Light* lights, U32 light_count, struct Memory_Arena* temp_memory)
typedef RENDERER_PREPARE_RENDER_FUNCTION(renderer_prepare_render_function);

#define RENDERER_RENDER_FUNCTION(name) void name (Camera* camera, Framebuffer* framebuffer, G_Buffer* gbuffer)
typedef RENDERER_RENDER_FUNCTION(renderer_render_function);