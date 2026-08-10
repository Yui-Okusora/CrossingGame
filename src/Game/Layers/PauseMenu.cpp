#include "PauseMenu.hpp"
#include "SettingsLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "MainMenu.hpp"
#include <iostream>
#include <cstdio>

void PauseMenuLayer::onAttach(EngineContext* ctx) {
    std::cout << "[PauseMenuLayer] Pause menu attached -> Gameplay updates and inputs suspended.\n";
}

void PauseMenuLayer::onDetach(EngineContext* ctx) {
    std::cout << "[PauseMenuLayer] Pause menu detached -> Resuming active gameplay.\n";
}

void PauseMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS) {
            // Pressing ESCAPE or P toggles pause off and resumes gameplay
            if (ev.key == GLFW_KEY_ESCAPE || ev.key == GLFW_KEY_P) {
                ctx->layerStack->deferDetach(this);
            }
        }
    }
}

void PauseMenuLayer::update(double dt, EngineContext* ctx) {
    // Logic updates remain completely frozen while paused
}

void PauseMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Full-screen semi-transparent dimming backdrop
    writeBuffer.push_command(750, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.72f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Pause Menu Dialog Box
    writeBuffer.push_command(760, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.14f, 0.18f, 0.95f },
        .no_texture = true,
        .is_world_space = false
        });

    // 3. Header Title
    TextPayload titleText;
    titleText.color = { 1.0f, 0.85f, 0.2f, 1.0f }; // Gold accent
    titleText.scale = 26.0f;
    titleText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f };
    titleText.showInCenter = true;
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "PAUSED");
    writeBuffer.push_command(770, 0, titleText);

    // 4. Immediate-Mode UI Buttons
    float btnX = m_panelBounds.x + 30.0f;
    float btnW = m_panelBounds.z - 60.0f;
    float btnH = 45.0f;
    float startY = m_panelBounds.y + 75.0f;
    float gap = 58.0f;

    // RESUME: Detaches pause menu layer, returning control to GameplayLayer
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Resume, { btnX, startY, btnW, btnH }, "RESUME")) {
        ctx->layerStack->deferDetach(this);
        return;
    }

    // SAVE GAME: Pushes SaveMenuLayer on top of PauseMenuLayer
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Save, { btnX, startY + gap, btnW, btnH }, "SAVE GAME")) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
        return;
    }

    // SETTINGS: Pushes SettingsMenuLayer on top of PauseMenuLayer
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Settings, { btnX, startY + (gap * 2.0f), btnW, btnH }, "SETTINGS")) {
        ctx->layerStack->deferAttach(std::make_unique<SettingsMenuLayer>());
        return;
    }

    // MAIN MENU: Clears all running layers (Gameplay, HUD, Pause) and reloads MainMenuLayer
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_MainMenu, { btnX, startY + (gap * 3.0f), btnW, btnH }, "MAIN MENU")) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
        return;
    }

    // EXIT GAME: Gracefully signals application thread to stop
    if (ctx->ui.Button(writeBuffer, ctx, ID_PAUSE_Exit, { btnX, startY + (gap * 4.0f), btnW, btnH }, "EXIT GAME")) {
        ctx->isRunning.store(false, std::memory_order_relaxed);
        return;
    }
}