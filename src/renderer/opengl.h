#pragma once

#include "ant_types.h"
#include "renderer/opengl_loader.h"

RENDERER_PREPARE_FRAME_FUNCTION(GLPrepareFrame)
{
    GLBinding.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GLBinding.glClearDepthf(1.0f);
    GLBinding.glClearStencil(0);
    GLBinding.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

RENDERER_PRESENT_FRAME_FUNCTION(GLPresentFrame)
{
#ifdef ANT_PLATFORM_WINDOWS
    SwapBuffers(GLBinding.device_context);
#endif
}