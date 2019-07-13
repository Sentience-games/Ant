#include "renderer/renderer.h"

#include "ant_shared.h"
#include "ant_memory.h"

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
//              * The only exceptions are when the context is created or recreated, however this
//                does not interfere with the function invocations, as the context is always created / recreated
//                before any of the containing functions are invoked.
global OpenGL_Binding GLBinding = {};

struct Indirect_Draw_Command
{
    U32 vertex_count;
    U32 instance_count;
    U32 first_index;
    I32 base_vertex;
    U32 base_instance;
};

struct IDC_Sort_Entry
{
    U64 sort_key;
    Indirect_Draw_Command command;
};

struct Draw_Param_Entry
{
    M4 mvp;
    U32 material;
};

internal Render_Info
RendererInitRenderInfo(Camera camera)
{
    Render_Info result = {};
    
    V3 right  = {1.0f, 0.0f, 0.0f};
    V3 down   = {0.0f, 1.0f, 0.0f};
    V3 bridge = {};
    {
        F32 l  = camera.far - camera.near;
        F32 dx = Tan(camera.fov / 2.0f) * l;
        F32 dy = dx * (1.0f / camera.aspect_ratio);
        
        bridge = Normalized(Vec3(-dx, -dy, 1.0f));
    }
    
    V3 plane_up    = Cross(right, bridge);
    V3 plane_down  = {-plane_up.x, -plane_up.y, plane_up.z};
    V3 plane_left  = Cross(bridge, down);
    V3 plane_right = {-plane_left.x, -plane_left.y, plane_left.z};
    
    result.culling_vectors[0] = Normalized(plane_up);
    result.culling_vectors[1] = Normalized(plane_down);
    result.culling_vectors[2] = Normalized(plane_left);
    result.culling_vectors[3] = Normalized(plane_right);
    
    result.view_projection_matrix = Perspective(camera.aspect_ratio, camera.fov, camera.near, camera.far).m * ViewMatrix(camera.position, camera.rotation).m;
    
    return result;
}

// TODO(soimn): Extra draw params
// TODO(soimn): Shader id
// TODO(soimn): Vertex and index buffer location
// TODO(soimn): Faster sorting
internal void
RendererCullAndSortRequests(Camera camera, Render_Info render_info, Mesh_Rendering_Info* infos, U32 info_count, IDC_Sort_Entry* command_buffer, Draw_Param_Entry* draw_param_buffer, U32 buffer_size)
{
    U32 current_command_count = 0;
    
    Mesh_Rendering_Info* end = infos + MAX(info_count, buffer_size);
    for (Mesh_Rendering_Info* current = infos; current < end; ++current)
    {
        if (current->mesh && (current->camera_filter & camera.filter) == camera.filter)
        {
            V3 to_p      = current->bounding_sphere.position - camera.position;
            to_p         = Rotate(to_p, Conjugate(camera.rotation));
            F32 p_radius = current->bounding_sphere.radius;
            
            if (Inner(to_p, render_info.culling_vectors[0]) - p_radius <= 0.0f && Inner(to_p, render_info.culling_vectors[1]) - p_radius <= 0.0f && Inner(to_p, render_info.culling_vectors[2]) - p_radius <= 0.0f && Inner(to_p, render_info.culling_vectors[3]) - p_radius <= 0.0f &&
                to_p.z >= camera.near && to_p.z <= camera.far)
            {
                M4 mvp = render_info.view_projection_matrix * ModelMatrix(current->transform).m;
                
                Vertex_Buffer_Handle vertex_buffer = current->mesh->vertex_buffer;
                Index_Buffer_Handle index_buffer   = current->mesh->index_buffer;
                
                // NOTE(soimn): This restricts the vertex and index buffers to the same gpu side memory buffer
                // TODO(soimn): Consider relaxing this constraint
                Assert(vertex_buffer.index == index_buffer.index);
                
                U64 sort_key = 0;
                {
                    U16 buffer_id = (U16) vertex_buffer.index;
                    
                    // TODO(soimn): Find out where the shader should be specified
                    Shader_ID shader = 0;
                    
                    U32 depth = MAX((U32) to_p.z, U32_MAX);
                    
                    sort_key = ((U64)buffer_id << 55) | ((U64)shader << 47) | (U64)depth;
                }
                
                Triangle_List* triangle_lists = current->mesh->triangle_lists;
                U32 list_count = current->mesh->triangle_list_count;
                
                for (Triangle_List* list = triangle_lists; list < triangle_lists + list_count; ++list)
                {
                    IDC_Sort_Entry* new_command = &command_buffer[current_command_count];
                    new_command->sort_key = sort_key;
                    
                    new_command->command.vertex_count = list->vertex_count;
                    new_command->command.first_index  = list->offset + index_buffer.offset;;
                    new_command->command.base_vertex  = vertex_buffer.offset;
                    
                    new_command->command.base_instance = current_command_count;
                    
                    Draw_Param_Entry* new_draw_param = &draw_param_buffer[current_command_count];
                    
                    new_draw_param->mvp      = mvp;
                    new_draw_param->material = list->material;
                    
                    ++current_command_count;
                }
            }
        }
    }
    
    for (U32 i = 0; i < current_command_count - 1; ++i)
    {
        for (U32 j = i + 1; j < current_command_count; ++j)
        {
            bool should_swap = false;
            
            U16 buffer_i = (U16)(command_buffer[i].sort_key & ((U64)0xFFFF0000 << 32));
            U16 buffer_j = (U16)(command_buffer[j].sort_key & ((U64)0xFFFF0000 << 32));
            
            if (buffer_i > buffer_j)
            {
                should_swap = true;
            }
            
            else if (buffer_i == buffer_j)
            {
                U16 shader_i = (U16)(command_buffer[i].sort_key & ((U64)0xFFFF << 32));
                U16 shader_j = (U16)(command_buffer[j].sort_key & ((U64)0xFFFF << 32));
                
                if (shader_i > shader_j)
                {
                    should_swap = true;
                }
                
                else if (shader_i == shader_j)
                {
                    U32 depth_i = (command_buffer[i].sort_key & 0xFFFFFFFF);
                    U32 depth_j = (command_buffer[j].sort_key & 0xFFFFFFFF);
                    
                    if (depth_i > depth_j)
                    {
                        should_swap = true;
                    }
                }
            }
            
            if (should_swap)
            {
                IDC_Sort_Entry temp = {};
                CopyStruct(&command_buffer[i], &temp);
                CopyStruct(&command_buffer[j], &command_buffer[i]);
                CopyStruct(&temp, &command_buffer[j]);
            }
        }
    }
}

#include "renderer/opengl.h"

inline bool
InitRenderer (Platform_API_Functions* platform_api, Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    if (GLLoad(&GLBinding, process_handle, window_handle))
    {
        platform_api->PrepareFrame  = &GLPrepareFrame;
        platform_api->PresentFrame  = &GLPresentFrame;
        
        platform_api->PrepareRender = &GLPrepareRender;
        platform_api->Render        = &GLRender;
        
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