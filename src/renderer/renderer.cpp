#include "renderer/renderer.h"

#ifdef ANT_PLATFORM_WINDOWS
#define Error(message, ...) Win32LogError("Renderer", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define Info(message, ...) Win32LogInfo("Renderer", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)

typedef HWND Window_Handle;
typedef HINSTANCE Process_Handle;
typedef HMODULE Module;
#else
#endif

#include "renderer/opengl_loader.h"

// NOTE(soimn): This is a read-only* global containing opengl function pointers.
//              The only exceptions are when the context is created or recreated, however this
//              does not interfere with the function invocations, as the context is allways created / recreated
//              before any of the containing functions are invoked.
global OpenGL_Binding GLBinding = {};

#include "renderer/opengl.h"

#include "math/trigonometry.h"

struct Render_Batch_Entry
{
    Transform transform;
    Bounding_Sphere bounding_sphere;
    Triangle_Mesh* mesh;
};

struct Render_Batch_Cull_Entry
{
    Triangle_Mesh* mesh;
    M4 mvp_matrix;
};

inline void
RendererPushNewRenderBatchBlock(Render_Batch* batch)
{
    void* new_block = PushSize(batch->arena, sizeof(Render_Batch_Entry) * batch->block_size + sizeof(void*), alignof(void*));
    
    new_block = (U8*) new_block + sizeof(void*);
    
    *(void**)((U8*) batch->current_block - sizeof(void*)) = new_block;
    batch->current_block = (Render_Batch_Entry*) new_block;
    
    batch->block_count++;
}

RENDERER_PUSH_MESH_FUNCTION(RendererPushMesh)
{
    if (batch->current_entry_count >= batch->block_count)
    {
        if (batch->current_block_index == batch->block_count - 1)
        {
            RendererPushNewRenderBatchBlock(batch);
        }
        
        else
        {
            batch->current_block = (Render_Batch_Entry*) *((void**)((U8*) batch->current_block - sizeof(void*)));
        }
        
        batch->current_entry_count = 0;
        ++batch->current_block_index;
    }
    
    Render_Batch_Entry* new_entry = (batch->current_block + batch->current_entry_count);
    
    new_entry->transform       = transform;
    new_entry->bounding_sphere = bounding_sphere;
    new_entry->mesh            = mesh;
    
    ++batch->entry_count, ++batch->current_entry_count;
}

RENDERER_CREATE_PREPPING_BATCH_FUNCTION(RendererCreatePreppingBatch)
{
    Prepped_Render_Batch result = {};
    
    result.first    = PushArray(arena, Render_Batch_Cull_Entry, batch->entry_count);
    result.capacity = batch->entry_count;
    
    result.camera = camera;
    
    M4 view_matrix = Translation(camera.position) * Rotation(-camera.heading);
    M4 perspective_matrix = (Perspective(camera.aspect_ratio, camera.fov, camera.near, camera.far)).m;
    
    result.view_projection = perspective_matrix * view_matrix;
    
    return result;
}

RENDERER_PREP_RENDER_BATCH_FUNCTION(RendererPrepBatch)
{
    Assert(resulting_batch->first && resulting_batch->capacity >= batch->entry_count);
    
    Camera* camera = &resulting_batch->camera;
    
    if (batch->entry_count)
    {
        F32 near_width  = Cos(camera->fov / 2) * camera->near;
        F32 far_width   = Cos(camera->fov / 2) * camera->far;
        F32 near_height = near_width / camera->aspect_ratio;
        F32 far_height  = near_height / camera->aspect_ratio;
        
        V3 frustum_vectors[3] = {};
        
        frustum_vectors[0] = Rotate(Normalized(Vec3(far_width, far_height, camera->far) - Vec3(near_width, near_height, camera->near)), camera->heading);
        
        frustum_vectors[1] = Rotate(Normalized(Vec3(near_width, -near_height, camera->far) - Vec3(near_width, near_height, camera->near)), camera->heading);
        
        frustum_vectors[2] = Rotate(Normalized(Vec3(-near_width, near_height, camera->far) - Vec3(near_width, -near_height, camera->far)), camera->heading);
        
        V3 frustum_planes[4] = {};
        
        frustum_planes[0] = Cross(frustum_vectors[1],  frustum_vectors[0]);
        frustum_planes[1] = Cross(frustum_vectors[0],  frustum_vectors[2]);
        frustum_planes[2] = Cross(frustum_vectors[0],  frustum_vectors[1]);
        frustum_planes[3] = Cross(frustum_vectors[0], -frustum_vectors[1]);
        
        {
            Render_Batch_Entry* scan = batch->first_block;
            V3 view_vector = Rotate(Vec3(0, 0, 1), camera->heading);
            
            do
            {
                U32 entry_count = (scan == batch->current_block ? batch->current_entry_count : batch->block_size);
                
                for (U32 i = 0; i < entry_count; ++i)
                {
                    Render_Batch_Entry* current_entry = scan + i;
                    V3 p = current_entry->bounding_sphere.position - camera->position;
                    
                    if (Inner(frustum_planes[0], p) - current_entry->bounding_sphere.radius > 0) continue;
                    else if (Inner(frustum_planes[1], p) - current_entry->bounding_sphere.radius > 0) continue;
                    else if (Inner(frustum_planes[2], p) - current_entry->bounding_sphere.radius > 0) continue;
                    else if (Inner(frustum_planes[3], p) - current_entry->bounding_sphere.radius > 0) continue;
                    else
                    {
                        F32 dist_from_camera = Inner(view_vector, p);
                        
                        if (dist_from_camera > camera->far || dist_from_camera < camera->near) continue;
                        else
                        {
                            M4 model_matrix = Translation(current_entry->transform.position) * Rotation(current_entry->transform.rotation);
                            
                            Render_Batch_Cull_Entry new_entry = {current_entry->mesh, resulting_batch->view_projection * model_matrix};
                            CopyStruct(&new_entry, resulting_batch->first + resulting_batch->count++);
                        }
                    }
                }
                
                scan = (Render_Batch_Entry*) *(void**)((U8*) scan - sizeof(void*));
            } while (scan != batch->current_block);
            
            // TODO(soimn): sort
        }
    }
}

RENDERER_RENDER_BATCH_FUNCTION(RendererRenderBatch)
{
    Camera* camera = &batch->camera;
    
    (void) camera;
}

RENDERER_CLEAN_BATCH_FUNCTION(RendererCleanBatch)
{
    if (should_deallocate)
    {
        ClearArena(batch->arena);
        batch->first_block = 0;
        batch->block_count = 0;
    }
    
    batch->current_block       = 0;
    batch->current_block_index = 0;
    batch->entry_count         = 0;
    batch->current_entry_count = 0;
}

inline bool
InitRenderer (Platform_API_Functions* platform_api, Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    if (GLLoad(&GLBinding, process_handle, window_handle))
    {
        platform_api->PrepareFrame  = &GLPrepareFrame;
        platform_api->PushMesh      = &RendererPushMesh;
        platform_api->PresentFrame  = &GLPresentFrame;
        
        platform_api->CreatePreppingBatch = &RendererCreatePreppingBatch;
        platform_api->PrepBatch           = &RendererPrepBatch;
        platform_api->RenderBatch         = &RendererRenderBatch;
        platform_api->CleanBatch          = &RendererCleanBatch;
        
        platform_api->CreateTexture = &GLCreateTexture;
        platform_api->DeleteTexture = &GLDeleteTexture;
        
        succeeded = true;
    }
    
    else
    {
        // OpenGL load failed
    }
    
    return succeeded;
}


#undef Error
#undef Info