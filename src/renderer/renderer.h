#pragma once

#include "ant_shared.h"

#include "ant_memory.h"

#include "math/vector.h"
#include "math/matrix.h"
#include "math/transform.h"
#include "math/aabb.h"

#define RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS 8
#define RENDERER_MAX_CATALOG_COUNT 16
#define RENDERER_MAX_GROUP_COUNT_PER_CATALOG 8
#define RENDERER_TEXTURE_GROUP_SIZE 2048

typedef U32 Shader_ID;
typedef U32 Material_ID;
typedef U32 Triangle_Mesh_ID;
typedef U8 Texture_Catalog_ID;
typedef struct { Texture_Catalog_ID catalog; U8 group; U8 offset; U8 is_valid; } Texture_ID;
typedef U32 Framebuffer_ID;
typedef U32 Render_Batch_ID;

// Stored in the renderer

struct GPU_Buffer
{
    U64 handle;
    U32 offset;
};

typedef GPU_Buffer Vertex_Buffer;
typedef GPU_Buffer Index_Buffer;

struct Shader
{
    // Reflection, shader parameter compatibility and other stuff
};

struct Material
{
    Shader_ID shader_id;
    // Material props and textures
    // bitfield containing parameter options, will be checked against shader parameters when validating
};


struct Triangle_List
{
    // Tag to help identify parts of meshes in the editor, this needs to be specified in the asset file
    
    Material_ID material;
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
    TextureFormat_BC3_RGB,
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

// NOTE(soimn): currently the texture handles are only used to support smaller textures embedded in texture groups //              with a larger texture size, and to simplify handle Texture_ID creation and deletion. The handle, 
//              and therfore also the texture groups' handle arrays, could be removed in favor of assigning a 
//              constant ID to every asset at pack time, and preallocating sparse texture arrays for every 
//              catalog. The ID would then contain the catalog, group and offset into that group. This would break 
//              the ability to store smaller textures in larger texture arrays, but this might not be a bad thing 
//              at all.

// Individual elements in a texture group, used when "binding" textures
struct Texture_Handle
{
    Texture_Handle* next_free;
    Texture_ID id;
    U32 width;
    U32 height;
};

// Corresponds to an array of textures in GPU memory and keeps track of an array of handles to those textures
struct Texture_Group
{
    U64 id;
    U64 handle;
    Texture_Handle* first;
    Texture_Handle* next_free;
    U32 num_free;
};

// Header for array of texture groups, used as an access point
struct Texture_Catalog
{
    Texture_Group groups[RENDERER_MAX_GROUP_COUNT_PER_CATALOG];
    
    bool is_initialized;
    
    U8 group_count;
    U16 layer_count;
    U8 mip_levels;
    
    Enum8(TEXTURE_TYPE) type;
    Enum8(TEXTURE_FORMAT) format;
    Enum8(TEXTURE_WRAPPING) u_wrapping;
    Enum8(TEXTURE_WRAPPING) v_wrapping;
    Enum8(TEXTURE_FILTERING) min_filtering;
    Enum8(TEXTURE_FILTERING) mag_filtering;
    U32 max_width;
    U32 max_height;
};

enum FRAMEBUFFER_FLAGS
{
    FramebufferFlags_NoFlags       = 0x0,
    FramebufferFlags_DepthStencil  = 0x1,
    FramebufferFlags_FloatingPoint = 0x2,
    FramebufferFlags_Multisampled  = 0x4,
};

struct Framebuffer
{
    U64 handle;
    U32 color_attachment_count;
    Texture_ID color_attachments[RENDERER_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
    Texture_ID depth_stencil_attachment;
    
    Flag8(FRAMEBUFFER_FLAGS) flags;
};

struct alignas(MEMORY_64BIT_ALIGNED) Render_Batch
{
    U32 block_count;
    U32 entry_count;
    Memory_Arena arena;
};

// Stored in the game

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

#define RENDERER_REQUEST_BATCHES_FUNCTION(name) void name (U8 batch_count)
typedef RENDERER_REQUEST_BATCHES_FUNCTION(renderer_request_batches_function);

#define RENDERER_CLEAN_BATCH_FUNCTION(name) void name (Render_Batch_ID render_batch_id)
typedef RENDERER_CLEAN_BATCH_FUNCTION(renderer_clean_batch_function);

// TEXTURES
////////////////////////////////

#define RENDERER_CREATE_TEXTURE_CATALOG_FUNCTION(name) void name (Enum8(TEXTURE_TYPE) type, Enum8(TEXTURE_FORMAT) format, U32 max_width, U32 max_height, Enum8(TEXTURE_WRAPPING) u_wrapping, Enum8(TEXTURE_WRAPPING) v_wrapping, Enum8(TEXTURE_FILTERING) min_filtering, Enum8(TEXTURE_FILTERING) mag_filtering,  U8 mip_levels, U16 layer_count)
typedef RENDERER_CREATE_TEXTURE_CATALOG_FUNCTION(renderer_create_texture_catalog_function);

#define RENDERER_CREATE_TEXTURE_FUNCTION(name) Texture_ID name (Texture_Catalog_ID catalog_id, U32 count)
typedef RENDERER_CREATE_TEXTURE_FUNCTION(renderer_create_texture_function);

#define RENDERER_REMOVE_TEXTURE_FUNCTION(name) void name (Texture_ID id)
typedef RENDERER_REMOVE_TEXTURE_FUNCTION(renderer_remove_texture_function);

#define RENDERER_UPLOAD_TEXTURE_DATA_FUNCTION(name) void name (Texture_ID id, Enum8(TEXTURE_TYPE) type, U8 index,  Enum8(TEXTURE_FORMAT) format, void* data)
typedef RENDERER_UPLOAD_TEXTURE_DATA_FUNCTION(renderer_upload_texture_data_function);

struct Renderer_Context
{
    Memory_Arena renderer_arena;
    Texture_Catalog texture_catalogs[RENDERER_MAX_CATALOG_COUNT];
    
    // FUNCTION POINTERS
    //////////////////////
    
    renderer_prepare_frame_function* PrepareFrame;
    renderer_push_mesh_function* PushMesh;
    renderer_render_batch_function* RenderBatch;
    renderer_present_frame_function* PresentFrame;
    
    renderer_request_batches_function* RequestBatches;
    renderer_clean_batch_function* CleanBatch;
    
    renderer_create_texture_catalog_function* CreateTextureCatalog;
    renderer_create_texture_function* CreateTexture;
    renderer_remove_texture_function* RemoveTexture;
    renderer_upload_texture_data_function* UploadTextureData;
};