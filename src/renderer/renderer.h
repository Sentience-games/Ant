#pragma once

#include "ant_shared.h"

#include "math/vector.h"
#include "math/matrix.h"
#include "math/transform.h"
#include "math/aabb.h"

#define RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS 8
#define RENDERER_MAX_ENTRY_COUNT_PER_RENDER_BATCH_BLOCK 1024

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
    U32 normal_map;
    
    U16 shader;
    Flag16(MATERIAL_FLAGS) flags;
    
    union
    {
        U32 albedo_map;
        V3 abledo;
    };
    
    union
    {
        U32 specular_map;
        V3 specular;
    };
    
    union
    {
        U32 metallic_map;
        V3 metallic;
    };
    
    union
    {
        U32 roughness_map;
        V3 roughness;
    };
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

struct Render_Batch
{
    struct Memory_Arena* arena;
    void* first_block;
    U32 block_count;
    U32 block_size;
    U32 entry_count;
};

enum FRAMEBUFFER_FLAGS
{
    FramebufferFlag_Multisampled,
    FramebufferFlag_HasDepthStencil,
};

struct Framebuffer
{
    Texture color_attachments[RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
    U32 color_attachment_count;
    Texture depth_stencil_attachment;
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

#define RENDERER_PUSH_MESH_FUNCTION(name) void name (Render_Batch* batch, Transform transform, Bounding_Sphere bounding_sphere, Triangle_Mesh* mesh)
typedef RENDERER_PUSH_MESH_FUNCTION(renderer_push_mesh_function);

#define RENDERER_RENDER_BATCH_FUNCTION(name) void name (Render_Batch* batch, Camera camera, Framebuffer_ID framebuffer_id)
typedef RENDERER_RENDER_BATCH_FUNCTION(renderer_render_batch_function);

// TODO(soimn): Post process stage

#define RENDERER_PRESENT_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_PRESENT_FRAME_FUNCTION(renderer_present_frame_function);

// RENDER BATCH
////////////////////////////////

#define RENDERER_CLEAN_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_CLEAN_BATCH_FUNCTION(renderer_clean_batch_function);


// TEXTURES
////////////////////////////////

#define RENDERER_CREATE_TEXTURE_FUNCTION(name) Texture name (Enum8(TEXTURE_TYPE) type, Enum8(TEXTURE_FORMAT) format, Enum8(TEXTURE_WRAPPING) u_wrapping, Enum8(TEXTURE_WRAPPING) v_wrapping, Enum8(TEXTURE_FILTERING) min_filtering, Enum8(TEXTURE_FILTERING) mag_filtering, U16 width, U16 height, U8 mip_levels)
typedef RENDERER_CREATE_TEXTURE_FUNCTION(renderer_create_texture_function);

#define RENDERER_DELETE_TEXTURE_FUNCTION(name) void name (Texture* handle)
typedef RENDERER_DELETE_TEXTURE_FUNCTION(renderer_delete_texture_function);