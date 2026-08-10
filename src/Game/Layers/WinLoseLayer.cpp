#include "WinLoseLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "MainMenu.hpp"
#include "Gameplay.hpp"
#include "HUD.hpp"
#include <iostream>
#include <cstdio>

void WinLosePopupLayer::onAttach(EngineContext* ctx) {
    std::cout << "[WinLosePopupLayer] Game end overlay opened -> Suspending game simulation.\n";

    // Retrieve end-of-match metrics from Blackboard
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) {
        m_playerName = *nameOpt;
    }
    if (auto scoreOpt = ctx->blackboard.get<int>("currentScore")) {
        m_finalScore = *scoreOpt;
    }
    if (auto highScoreOpt = ctx->blackboard.get<int>("highestScore")) {
        m_highestScore = *highScoreOpt;
    }
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) {
        m_currentLevel = *levelOpt;
    }

    // Victory condition if max level (5) is reached with score intact
    m_isVictory = (m_currentLevel >= 5);
}

void WinLosePopupLayer::onDetach(EngineContext* ctx) {
    std::cout << "[WinLosePopupLayer] Game end overlay closed.\n";
}

void WinLosePopupLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS) {
            // Pressing 'Y' triggers an instant level restart
            if (ev.key == GLFW_KEY_Y) {
                ctx->layerStack->deferClear();
                ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
                ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
            }
        }
    }
}

void WinLosePopupLayer::update(double dt, EngineContext* ctx) {
    // Logic updates remain suspended while overlay is active
}

void WinLosePopupLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Semi-transparent backdrop dimming pass
    writeBuffer.push_command(800, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.78f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Dialog Box Container
    writeBuffer.push_command(810, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.13f, 0.16f, 0.96f },
        .no_texture = true,
        .is_world_space = false
        });

    // 3. Dynamic Match Result Banner
    TextPayload titleText;
    titleText.color = m_isVictory ? glm::vec4{ 0.2f, 0.9f, 0.3f, 1.0f }  // Green for Victory
    : glm::vec4{ 0.95f, 0.2f, 0.2f, 1.0f }; // Red for Defeat
    titleText.scale = 28.0f;
    titleText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f };
    titleText.showInCenter = true;
    std::snprintf(titleText.text_content, sizeof(titleText.text_content),
        m_isVictory ? "LEVEL CLEARED!" : "GAME OVER");
    writeBuffer.push_command(820, 0, titleText);

    // 4. Match Statistics Telemetry
    TextPayload statsText;
    statsText.color = { 0.9f, 0.92f, 0.98f, 1.0f };
    statsText.scale = 20.0f;
    statsText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 85.0f };
    statsText.showInCenter = true;
    std::snprintf(statsText.text_content, sizeof(statsText.text_content),
        "FINAL SCORE: %d\nHIGH SCORE: %d\nPRESS 'Y' TO RESTART",
        m_finalScore, m_highestScore);
    writeBuffer.push_command(820, 0, statsText);

    // 5. Interactive UI Action Buttons
    float btnX = m_panelBounds.x + 40.0f;
    float btnW = m_panelBounds.z - 80.0f;
    float btnH = 45.0f;
    float startY = m_panelBounds.y + 180.0f;
    float gap = 58.0f;

    // SAVE RECORD: Attaches SaveMenuLayer to save progress via FileSystem binary serializer
    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_SaveRecord, { btnX, startY, btnW, btnH }, "SAVE RECORD")) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
        return;
    }

    // MAIN MENU: Resets the entire layer stack and navigates back to MainMenuLayer
    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_MainMenu, { btnX, startY + gap, btnW, btnH }, "MAIN MENU")) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
        return;
    }
}