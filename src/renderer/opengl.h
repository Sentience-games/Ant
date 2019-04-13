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

RENDERER_CREATE_TEXTURE_FUNCTION(GLCreateTexture)
{
    Texture_Handle handle = {};
    handle.type           = type;
    handle.format         = format;
    handle.u_wrapping     = u_wrapping;
    handle.v_wrapping     = v_wrapping;
    handle.min_filtering  = min_filtering;
    handle.mag_filtering  = mag_filtering;
    
    GLBinding.glGenTextures(1, (GLuint*) &handle.id);
    GLBinding.glBindTexture(GL_TEXTURE_2D, (GLuint) handle.id);
    GLBinding.glTexStorage2D(GL_TEXTURE_2D, handle.mip_levels, handle.format, handle.width, handle.height);
    
    GLBinding.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, handle.min_filtering);
    GLBinding.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, handle.mag_filtering);
    GLBinding.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     handle.u_wrapping);
    GLBinding.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     handle.v_wrapping);
    
    handle.handle = GLBinding.glGetTextureHandleARB((GLuint) handle.id);
    GLBinding.glMakeTextureHandleResidentARB(handle.handle);
    
    GLBinding.glBindTexture(GL_TEXTURE_2D, 0);
    
    handle.is_valid = true;
    
    return handle;
}

RENDERER_DELETE_TEXTURE_FUNCTION(GLDeleteTexture)
{
    Assert(handle->is_valid);
    
    GLBinding.glMakeTextureHandleNonResidentARB(handle->handle);
    GLBinding.glDeleteTextures(1, &handle->id);
    handle->is_valid = true;
}
