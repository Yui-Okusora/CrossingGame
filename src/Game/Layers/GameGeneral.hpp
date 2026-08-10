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

    ID_SET_MasterSlider = 2001,
    ID_SET_MusicSlider,  // 2002
    ID_SET_SFXSlider,    // 2003
    ID_SET_Back,         // 2004

    ID_NAME_TextBox = 3001,
    ID_NAME_Confirm,
    ID_NAME_Back,

    // Reserved range for procedural Load Slots (4000 to 4009)
    ID_LOAD_Slot_Base = 4000,
    ID_LOAD_Back = 4020,

    ID_PAUSE_Resume = 5001,
    ID_PAUSE_Save,
    ID_PAUSE_Settings,
    ID_PAUSE_MainMenu,
    ID_PAUSE_Exit,

    ID_POP_SaveRecord = 6001,
    ID_POP_MainMenu,

    // Reserved range for procedural Save Slots (7000 to 7009)
    ID_SAVE_Slot_Base = 7000,
    ID_SAVE_Back = 7020
};