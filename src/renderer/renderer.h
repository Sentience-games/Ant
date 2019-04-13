#pragma once

#include "ant_shared.h"

#include "ant_memory.h"

#include "math/vector.h"
#include "math/matrix.h"
#include "math/transform.h"
#include "math/aabb.h"

#define RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS 8
#define RENDERER_MAX_RENDER_BATCH_COUNT 16

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

struct Texture_Handle
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

enum MATERIAL_FLAGS
{
    MaterialFlag_HasNormalMap    = BITS(0),
    MaterialFlag_HasAlbedoMap    = BITS(1),
    MaterialFlag_HasSpecularMap  = BITS(2),
    MaterialFlag_HasMetallicMap  = BITS(3),
    MaterialFlag_HasRoughnessMap = BITS(4),
};

struct Material
{
    Flag32(MATERIAL_FLAGS) flags;
    
    U32 normal_map;
    
    U32 albedo_map;
    U32 specular_map;
    U32 metallic_map;
    U32 roughness_map;
    
    V3 abledo;
    V3 specular;
    V3 metallic;
    V3 roughness;
};

struct Triangle_Mesh
{
    Vertex_Buffer vertex_buffer;
    Index_Buffer index_buffer;
    
    struct Triangle_List* triangle_lists;
    U32 triangle_list_count;
};

typedef U32 Triangle_Mesh_ID;

struct alignas(MEMORY_64BIT_ALIGNED) Render_Batch
{
    U32 block_count;
    U32 entry_count;
    Memory_Arena arena;
};

typedef U32 Render_Batch_ID;

enum FRAMEBUFFER_FLAGS
{
    FramebufferFlag_Multisampled,
    FramebufferFlag_HasDepthStencil,
};

struct Framebuffer
{
    Texture_Handle color_attachments[RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
    U32 color_attachment_count;
    Texture_Handle depth_stencil_attachment;
    Flag8(FRAMEBUFFER_FLAGS) flags;
};

typedef U32 Framebuffer_ID;

struct Camera
{
    V3 position;
    V3 heading; // Normalized vector pointing along the view direction
    F32 near_plane;
    F32 far_plane;
    F32 fov;
    F32 cos_fov_squared; // Used for frustum culling
};

// MANAGED DRAWING
////////////////////////////////

#define RENDERER_PREPARE_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PREPARE_FRAME_FUNCTION(renderer_prepare_frame_function);

#define RENDERER_PUSH_MESH_FUNCTION(name) void name (Render_Batch_ID render_batch_id, Transform transform, Bounding_Sphere bounding_sphere, Triangle_Mesh_ID mesh_id)
typedef RENDERER_PUSH_MESH_FUNCTION(renderer_push_mesh_function);

#define RENDERER_RENDER_BATCH_FUNCTION(name) void name (Render_Batch_ID render_batch_id, Camera camera, Framebuffer_ID framebuffer_id)
typedef RENDERER_RENDER_BATCH_FUNCTION(renderer_render_batch_function);

// TODO(soimn): Post process stage

#define RENDERER_PRESENT_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PRESENT_FRAME_FUNCTION(renderer_present_frame_function);

// RENDER BATCH
////////////////////////////////

#define RENDERER_REQUEST_BATCH_FUNCTION(name) Render_Batch_ID name (struct Renderer_Context* context)
typedef RENDERER_REQUEST_BATCH_FUNCTION(renderer_request_batches_function);

#define RENDERER_CLEAN_BATCH_FUNCTION(name) void name (struct Renderer_Context* context, Render_Batch_ID render_batch_id)
typedef RENDERER_CLEAN_BATCH_FUNCTION(renderer_clean_batch_function);


// TEXTURES
////////////////////////////////

#define RENDERER_CREATE_TEXTURE_FUNCTION(name) Texture_Handle name (Enum8(TEXTURE_TYPE) type, Enum8(TEXTURE_FORMAT) format, Enum8(TEXTURE_WRAPPING) u_wrapping, Enum8(TEXTURE_WRAPPING) v_wrapping, Enum8(TEXTURE_FILTERING) min_filtering, Enum8(TEXTURE_FILTERING) mag_filtering, U16 width, U16 height, U8 mip_levels)
typedef RENDERER_CREATE_TEXTURE_FUNCTION(renderer_create_texture_function);

#define RENDERER_DELETE_TEXTURE_FUNCTION(name) void name (Texture_Handle* handle)
typedef RENDERER_DELETE_TEXTURE_FUNCTION(renderer_delete_texture_function);

struct Renderer_Context
{
    Render_Batch render_batches[RENDERER_MAX_RENDER_BATCH_COUNT];
    U32 render_batch_count;
    
    renderer_prepare_frame_function* PrepareFrame;
    renderer_push_mesh_function* PushMesh;
    renderer_render_batch_function* RenderBatch;
    renderer_present_frame_function* PresentFrame;
    
    renderer_request_batches_function* RequestBatch;
    renderer_clean_batch_function* CleanBatch;
    
    renderer_create_texture_function* CreateTexture;
    renderer_delete_texture_function* DeleteTexture;
};

// TODO(soimn): migrate all data over to the game side.