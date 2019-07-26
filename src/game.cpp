#include "game.h"

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    if (!state->is_initialized)
    {
        { /// Init VFS
            game_memory->vfs = PushStruct(game_memory->persistent_arena, VFS);
            VFS* vfs         = game_memory->vfs;
            
            vfs->arena = PushStruct(game_memory->persistent_arena, Memory_Arena);
            
            VFS_Mounting_Point temp_mounting_points[] = {
                {CONST_STRING("./data"), CONST_STRING(".")},
            };
            
            String temp_essential_files[] = {
                CONST_STRING("global.vars"),
            };
            
            vfs->mounting_point_count = ARRAY_COUNT(temp_mounting_points);
            vfs->essential_file_count = ARRAY_COUNT(temp_essential_files);
            
            vfs->mounting_points = PushArray(game_memory->persistent_arena, VFS_Mounting_Point, vfs->mounting_point_count);
            CopyArray(temp_mounting_points, vfs->mounting_points, vfs->mounting_point_count);
            
            vfs->essential_files = PushArray(game_memory->persistent_arena, String, vfs->essential_file_count);
            CopyArray(temp_essential_files, vfs->essential_files, vfs->essential_file_count);
            
            Platform->ReloadVFS(game_memory->vfs);
        }
        
        state->is_initialized = true;
    }
}