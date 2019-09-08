#include "ant_renderer.h"

enum RENDERER_API
{
    RendererAPI_None,
    
    RendererAPI_OpenGL,
    RendererAPI_Vulkan,
    RendererAPI_DirectX,
};

// TODO(soimn): How much global data is acceptable?
global struct
{
    Enum8(RENDERER_API) api;
    
    // Functions
    
} RendererGlobals;

// TODO(soimn): Store commands somehow
// TODO(soimn): Keep track of memory blocks in gpu memory

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
    
    void* light_data;
    UMM light_data_size;
};

internal inline void
GetFrustumVectors(Camera camera, V3* result)
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

internal inline Camera_Render_Info
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
    GetFrustumVectors(camera, result.culling_vectors);
    
    // NOTE(soimn): Correction for Vulkan/OpenGL/DirectX/Metal clip space coords crazyness
    switch (RendererGlobals.api)
    {
        case RendererAPI_OpenGL:
        {
            // NOTE(soimn): Remapping Z from [0, 1] -> [-1, 1]
            result.view_projection_matrix.k.z = (camera.far - camera.near) / (camera.near - camera.far);
            result.view_projection_matrix.w.z = (2 * camera.far * camera.near) / (camera.near - camera.far);
        } break;
        
        case RendererAPI_Vulkan:
        {
            // NOTE(soimn): Flipping the y-axis
            result.view_projection_matrix.j.y *= -1;
        } break;
    }
    
    return result;
};