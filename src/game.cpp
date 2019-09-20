#include "game.h"

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    if (!state->is_initialized)
    {
        { /// Load Default Keymap
            Enum32(KEYCODE)* game_keymap   = game_memory->keyboard_game_keymap;
            Enum32(KEYCODE)* editor_keymap = game_memory->keyboard_editor_keymap;
            
            game_keymap[Button_Test]         = Key_A;
            editor_keymap[EditorButton_Test] = Key_T;
        }
        
        state->is_initialized = true;
    }
    
    Game_Controller_Input controller = GetController(input, 0);
    
    if (WasPressed(controller, Button_Test)) Platform->Log(Log_Info | Log_Verbose, "Game button pressed");
    if (WasPressed(input->editor_buttons[EditorButton_Test])) Platform->Log(Log_Info | Log_Verbose, "Editor button pressed");
}