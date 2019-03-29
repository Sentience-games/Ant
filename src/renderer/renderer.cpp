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
#include "renderer/opengl.h"

inline bool
InitRenderer (Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    OpenGL_Binding gl_binding;
    if (GLLoad(&gl_binding, process_handle, window_handle))
    {
    }
    
    else
    {
        // OpenGL load failed
    }
    
    return succeeded;
}

#undef Error
#undef Info