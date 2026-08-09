#pragma once
#include <memory>

// ============================================================================
// UNIQUE WIDGET IDS FOR UI ACTIONS
// ============================================================================
enum UIWidgetID : uint32_t {
    ID_MM_Start = 1001,
    ID_MM_Continue,
    ID_MM_Settings,
    ID_MM_Exit,

    ID_SET_VolumeSlider = 2001,
    ID_SET_Back,

    ID_NAME_TextBox = 3001,
    ID_NAME_Confirm,
    ID_NAME_Back,

    ID_LOAD_Slot1 = 4001,
    ID_LOAD_Slot2,
    ID_LOAD_Back,

    ID_PAUSE_Resume = 5001,
    ID_PAUSE_Save,
    ID_PAUSE_Settings,
    ID_PAUSE_MainMenu,
    ID_PAUSE_Exit,

    ID_POP_SaveRecord = 6001,
    ID_POP_MainMenu,

    ID_SAVE_Slot1 = 7001,
    ID_SAVE_Back
};