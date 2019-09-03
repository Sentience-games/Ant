#include "renderer/ant_renderer.h"
#include "renderer/ant_renderer.cpp"

internal bool
Win32InitRenderer(Platform_API* platform_api, Enum8(RENDERER_API) preferred_api)
{
    bool succeeded = false;
    
    Enum8(RENDERER_API) selected_api = RendererAPI_None;
    
    // TODO(soimn): Init Vulkan/OpenGL/DX/Metal
    
    if (selected_api != RendererAPI_None)
    {
        // TODO(soimn): Setup platform_api function calls and Renderer_API_Function_Table
        succeeded = true;
    }
    
    return succeeded;
}