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

inline void
GLAllocateTextureGroup (Texture_Catalog* catalog, Texture_Group* group)
{
    Assert(catalog->layer_count && catalog->layer_count <= GLBinding.capabilities.max_texture_array_layers);
    
    *group = {};
    
    Texture_Handle* group_handles = (Texture_Handle*) RendererAllocateMemory(RoundToAligned(sizeof(Texture_Handle), alignof(Texture_Handle)) * catalog->layer_count, alignof(Texture_Handle));
    
    GLBinding.glGenTextures(1, (GLuint*) &group->id);
    GLBinding.glBindTexture(GL_TEXTURE_2D_ARRAY, (GLuint) group->id);
    GLBinding.glTexStorage3D(GL_TEXTURE_2D_ARRAY, catalog->mip_levels, catalog->format, catalog->max_width, catalog->max_height, GLBinding.capabilities.max_texture_array_layers);
    
    GLBinding.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, catalog->min_filtering);
    GLBinding.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, catalog->mag_filtering);
    GLBinding.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,     catalog->u_wrapping);
    GLBinding.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,     catalog->v_wrapping);
    
    group->handle = GLBinding.glGetTextureHandleARB((GLuint) group->id);
    GLBinding.glMakeTextureHandleResidentARB(group->handle);
    
    GLBinding.glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    group->first     = group_handles;
    group->next_free = group->first;
    group->num_free  = catalog->layer_count;
}

RENDERER_CREATE_TEXTURE_FUNCTION(GLCreateTexture)
{
    Texture_Handle* handle = RendererGetTextureHandle(catalog_id, &GLAllocateTextureGroup);
    
    return {};
}