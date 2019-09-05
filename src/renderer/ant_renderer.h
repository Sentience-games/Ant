#pragma once

#include "utils/transform.h"
#include "utils/bounding_volumes.h"
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
    
    U32 material_count;
    U32* materials;
    
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
    
    V3 culling_vectors[3];
    M4_Inv view_matrix;
    M4_Inv projection_matrix;
    M4 view_projection_matrix;
};

struct Render_Batch;
struct Light_Batch;

#define RENDERER_PUSH_BATCH_FUNCTION(name) Render_Batch* name (Camera camera, U32 expected_max_request_count)
typedef RENDERER_PUSH_BATCH_FUNCTION(renderer_push_batch_function);

#define RENDERER_PUSH_MESHES_FUNCTION(name) void name (Render_Batch* batch, Render_Request* requests, U32 request_count)
typedef RENDERER_PUSH_MESHES_FUNCTION(renderer_push_meshes_function);

#define RENDERER_SORT_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_SORT_BATCH_FUNCTION(renderer_sort_batch_function);

#define RENDERER_CLEAN_BATCH_FUNCTION(name) void name (Render_Batch* batch)
typedef RENDERER_CLEAN_BATCH_FUNCTION(renderer_clean_batch_function);

// TODO(soimn): Correction for Vulkan/OpenGL/DirectX/Metal coords crazyness
void
UpdateCameraRenderInfo(Camera* camera)
{
    Assert(camera->fov && camera->fov <= RENDERER_MAX_CAMERA_FOV);
    Assert(camera->near && camera->far > camera->near);
    Assert(camera->rotation.x || camera->rotation.y || camera->rotation.z || camera->rotation.w);
    Assert(camera->aspect_ratio);
    
    camera->view_matrix = ViewMatrix(camera->position, camera->rotation);
    
    if (camera->projection_mode == Camera_Perspective)
    {
        F32 half_width = Tan(camera->fov / 2.0f) * camera->near;
        
        camera->projection_matrix = PerspectiveMatrix(camera->aspect_ratio, camera->fov, camera->near, camera->far);
        
        V3 upper_near_to_far = Vec3(-half_width, half_width / camera->aspect_ratio, -1.0f);
        
        camera->culling_vectors[0] = Normalized(Cross(Vec3(1, 0, 0), upper_near_to_far));
        camera->culling_vectors[2] = Normalized(Cross(upper_near_to_far, Vec3(0, -1, 0)));
        
        // NOTE(soimn): Rotate the previous vectors to produce the remaining ones
        camera->culling_vectors[1] = -camera->culling_vectors[0];
        camera->culling_vectors[3] = -camera->culling_vectors[2];
        
        camera->culling_vectors[1].z *= -1.0f;
        camera->culling_vectors[3].z *= -1.0f;
    }
    
    else
    {
        camera->projection_matrix  = OrthographicMatrix(camera->aspect_ratio, camera->fov, camera->near, camera->far);
        camera->culling_vectors[0] = Vec3(1, 0, 0);
        camera->culling_vectors[1] = Vec3(0, 1, 0);
    }
    
    camera->view_projection_matrix = camera->projection_matrix.m * camera->view_matrix.m;
};

// NOTE(soimn): Render Commands
///////////////////////////////

// TODO(soimn): Figure out if it is possible to conditionally wait on 
//              render texture operations to finish before using them 
//              by not stalling but rather complete all independent ops 
//              in a batch before conditionally stalling instead.

// TODO(soimn): Figure out where compute shaders fit into all of this

enum RENDER_COMMAND_TYPE
{
    RC_RenderBatch,
    RC_CopyFramebuffer,
    RC_ClearFramebuffer,
    RC_ApplyShader,
    
    // TODO(soimn): Immediate mode rendering with commands
};

#define RENDERER_MAX_COMMAND_PARAM_BUFFER_SIZE BYTES(64)
struct Render_Command
{
    Enum64(RENDER_COMMAND_TYPE) type;
    U8 param_buffer[RENDERER_MAX_COMMAND_PARAM_BUFFER_SIZE];
};

struct Render_Command_Buffer
{
    Render_Command* first;
    Render_Command* last;
    UMM capacity;
};

inline Render_Command
RCRenderBatch(Render_Batch* batch, Light_Batch* light_batch)
{
    Render_Command command = {RC_RenderBatch};
    
    Copy(&batch, &command.param_buffer[0], sizeof(Render_Batch*));
    Copy(&light_batch, &command.param_buffer[sizeof(Render_Batch*)], sizeof(Light_Batch*));
    
    return command;
}

inline Render_Command
RCCopyFramebuffer(Framebuffer* source, Framebuffer* dest, Enum8(RENDERER_COLOR_CHANNEL) channels)
{
    Render_Command command = {RC_CopyFramebuffer};
    
    Copy(&source, &command.param_buffer[0], sizeof(Framebuffer*));
    Copy(&dest, &command.param_buffer[sizeof(Framebuffer*)], sizeof(Framebuffer*));
    
    Copy(&channels, &command.param_buffer[2 * sizeof(Framebuffer*)], sizeof(Enum32));
    
    return command;
}

inline Render_Command
RCClearFramebuffer(Framebuffer* framebuffer, Enum8(RENDERER_COLOR_CHANNEL) channels, U32 clear_value)
{
    Render_Command command = {RC_CopyFramebuffer};
    
    Copy(&framebuffer, &command.param_buffer[0], sizeof(Framebuffer*));
    Copy(&channels, &command.param_buffer[sizeof(Framebuffer*)], sizeof(Enum32));
    Copy(&clear_value, &command.param_buffer[sizeof(Framebuffer*) + sizeof(Enum32)], sizeof(U32));
    
    return command;
}

// TODO(soimn):

// TODO(soimn): Figure out how to pass the shader
// inline Render_Command
// RCApplyShader(Framebuffer* framebuffer, Shader shader);

// Flush all allocations and objects
// Create / destroy framebuffer

// TODO(soimn): All of these functions relate to managing which assets 
//              are located in gpu memory. Should these be render 
//              commands or regular functions?
// Flush materials, add material, remove material(?)
// GetShader, add shader, remove shader (?), update shader
// Add / remove mesh, update mesh
// Add / remove texture, update texture