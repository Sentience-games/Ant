#include "game.h"

GAME_INIT_FUNCTION(GameInit)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    // NOTE(soimn): This handles initialization of persistent data
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
        
        { /// Load default keymap
            // TODO(soimn): Possibly load keymaps from a file
            
            ZeroArray(game_memory->controller_infos, GAME_MAX_CONTROLLER_COUNT);
            
            // NOTE(soimn): Apply default keymaps
            for (U32 i = 0; i < GAME_MAX_CONTROLLER_COUNT; ++i)
            {
                U64* keymap = &game_memory->controller_infos[i].keymap[0];
                
                keymap[0] = Key_A;
                
                game_memory->controller_infos[i].was_remapped = true;
            }
        }
        
        state->is_initialized = true;
    }
}

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    Game_Controller_Input* controller = &input->controllers[0];
    
    if (WasPressed(controller->test_button))
    {
        LOG_INFO("Pressed!");
    }
    
    if (WasHeld(controller->test_button))
    {
        LOG_INFO("Held!");
    }
    
    if (IsHeld(controller->test_button))
    {
        LOG_INFO("Is held!");
    }
}