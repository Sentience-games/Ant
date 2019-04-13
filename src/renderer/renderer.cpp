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

RENDERER_PUSH_MESH_FUNCTION(RendererPushMesh)
{
    
}

RENDERER_RENDER_BATCH_FUNCTION(RendererRenderBatch)
{
    
}

RENDERER_REQUEST_BATCH_FUNCTION(RendererRequestBatch)
{
    Assert(context->render_batch_count < RENDERER_MAX_RENDER_BATCH_COUNT);
    return context->render_batch_count++;
}

RENDERER_CLEAN_BATCH_FUNCTION(RendererCleanBatch)
{
    Assert(render_batch_id < context->render_batch_count);
    Render_Batch* batch = &context->render_batches[render_batch_id];
    
    ResetMemoryArena(&batch->arena);
    batch->entry_count = 0;
}

inline bool
InitRenderer (Renderer_Context* context, Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    if (GLLoad(&GLBinding, process_handle, window_handle))
    {
        context->PrepareFrame  = &GLPrepareFrame;
        context->PushMesh      = &RendererPushMesh;
        context->RenderBatch   = &RendererRenderBatch;
        context->PresentFrame  = &GLPresentFrame;
        
        context->RequestBatch  = &RendererRequestBatch;
        context->CleanBatch    = &RendererCleanBatch;
        
        context->CreateTexture = &GLCreateTexture;
        context->DeleteTexture = &GLDeleteTexture;
        
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