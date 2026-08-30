#pragma once
#include <cstdint>
#include <glm/glm.hpp>

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
    ID_SAVE_Back = 7020,

    ID_DEADLINE_Dismiss = 8001
};

struct AcademicBadgeInfo {
    const char* title;
    const char* badgePath;
    glm::vec4 color;
};

inline AcademicBadgeInfo GetAcademicBadgeInfo(GameMode mode, int level, int score) {
    if (mode == GameMode::Campaign) {
        if (level >= 10 && score >= 7000) {
            // Gold Shield
            return { "DOCTOR OF PHILOSOPHY (PH.D)", RESOURCES_PATH "Sprite/Result screen/PhDBadge.png", { 1.0f, 0.85f, 0.2f, 1.0f } };
        }
        if (level >= 7 || score >= 4500) {
            // Blue Shield
            return { "MASTER OF SCIENCE", RESOURCES_PATH "Sprite/Result screen/MasterBadge.png", { 0.3f, 0.85f, 1.0f, 1.0f } };
        }
        if (level >= 4 || score >= 2000) {
            // Silver Shield
            return { "BACHELOR OF SCIENCE", RESOURCES_PATH "Sprite/Result screen/BachelorBadge.png", { 0.85f, 0.90f, 0.95f, 1.0f } };
        }
        // Bronze Shield
        return { "UNDERGRADUATE STUDENT", RESOURCES_PATH "Sprite/Result screen/StudentBadge.png", { 0.95f, 0.60f, 0.35f, 1.0f } };
    }
    else {
        if (score >= 6500) {
            // Gold Shield
            return { "DOCTOR OF PHILOSOPHY (PH.D)", RESOURCES_PATH "Sprite/Result screen/PhDBadge.png", { 1.0f, 0.85f, 0.2f, 1.0f } };
        }
        if (score >= 3500) {
            // Blue Shield
            return { "MASTER OF SCIENCE", RESOURCES_PATH "Sprite/Result screen/MasterBadge.png", { 0.3f, 0.85f, 1.0f, 1.0f } };
        }
        if (score >= 1500) {
            // Silver Shield
            return { "BACHELOR OF SCIENCE", RESOURCES_PATH "Sprite/Result screen/BachelorBadge.png", { 0.85f, 0.90f, 0.95f, 1.0f } };
        }
        // Bronze Shield
        return { "UNDERGRADUATE STUDENT", RESOURCES_PATH "Sprite/Result screen/StudentBadge.png", { 0.95f, 0.60f, 0.35f, 1.0f } };
    }
}