#include "renderer/ant_renderer.h"
#include "renderer/ant_renderer.cpp"

internal bool
Win32InitRenderer(Platform_API* platform_api, Enum8(RENDERER_API) preferred_api, Memory_Arena* state_arena, Memory_Arena* work_arena)
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
        
        RendererGlobals.state_arena = state_arena;
        RendererGlobals.work_arena  = work_arena;
        
        for (U32 i = 0; i < RENDERER_MAX_GPU_MEMORY_BLOCK_COUNT; ++i)
        {
            GPU_Memory_Block* current = &RendererGlobals.memory_blocks[i];
            *current = {};
            
            U32 free_list_size = RENDERER_GPU_MEMORY_BLOCK_SIZE / RENDERER_MIN_ALLOCATION_SIZE;
            current->free_list_size = free_list_size;
            current->free_list = PushArray(state_arena, GPU_Free_List, free_list_size);
        }
        
        RendererGlobals.commands           = BUCKET_ARRAY(work_arena, Render_Command, 32);
        RendererGlobals.render_batch_array = BUCKET_ARRAY(work_arena, Render_Command, 8);
        RendererGlobals.light_batch_array  = BUCKET_ARRAY(work_arena, Render_Command, 8);
    }
    
    return succeeded;
}