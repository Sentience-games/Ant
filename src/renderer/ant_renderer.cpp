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