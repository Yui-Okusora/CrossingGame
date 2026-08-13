#include <Engine/Engine.hpp>
#include <iostream>
#include <string>
#include <memory>

#include "Game/Layers/MainMenu.hpp"
#include "Game/Layers/Gameplay.hpp"
#include "Game/Layers/PauseMenu.hpp"
#include "Game/Layers/SaveLoadLayer.hpp"
#include "Game/Layers/SettingsLayer.hpp"
#include "Game/Layers/WinLoseLayer.hpp"
#include "Game/Layers/HUD.hpp"


// ============================================================================
// SYSTEM ENTRY DRIVER
// ============================================================================
int main() {
    try {
        std::cout << "[Horizon Engine] Starting Flowchart Backbone Application...\n";
        Application app("CrossingGame", 1200, 805);
        EngineContext& ctx = app.getContext();

        // --- BUSES ---
        TextureHandle texBusLong = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Bus/LongBus.png");
        TextureHandle texBusShortSheet = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Bus/shortBus-sheet.png");

        // --- CODE OBSTACLES ---
        TextureHandle texCodeCondition = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-Condition.png");
        TextureHandle texCodeMissing = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-Missing_.png");
        TextureHandle texCodeNullPtr = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-NullPtr.png");
        TextureHandle texCodeOutOfMemory = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-OutOfMemory.png");
        TextureHandle texCodeStackOverflow = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-StackOverflow.png");
        TextureHandle texCodeWhile = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-While.png");
        TextureHandle texCodeLine = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine.png");

        // --- DEADLINE POPUP ---
        TextureHandle texDeadlinePopup = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/DeadlinePopup/DeadlinePopup.png");

        // --- ELEVATOR ---
        TextureHandle texElevatorClosed = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/elevator-closed.png");
        TextureHandle texElevatorClosing = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/elevator-closing.png");
        TextureHandle texElevatorOpened = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/elevator-opened.png");
        TextureHandle texElevatorOpening = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/elevator-opening.png");
        TextureHandle texElevatorRoad = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/ElevatorRoad.png");
        TextureHandle texWaitingLine = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/WaitingLine.png");

        // --- EXAM PAPER & FONT ---
        TextureHandle texExamPaper = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/ExamPaper/ExamPaper.png");
        TextureHandle texFontBoldPixels = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Font/BoldPixels.png");

        // --- GAME HUD ---
        TextureHandle texIconBuff = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/GameHUD/iconBuff.png");
        TextureHandle texNewUI = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/GameHUD/NewUI.png");
        TextureHandle texPointMeter = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/GameHUD/PointMeter/PointMeter.png");

        // --- GRASS, ROAD & TERRAIN ---
        TextureHandle texGrassEnd = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassEnd.png");
        TextureHandle texGrassSafePath = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassSafePath.png");
        TextureHandle texGrassStart = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassStart.png");
        TextureHandle texLevelUpLine = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/LevelUpLine.png");
        TextureHandle texStoneBench = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/Stone Bench.png");
        TextureHandle texRiver = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/River/River.png");
        TextureHandle texRoad = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Road/Road.png");

        // --- MAIN MENU ---
        TextureHandle texContinueButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Continue button.png");
        TextureHandle texDonateButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/DonateButton.png");
        TextureHandle texExitButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Exit button.png");
        TextureHandle texMainBackground = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/MainBackground.png");
        TextureHandle texOptionButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Option button.png");
        TextureHandle texPlayButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Play button.png");

        // --- MAIN CHARACTER ---
        TextureHandle texAuraBuff = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/auraBuff.png");
        TextureHandle texCatAnim = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/Cat character rework - animation.png");
        TextureHandle texFallWater = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/Fallwater.png");
        TextureHandle texLoseAnim = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/loseAnimation.png");
        TextureHandle texWinAnim = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/winAnimation.png");

        // --- MENUS & OVERLAYS ---
        TextureHandle texMainMenuButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/MainMenuButton.png");
        TextureHandle texMenuButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/MenuButton.png");
        TextureHandle texMenuPanel = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/MenuPanel.png");
        TextureHandle texPlayButtonUI = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/PlayButton.png");
        TextureHandle texRestartButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/RestartButton.png");
        TextureHandle texSaveButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/SaveButton.png");
        TextureHandle texSettingButton = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/SettingButton.png");
        TextureHandle texMusicSlider = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/MusicSetting/MusicSlider.png");
        TextureHandle texSaveGameBox = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/SaveGame/savegame box.png");

        // --- RESULT SCREEN ---
        TextureHandle texBachelorBadge = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Result screen/BachelorBadge.png");
        TextureHandle texLoseScreen = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Result screen/LoseScreen.png");
        TextureHandle texMasterBadge = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Result screen/MasterBadge.png");
        TextureHandle texPhDBadge = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Result screen/PhDBadge.png");
        TextureHandle texWinScreen = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Result screen/WinScreen.png");

        // --- TEACHERS ---
        TextureHandle texTeacherQuan = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Teacher/Teacher Quan - animation.png");
        TextureHandle texTeacherTinh = ctx.assetManager.loadTexture(RESOURCES_PATH "Sprite/Teacher/Teacher Tinh - animation.png");

        ctx.layerStack->pushLayer(std::make_unique<MainMenuLayer>(), &ctx);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error]: " << e.what() << "\n";
        return -1;
    }
    return 0;
}