#include "PauseMenu.hpp"
#include "SaveLoadLayer.hpp"
#include "SettingsLayer.hpp"
#include "MainMenu.hpp"
#include <iostream>

void PauseMenuLayer::onAttach(EngineContext* ctx) { std::cout << "[Layer] Pause Menu Opened -> Gameplay Frozen & Input Disabled\n"; }
void PauseMenuLayer::onDetach(EngineContext* ctx) { std::cout << "[Layer] Pause Menu Closed -> Resuming Gameplay\n"; }
void PauseMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void PauseMenuLayer::update(double dt, EngineContext* ctx) {}
void PauseMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(750, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0, 0, 0, 0.7f}, .no_texture = true });
    writeBuffer.push_command(760, 0, RectPayload{ .dest_rect = m_panelBounds, .color = {0.1f, 0.12f, 0.15f, 1.0f}, .no_texture = true });

    // RESUME -> Detaches Pause, input & updates naturally resume on Gameplay & HUD!
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Resume, { 480.0f, 250.0f, 240.0f, 45.0f }, "RESUME")) {
        ctx->layerStack->deferDetach(this);
    }

    // ACT -> Push Save Overlay on top of Pause
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Save, { 480.0f, 310.0f, 240.0f, 45.0f }, "SAVE GAME")) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
    }

    // ADD FRONT / DIS INPUT -> Push Settings Overlay on top of Pause
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Settings, { 480.0f, 370.0f, 240.0f, 45.0f }, "SETTINGS")) {
        ctx->layerStack->deferAttach(std::make_unique<SettingsMenuLayer>());
    }

    // RESET STACK -> Wipes all layers (Gameplay, HUD, Pause) -> Main Menu
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_MainMenu, { 480.0f, 430.0f, 240.0f, 45.0f }, "MAIN MENU")) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
        return;
    }

    // EXIT
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Exit, { 480.0f, 490.0f, 240.0f, 45.0f }, "EXIT GAME")) {
        ctx->isRunning.store(false, std::memory_order_relaxed);
    }
}