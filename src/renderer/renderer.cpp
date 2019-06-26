#include "renderer/renderer.h"

#include "ant_shared.h"
#include "ant_memory.h"

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

RENDERER_PUSH_MESH_FUNCTION(RendererPushMesh)
{
    Render_Batch* batch = camera->batch;
    
    if (!camera->is_prepared)
    {
        Assert(camera->default_block_size && camera->fov && camera->near && camera->far);
        
        UMM block_size = sizeof(Render_Batch_Block) + camera->default_block_size * sizeof(Render_Batch_Entry);
        Assert(MaxAlignOfPointer((Render_Batch_Block*)0 + 1) == 8 && MaxAlignOfPointer((Render_Batch_Block*)0 + 1) == MaxAlignOfPointer((Render_Batch*)0 + 1));
        
        if (!batch)
        {
            batch = BootstrapPushSize(Render_Batch, arena, block_size);
            batch->current_block  = (Render_Batch_Block*)(batch + 1);
            *batch->current_block = {0, 0, camera->default_block_size};
        }
        
        while (batch->current_block->previous)
        {
            batch->current_block = batch->current_block->previous;
        }
        
        { /// Find new culling vectors
            // NOTE(soimn): This is a collection of vectors which start from the upper left corner of the cameras 
            //              frustum, nearest to the near clip plane, and point towards the positive direction of 
            //              each axis along the bounds of the frustum. The "bridge" points from the upper left 
            //              corner nearest the near clip plane, to the upper left corner nearest the far clip 
            //              plane.
            V3 right  = {1.0f, 0.0f, 0.0f};
            V3 down   = {0.0f, 1.0f, 0.0f};
            V3 bridge = {};
            {
                F32 l  = camera->far - camera->near;
                F32 dx = Tan(camera->fov / 2.0f) * l;
                F32 dy = dx * (1.0f / camera->aspect_ratio);
                
                bridge = Normalized(Vec3(-dx, -dy, 1.0f));
            }
            
            V3 plane_up    = Cross(right, bridge);
            V3 plane_down  = {-plane_up.x, -plane_up.y, plane_up.z};
            V3 plane_left  = Cross(bridge, down);
            V3 plane_right = {-plane_left.x, -plane_left.y, plane_left.z};
            
            camera->culling_vectors[0] = plane_up;
            camera->culling_vectors[1] = plane_down;
            camera->culling_vectors[2] = plane_left;
            camera->culling_vectors[2] = plane_right;
        }
        
        camera->view_projection_matrix = Perspective(camera->aspect_ratio, camera->fov, camera->near, camera->far).m * Rotation(camera->rotation);
        
        camera->is_prepared = true;
    }
    
    V3 to_p = bounding_sphere.position - camera->position;
    to_p = Rotate(to_p, Conjugate(camera->rotation));
    
    if (Inner(to_p, camera->culling_vectors[0]) - bounding_sphere.radius <= 0.0f && Inner(to_p, camera->culling_vectors[1]) - bounding_sphere.radius <= 0.0f && Inner(to_p, camera->culling_vectors[2]) - bounding_sphere.radius <= 0.0f && Inner(to_p, camera->culling_vectors[3]) - bounding_sphere.radius <= 0.0f &&
        to_p.z >= camera->near && to_p.z <= camera->far)
    {
        M4 mvp = camera->view_projection_matrix * ModelMatrix(transform).m;
        Vertex_Buffer vertex_buffer = mesh->vertex_buffer;
        Index_Buffer index_buffer   = mesh->index_buffer;
        
        U32 lists_left = mesh->triangle_list_count;
        while (lists_left)
        {
            for (U32 i = 0; i < MAX(lists_left, batch->current_block->size - batch->current_index); ++i)
            {
                Triangle_List* current_list = &mesh->triangle_lists[i];
                
                Render_Batch_Entry* entry = (Render_Batch_Entry*)(batch->current_block + 1) + batch->current_index;
                
                entry->vertex_buffer = {vertex_buffer.handle, vertex_buffer.offset + current_list->offset};
                entry->index_buffer  = {index_buffer.handle, index_buffer.offset + current_list->offset};
                entry->material      = current_list->material;
                entry->vertex_count  = current_list->vertex_count;
                entry->mvp           = mvp;
                
                ++batch->current_index;
            }
            
            if (batch->current_index == batch->current_block->size)
            {
                batch->current_index = 0;
                
                if (batch->current_block->next)
                {
                    batch->current_block = batch->current_block->next;
                }
                
                else
                {
                    UMM block_size = sizeof(Render_Batch_Block) + camera->default_block_size * sizeof(Render_Batch_Entry);
                    
                    Render_Batch_Block* new_block = (Render_Batch_Block*) PushSize(batch->arena, block_size, alignof(Render_Batch_Block));
                    
                    *new_block = {0, 0, camera->default_block_size};
                    
                    batch->current_block->next = new_block;
                    new_block->previous = batch->current_block;
                    
                    batch->current_block = new_block;
                }
            }
        }
        
    }
}

inline bool
InitRenderer (Platform_API_Functions* platform_api, Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    if (GLLoad(&GLBinding, process_handle, window_handle))
    {
        platform_api->PrepareFrame  = &GLPrepareFrame;
        platform_api->PresentFrame  = &GLPresentFrame;
        
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