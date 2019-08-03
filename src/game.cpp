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
        
        { /// Load Default Keymap
            Enum16(KEYCODE) default_keyboard_keymap[] = {
                /* Button_Test */ Key_A,
            };
            
            for (U32 i = 0; i < GAME_MAX_ACTIVE_CONTROLLER_COUNT + 1; ++i)
            {
                CopyArray(default_keyboard_keymap, game_memory->controller_infos[i].keyboard_keymap, GAME_BUTTON_COUNT);
            }
            
            game_memory->active_controller_count = 0;
        }
        
        state->is_initialized = true;
    }
}

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    Game_Controller_Input* controller = GetController(input, 0);
    
    U32 count = 0;
    if (WasPressed(controller, Button_Test, &count))
    {
        Platform->Log(Log_Info, "Button pressed %u time(s)", count);
    }
    
    F32 duration = 0;
    if (WasHeld(controller, Button_Test, &duration))
    {
        Platform->Log(Log_Info, "Button held for %ums", (U32)(duration * 1000.0f));
    }
}