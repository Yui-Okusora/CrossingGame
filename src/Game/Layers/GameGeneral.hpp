#pragma once
#include <cstdint>

inline constexpr int MAX_CAMPAIGN_LEVELS = 10;

enum class GameMode : uint8_t {
    Campaign,
    Endless
};

enum UIWidgetID : uint32_t {
    ID_MM_Start = 1001,
    ID_MM_Continue,
    ID_MM_Settings,
    ID_MM_Exit,

    ID_MODE_Campaign = 1101,
    ID_MODE_Endless,
    ID_MODE_Back,

    ID_SET_MasterSlider = 2001,
    ID_SET_MusicSlider,
    ID_SET_SFXSlider,
    ID_SET_Back,

    ID_NAME_TextBox = 3001,
    ID_NAME_Confirm,
    ID_NAME_Back,

    ID_LOAD_Slot_Base = 4000,
    ID_LOAD_Back = 4020,

    ID_PAUSE_Resume = 5001,
    ID_PAUSE_Restart,
    ID_PAUSE_Save,
    ID_PAUSE_Settings,
    ID_PAUSE_MainMenu,
    ID_PAUSE_Exit,

    ID_POP_SaveRecord = 6001,
    ID_POP_MainMenu,

    ID_SAVE_Slot_Base = 7000,
    ID_SAVE_Back = 7020
};