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

        ctx.layerStack->pushLayer(std::make_unique<MainMenuLayer>(), &ctx);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error]: " << e.what() << "\n";
        return -1;
    }
    return 0;
}