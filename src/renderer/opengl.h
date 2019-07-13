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

RENDERER_PREPARE_RENDER_FUNCTION(GLPrepareRender)
{
    Render_Info render_info = RendererInitRenderInfo(*camera);
    
    // TODO(soimn): Allocate space for, and constuct the clustered list of lights
    
    U32 command_buffer_size = info_count;
    
    IDC_Sort_Entry* command_buffer = PushArray(temp_memory, IDC_Sort_Entry, command_buffer_size);
    Draw_Param_Entry* draw_param_buffer = PushArray(temp_memory, Draw_Param_Entry, command_buffer_size);
    
    RendererCullAndSortRequests(*camera, render_info, infos, info_count, command_buffer, draw_param_buffer, command_buffer_size);
    // TODO(soimn): Upload the command & draw_param_buffers
    
    render_info.command_buffer_handle    = {};
    render_info.draw_param_buffer_handle = {};
    
    camera->render_info = render_info;
}

RENDERER_RENDER_FUNCTION(GLRender)
{
}