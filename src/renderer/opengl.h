struct GL_DrawCommand
{
    U32 vertex_count;
    U32 instance_count;
    U32 first_index;
    U32 base_vertex;
    U32 base_instance;
};

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

RENDERER_RENDER_FUNCTION(GLRender)
{
    while (camera->batch->current_block->previous)
    {
        camera->batch->current_block = camera->batch->current_block->previous;
    }
    
    camera->batch->current_index = 0;
    camera->batch->entry_count   = 0;
    camera->is_prepared = false;
}