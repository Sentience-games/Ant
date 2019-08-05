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
            Enum32(KEYCODE)* game_keymap   = game_memory->keyboard_game_keymap;
            Enum32(KEYCODE)* editor_keymap = game_memory->keyboard_editor_keymap;
            
            game_keymap[Button_Test]         = Key_A;
            editor_keymap[EditorButton_Test] = Key_T;
        }
        
        state->is_initialized = true;
    }
}

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    Game_Controller_Input controller = GetController(input, 0);
    
    if (WasPressed(controller, Button_Test)) Platform->Log(Log_Info | Log_Verbose, "Game button pressed");
    if (WasPressed(input->editor_buttons[EditorButton_Test])) Platform->Log(Log_Info | Log_Verbose, "Editor button pressed");
}