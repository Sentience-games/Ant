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

// TODO(soimn): Memory management abstraction

struct Camera_Render_Info
{
    Camera_Filter filter;
    M4 view_projection_matrix;
    V3 culling_vectors[4];
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
    
    Camera_Render_Info camera;
    
    // TODO(soimn): Consider adding guards to ensure a batch is properly used
    // TODO(soimn): Lights and what to do about them
};

// TODO(soimn): Correction for Vulkan/OpenGL/DirectX/Metal coords crazyness
internal Camera_Render_Info
RendererGetCameraRenderInfo(Camera camera)
{
    Assert(camera.fov && camera.fov <= RENDERER_MAX_CAMERA_FOV);
    Assert(camera.near && camera.far > camera.near);
    Assert(camera.rotation.x || camera.rotation.y || camera.rotation.z || camera.rotation.w);
    Assert(camera.aspect_ratio);
    
    Camera_Render_Info result = {};
    
    M4 view_matrix       = ViewMatrix(camera.position, camera.rotation).m;
    M4 projection_matrix = {};
    
    if (camera.projection_mode = Camera_Perspective)
    {
        F32 half_width = Tan(camera.fov / 2.0f) * camera.near;
        
        projection_matrix = PerspectiveMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m;
        
        V3 upper_near_to_far = Vec3(-half_width, half_width / camera.aspect_ratio, -1.0f);
        
        result.culling_vectors[0] = Normalized(Cross(Vec3(1, 0, 0), upper_near_to_far));
        result.culling_vectors[2] = Normalized(Cross(upper_near_to_far, Vec3(0, -1, 0)));
        
        // NOTE(soimn): Rotate the previous vectors to produce the remaining ones
        result.culling_vectors[1] = -result.culling_vectors[0];
        result.culling_vectors[3] = -result.culling_vectors[2];
        
        result.culling_vectors[1].z *= -1.0f;
        result.culling_vectors[3].z *= -1.0f;
    }
    
    else
    {
        projection_matrix = OrthographicMatrix(camera.aspect_ratio, camera.fov, camera.near, camera.far).m;
    }
    
    result.view_projection_matrix = projection_matrix * view_matrix;
    
    return result;
};