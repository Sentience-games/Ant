#include "renderer/renderer.h"

#ifdef ANT_PLATFORM_WINDOWS
#define Error(message, ...) Win32LogError("Renderer", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define Info(message, ...) Win32LogInfo("Renderer", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)

typedef HWND Window_Handle;
typedef HINSTANCE Process_Handle;
typedef HMODULE Module;
#else
#endif

global Renderer_Context RendererContext = {};

inline void* 
RendererAllocateMemory(Memory_Index size, U8 alignment, bool should_zero = false)
{
    void* result = NULL;
    
    // TODO(soimn): Lock
    
    result = PushSize(&RendererContext.renderer_arena, size, alignment, should_zero);
    
    // TODO(soimn): Unlock
    
    return result;
}

RENDERER_CREATE_TEXTURE_CATALOG_FUNCTION(RendererCreateTextureCatalog)
{
    Texture_Catalog* catalog = NULL;
    
    // NOTE(soimn): This might not be necessary anymore, as texture_catalogs is not a global anymore, and more 
    //              information, e.g. catalog count, could be added instead of looping over the catalog list every 
    //              time a new catalog is to be created.
    for (Texture_Catalog* scan = &RendererContext.texture_catalogs[0]; scan < &RendererContext.texture_catalogs[0] + ARRAY_COUNT(RendererContext.texture_catalogs);
         ++scan)
    {
        if (!scan->is_initialized)
        {
            catalog = scan;
            break;
        }
    }
    
    // TODO(soimn): The game should never use more resources than the defined
    //              upper limit (RENDERER_MAX_CATALOG_COUNT) in release mode, however
    //              in debug/development mode the game could approach or even exced this limit.
    //              Consider handling this in a way that is not as destructive for the content
    //              creators.
    Assert(catalog);
    
    *catalog = {};
    
    ZeroArray(catalog->groups, ARRAY_COUNT(catalog->groups));
    
    catalog->is_initialized = true;
    
    catalog->group_count   = 0;
    catalog->layer_count   = layer_count;
    catalog->mip_levels    = mip_levels;
    
    catalog->type          = type;
    catalog->format        = format;
    catalog->u_wrapping    = u_wrapping;
    catalog->v_wrapping    = v_wrapping;
    catalog->min_filtering = min_filtering;
    catalog->mag_filtering = mag_filtering;
    catalog->max_width     = max_width;
    catalog->max_height    = max_height;
}

inline Texture_Handle*
RendererGetTextureHandle (Texture_Catalog_ID catalog_id, void (*allocate_texture_group) (Texture_Catalog* catalog, Texture_Group* group))
{
    Assert(allocate_texture_group);
    
    Texture_Handle* handle = NULL;
    
    // TODO(soimn): Lock
    
    Texture_Catalog* catalog = &RendererContext.texture_catalogs[catalog_id];
    Assert(catalog->is_initialized);
    
    Texture_Group* group = NULL;
    
    if (catalog->group_count)
    {
        for (Texture_Group* scan = catalog->groups;
             scan < catalog->groups + ARRAY_COUNT(catalog->groups);
             ++scan)
        {
            if (scan->num_free != 0)
            {
                group = scan;
                break;
            }
        }
    }
    
    if (!catalog->group_count || !group)
    {
        // TODO(soimn): The game should never use more resources than the defined
        //              upper limit (RENDERER_MAX_CATALOG_COUNT) in release mode, however
        //              in debug/development mode the game could approach or even exced this limit.
        //              Consider handling this in a way that is not as destructive for the content
        //              creators.
        Assert(catalog->group_count < RENDERER_MAX_GROUP_COUNT_PER_CATALOG);
        
        U8 group_index = (!catalog->group_count ? 0 : catalog->group_count - 1);
        allocate_texture_group(catalog, &catalog->groups[group_index]);
        
        { // Initialize free list
            Texture_Handle* it = catalog->groups[group_index].first;
            for (; it < catalog->groups[group_index].first + catalog->layer_count - 2;
                 ++it)
            {
                it->next_free = it + 1;
            }
            
            it->next_free = NULL;
        }
        
        catalog->groups[group_index].first->next_free = NULL;
        handle = catalog->groups[group_index].first;
        
        ++catalog->group_count;
        ++catalog->groups[group_index].next_free;
        --catalog->groups[group_index].num_free;
        
        group = &catalog->groups[group_index];
    }
    
    else
    {
        handle = group->next_free;
        
        group->next_free  = handle->next_free;
        handle->next_free = NULL;
        --group->num_free;
    }
    
    // TODO(soimn): Unlock
    
    handle->id = {(Texture_Catalog_ID)(catalog - &RendererContext.texture_catalogs[0]), (U8)(group - &catalog->groups[0]), (U8)(handle - group->first), (U8) true};
    
    return handle;
}

RENDERER_REMOVE_TEXTURE_FUNCTION(RendererRemoveTextureFunction)
{
    Assert(id.is_valid);
    
    // TODO(soimn): Lock
    
    Texture_Group* group = &RendererContext.texture_catalogs[id.catalog].groups[id.group];
    
    Texture_Handle* handle = group->first + id.offset;
    handle->next_free      = group->next_free;
    group->next_free       = handle;
    ++group->num_free;
    
    // TODO(soimn): Unlock
}

#include "renderer/opengl_loader.h"
#include "renderer/opengl.h"

inline bool
InitRenderer (Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    if (GLLoad(&GLBinding, process_handle, window_handle))
    {
        RendererContext.PrepareFrame = &GLPrepareFrame;
        RendererContext.PresentFrame = &GLPresentFrame;
        
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