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
//   remaining contious space still fitting the size of the data to be uploaded is chosen.
// - When data is to be evicted from gpu memory, the data is removed and the block is scheduled for reordering 
//   in order to minimize fragmentation. This reordering is performed once the accumulated waste space has crossed 
//   a certain threshold, or the block is full, and is performed at the start of the frame, running in the 
//   background, and must finish before any other interaction with gpu memory, or state changes in the case of 
//   OpenGL.

struct GPU_Buffer
{
    U64 handle;
    U32 offset;
};

typedef GPU_Buffer Vertex_Buffer;
typedef GPU_Buffer Index_Buffer;

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
};

struct Triangle_List
{
    U32 material;
    U32 offset;
    U32 vertex_count;
};

struct Triangle_Mesh
{
    Vertex_Buffer vertex_buffer;
    Index_Buffer index_buffer;
    
    Triangle_List* triangle_lists;
    U32 triangle_list_count;
};

struct Render_Batch_Block
{
    Render_Batch_Block* previous;
    Render_Batch_Block* next;
    U64 size;
};

struct Render_Batch_Entry
{
    Vertex_Buffer vertex_buffer;
    Index_Buffer index_buffer;
    U32 vertex_count;
    U32 material;
    M4 mvp;
};

struct Render_Batch
{
    struct Memory_Arena* arena;
    Render_Batch_Block* current_block;
    U32 current_index;
    U32 entry_count;
};

struct Camera
{
    U64 layer_flag;
    
    M4_Inv projection_matrix;
    M4_Inv view_matrix;
    M4 view_projection_matrix;
    
    V3 position;
    Quat rotation;
    
    F32 fov;
    F32 near;
    F32 far;
    F32 aspect_ratio;
    
    // NOTE(soimn): Up, down, left, right
    V3 culling_vectors[4];
    
    U32 default_block_size;
    Render_Batch* batch;
    
    bool is_prepared;
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

#define RENDERER_PREPARE_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PREPARE_FRAME_FUNCTION(renderer_prepare_frame_function);

#define RENDERER_PRESENT_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PRESENT_FRAME_FUNCTION(renderer_present_frame_function);

#define RENDERER_PUSH_MESH_FUNCTION(name) void name (Camera* camera, Triangle_Mesh* mesh, Transform transform, Bounding_Sphere bounding_sphere)
typedef RENDERER_PUSH_MESH_FUNCTION(renderer_push_mesh_function);

#define RENDERER_RENDER_FUNCTION(name) void name (Camera* camera, Framebuffer* framebuffer, G_Buffer* gbuffer, Memory_Arena* temp_memory)
typedef RENDERER_RENDER_FUNCTION(renderer_render_function);