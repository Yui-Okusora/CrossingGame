#include "WinLoseLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "MainMenu.hpp"
#include "Gameplay.hpp"
#include "HUD.hpp"

void WinLosePopupLayer::onAttach(EngineContext* ctx) {
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) m_playerName = *nameOpt;
    if (auto scoreOpt = ctx->blackboard.get<int>("currentScore")) m_finalScore = *scoreOpt;
    if (auto highScoreOpt = ctx->blackboard.get<int>("highestScore")) m_highestScore = *highScoreOpt;
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) m_currentLevel = *levelOpt;

    // Update and persist high score if broken
    if (m_finalScore > m_highestScore) {
        m_highestScore = m_finalScore;
        ctx->blackboard.set("highestScore", m_highestScore);
    }
}

void WinLosePopupLayer::onDetach(EngineContext* ctx) {}

void WinLosePopupLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_Y) {
            ctx->blackboard.set("currentLevel", 1);
            ctx->blackboard.set("currentScore", 0);
            ctx->blackboard.set("elapsedTime", 0.0f);
            // highestScore remains untouched in blackboard

            ctx->layerStack->deferClear();
            ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
            ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
        }
    }
}

void WinLosePopupLayer::update(double dt, EngineContext* ctx) {}

void WinLosePopupLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(800, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.78f },
        .no_texture = true,
        .is_world_space = false
        });

    glm::vec4 panelBounds{ 380.0f, 180.0f, 440.0f, 440.0f };
    writeBuffer.push_command(810, 0, RectPayload{
        .dest_rect = panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    glm::vec4 borderCol = m_isVictory ? glm::vec4{ 0.2f, 0.85f, 0.4f, 1.0f } : glm::vec4{ 0.85f, 0.25f, 0.25f, 1.0f };
    writeBuffer.push_command(815, 0, LinePayload{ {panelBounds.x, panelBounds.y}, {panelBounds.x + panelBounds.z, panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {panelBounds.x, panelBounds.y}, {panelBounds.x, panelBounds.y + panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {panelBounds.x + panelBounds.z, panelBounds.y}, {panelBounds.x + panelBounds.z, panelBounds.y + panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {panelBounds.x, panelBounds.y + panelBounds.w}, {panelBounds.x + panelBounds.z, panelBounds.y + panelBounds.w}, borderCol, 2.0f });

    TextPayload titleText{
        .position = { panelBounds.x + (panelBounds.z * 0.5f), panelBounds.y + 40.0f },
        .color = m_isVictory ? glm::vec4{ 0.2f, 0.95f, 0.35f, 1.0f } : glm::vec4{ 0.95f, 0.25f, 0.25f, 1.0f },
        .scale = 28.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), m_isVictory ? "LEVEL CLEARED!" : "GAME OVER");
    writeBuffer.push_command(820, 0, titleText);

    if (m_isVictory) {
        const char* badgePath = (m_finalScore > 2000) ? RESOURCES_PATH "Sprite/Result screen/PhDBadge.png"
            : (m_finalScore > 1000) ? RESOURCES_PATH "Sprite/Result screen/MasterBadge.png"
            : RESOURCES_PATH "Sprite/Result screen/BachelorBadge.png";
        TextureHandle badgeTex = ctx->assetManager.loadTexture(badgePath);
        writeBuffer.push_command(825, 0, RectPayload{
            .dest_rect = { panelBounds.x + (panelBounds.z * 0.5f) - 36.0f, panelBounds.y + 70.0f, 72.0f, 72.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = badgeTex,
            .no_texture = false,
            .is_world_space = false
            });
    }

    float statsY = m_isVictory ? panelBounds.y + 160.0f : panelBounds.y + 90.0f;
    TextPayload statsText{
        .position = { panelBounds.x + (panelBounds.z * 0.5f), statsY },
        .color = { 0.9f, 0.93f, 0.98f, 1.0f },
        .scale = 18.0f,
        .showInCenter = true
    };
    std::snprintf(statsText.text_content, sizeof(statsText.text_content),
        "STUDENT: %s\nFINAL SCORE: %d | HIGH: %d\n\nPRESS 'Y' TO RESTART",
        m_playerName.c_str(), m_finalScore, m_highestScore);
    writeBuffer.push_command(830, 0, statsText);

    float btnX = panelBounds.x + 40.0f;
    float btnW = panelBounds.z - 80.0f;
    float startY = panelBounds.y + 285.0f;

    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_SaveRecord, { btnX, startY, btnW, 45.0f }, "SAVE RECORD", 18.0f)) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
        return;
    }

    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_MainMenu, { btnX, startY + 58.0f, btnW, 45.0f }, "MAIN MENU", 18.0f)) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
    }
}