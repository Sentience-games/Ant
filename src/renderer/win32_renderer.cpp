#include "renderer/ant_renderer.h"
#include "renderer/ant_renderer.cpp"

internal bool
Win32InitRenderer(Platform_API* platform_api, Enum8(RENDERER_API) preferred_api, Memory_Arena* state_arena, Memory_Arena* work_arena, Renderer_Settings settings)
{
    bool succeeded = false;
    
    Enum8(RENDERER_API) selected_api = RendererAPI_None;
    
    // TODO(soimn): Init Vulkan/OpenGL/DX/Metal
    
    if (selected_api != RendererAPI_None)
    {
        // TODO(soimn): Setup platform_api function calls and Renderer_API_Function_Table
        succeeded = true;
    }
    
    if (selected_api != RendererAPI_None)
    {
        RendererGlobals = {};
        RendererGlobals.api = selected_api;
        
        RendererGlobals.settings = settings;
        
        RendererGlobals.state_arena = state_arena;
        RendererGlobals.work_arena  = work_arena;
        
        RendererGlobals.memory_blocks = BUCKET_ARRAY(state_arena, GPU_Memory_Block, 8);
        
        RendererGlobals.commands           = BUCKET_ARRAY(work_arena, Render_Command, 32);
        RendererGlobals.render_batch_array = BUCKET_ARRAY(work_arena, Render_Command, 8);
        RendererGlobals.light_batch_array  = BUCKET_ARRAY(work_arena, Render_Command, 8);
    }
    
    return succeeded;
}