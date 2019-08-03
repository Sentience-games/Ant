enum KEYCODE
{
    Key_Invalid = 0,
    
    // Row 0
    Key_Escape,
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    Key_Pause,
    Key_Break,
    Key_Insert,
    Key_PrintScreen,
    Key_Delete,
    Key_SysRq,
    
    // Row 1
    Key_NL0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    Key_0,
    Key_NL1,
    Key_NL2,
    Key_Backspace,
    
    // Alpha
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    
    // NOTE(soimn):
    // US: '[', ']', '\', ';', '''
    // ISO Nordic: 'å', '¨', '*', 'ø', 'æ'
    Key_Int0,
    Key_Int1,
    Key_Int2,
    Key_Int3,
    Key_Int4,
    Key_Int5,
    
    Key_Enter,
    
    Key_Comma,
    Key_Period,
    
    // NOTE(soimn): ISO Nordic: '<', covered by shift in US
    Key_Int6,
    
    // NOTE(soimn): US: '/', ISO Nordic: '-'
    Key_Int7,
    
    // Arrow keys
    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,
    
    // Navigation
    Key_PgUp,
    Key_PgDn,
    Key_Home,
    Key_End,
    
    // Numpad
    Key_KP0,
    Key_KP1,
    Key_KP2,
    Key_KP3,
    Key_KP4,
    Key_KP5,
    Key_KP6,
    Key_KP7,
    Key_KP8,
    Key_KP9,
    Key_NumLock,
    Key_ScrollLock,
    Key_KPDiv,
    Key_KPMult,
    Key_KPMinus,
    Key_KPPlus,
    Key_KPEnter,
    Key_KPDecimal,
    
    // Modifiers
    Key_Tab,
    Key_CapsLock,
    Key_LShift,
    Key_LCtrl,
    Key_LSuper,
    Key_LAlt,
    Key_Space,
    Key_RAlt,
    Key_RSuper,
    Key_RCtrl,
    Key_RShift,
    
    // Mouse buttons
    Key_Mouse1,
    Key_Mouse2,
    Key_Mouse3,
    Key_Mouse4,
    Key_Mouse5,
    
    Key_MouseWheelUp,
    Key_MouseWheelDn,
    
    KEYCODE_KEY_COUNT
};

StaticAssert(KEYCODE_KEY_COUNT <= U32_MAX);