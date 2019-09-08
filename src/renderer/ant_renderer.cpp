#include "ant_renderer.h"

global struct Renderer_API_Function_Table
{
    
};

enum RENDERER_API
{
    RendererAPI_None,
    
    RendererAPI_OpenGL,
    RendererAPI_Vulkan,
    RendererAPI_DirectX,
};

struct Camera_Render_Info
{
    V3 culling_vectors[3];
    M4 view_projection_matrix;
};

struct Render_Batch_Block
{
    void* data;
    Render_Batch_Block* next;
    U32 size;
    U32 capacity;
};

struct Render_Batch
{
    Render_Batch_Block* first;
    U32 block_count;
    U32 total_request_count;
    
    Camera_Render_Info camera_info;
};

// TODO(soimn): Should all light batches be resident in gpu memory or 
//              should the current batch be uploaded before it is used, 
//              and freed afterwards (before the next one is uploaded)?
struct Light_Batch
{
    void* binned_light_data;
    U32 binned_light_data_size;
    U32 directional_light_count;
    Light* directional_lights;
};

inline void
GetCullingVectors(Camera camera, V3* result)
{
    if (camera.projection_mode == Camera_Perspective)
    {
        F32 half_width = Tan(camera.fov / 2.0f) * camera.near;
        
        V3 upper_near_to_far = Vec3(-half_width, half_width / camera.aspect_ratio, -1.0f);
        
        result[0] = Normalized(Cross(Vec3(1, 0, 0), upper_near_to_far));
        result[2] = Normalized(Cross(upper_near_to_far, Vec3(0, -1, 0)));
        
        // NOTE(soimn): Rotate the previous vectors to produce the remaining ones
        result[1] = -result[0];
        result[3] = -result[2];
        
        result[1].z *= -1.0f;
        result[3].z *= -1.0f;
    }
    
    else
    {
        result[0] = Vec3(1, 0, 0);
        result[1] = Vec3(0, 1, 0);
    }
}

// TODO(soimn): Correction for Vulkan/OpenGL/DirectX/Metal coords crazyness
inline Camera_Render_Info
GetCameraRenderInfo(Camera camera)
{
    Assert(camera.fov && camera.fov <= RENDERER_MAX_CAMERA_FOV);
    Assert(camera.near && camera.far > camera.near);
    Assert(camera.rotation.x || camera.rotation.y || camera.rotation.z || camera.rotation.w);
    Assert(camera.aspect_ratio);
    
    Camera_Render_Info result = {0};
    
    M4 projection_matrix;
    M4 view_matrix = ViewMatrix(camera.position, camera.rotation).m;
    
    if (camera.projection_mode == Camera_Perspective)
    {
        projection_matrix = PerspectiveMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m;
    }
    
    else
    {
        projection_matrix  = OrthographicMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m;
    }
    
    result.view_projection_matrix = projection_matrix * view_matrix;
    GetCullingVectors(camera, result.culling_vectors);
    
    return result;
};


// NOTE(soimn): Utility functions for RC construction
// 
// inline Render_Command
// RCRenderBatch(Render_Batch* batch, Light_Batch* light_batch)
// {
//     Render_Command command = {RC_RenderBatch};
//     
//     Copy(&batch, &command.param_buffer[0], sizeof(Render_Batch*));
//     Copy(&light_batch, &command.param_buffer[sizeof(Render_Batch*)], sizeof(Light_Batch*));
//     
//     return command;
// }
// 
// inline Render_Command
// RCCopyFramebuffer(Framebuffer* source, Framebuffer* dest, Enum8(RENDERER_COLOR_CHANNEL) channels)
// {
//     Render_Command command = {RC_CopyFramebuffer};
//     
//     Copy(&source, &command.param_buffer[0], sizeof(Framebuffer*));
//     Copy(&dest, &command.param_buffer[sizeof(Framebuffer*)], sizeof(Framebuffer*));
//     
//     Copy(&channels, &command.param_buffer[2 * sizeof(Framebuffer*)], sizeof(U32));
//     
//     return command;
// }
// 
// inline Render_Command
// RCClearFramebuffer(Framebuffer* framebuffer, Enum8(RENDERER_COLOR_CHANNEL) channels, U32 clear_value)
// {
//     Render_Command command = {RC_CopyFramebuffer};
//     
//     Copy(&framebuffer, &command.param_buffer[0], sizeof(Framebuffer*));
//     Copy(&channels, &command.param_buffer[sizeof(Framebuffer*)], sizeof(U32));
//     Copy(&clear_value, &command.param_buffer[sizeof(Framebuffer*) + sizeof(U32)], sizeof(U32));
//     
//     return command;
// }