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
    if (auto timeOpt = ctx->blackboard.get<float>("elapsedTime")) m_elapsedTime = *timeOpt;

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

    writeBuffer.push_command(810, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    glm::vec4 borderCol = m_isVictory ? glm::vec4{ 0.2f, 0.85f, 0.4f, 1.0f } : glm::vec4{ 0.85f, 0.25f, 0.25f, 1.0f };
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });

    // 1. Title Banner
    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 36.0f },
        .color = m_isVictory ? glm::vec4{ 0.2f, 0.95f, 0.35f, 1.0f } : glm::vec4{ 0.95f, 0.25f, 0.25f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), m_isVictory ? "CONGRATULATIONS!" : "GAME OVER");
    writeBuffer.push_command(820, 0, titleText);

    // 2. Degree Title & Badge Logo
    const char* degreeTitle = (m_finalScore > 2500) ? "DOCTOR OF PHILOSOPHY (PH.D)"
        : (m_finalScore > 1200) ? "MASTER OF SCIENCE"
        : "BACHELOR OF SCIENCE";

    const char* badgePath = (m_finalScore > 2500) ? RESOURCES_PATH "Sprite/Result screen/PhDBadge.png"
        : (m_finalScore > 1200) ? RESOURCES_PATH "Sprite/Result screen/MasterBadge.png"
        : RESOURCES_PATH "Sprite/Result screen/BachelorBadge.png";

    TextureHandle badgeTex = ctx->assetManager.loadTexture(badgePath);
    writeBuffer.push_command(825, 0, RectPayload{
        .dest_rect = { m_panelBounds.x + (m_panelBounds.z * 0.5f) - 34.0f, m_panelBounds.y + 60.0f, 68.0f, 68.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = badgeTex,
        .no_texture = false,
        .is_world_space = false
        });

    TextPayload degreeText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 145.0f },
        .color = { 1.0f, 0.85f, 0.2f, 1.0f },
        .scale = 16.0f,
        .showInCenter = true
    };
    std::snprintf(degreeText.text_content, sizeof(degreeText.text_content), "TITLE: %s", degreeTitle);
    writeBuffer.push_command(830, 0, degreeText);

    // 3. Stats Block (Student, Score, Time)
    int minutes = static_cast<int>(m_elapsedTime) / 60;
    float seconds = std::fmod(m_elapsedTime, 60.0f);

    TextPayload statsText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 195.0f },
        .color = { 0.9f, 0.93f, 0.98f, 1.0f },
        .scale = 16.0f,
        .showInCenter = true
    };
    std::snprintf(statsText.text_content, sizeof(statsText.text_content),
        "STUDENT: %s\nFINAL SCORE: %d | HIGH SCORE: %d\nTIME SURVIVED: %02d:%04.1f\n\nPRESS 'Y' TO RESTART",
        m_playerName.c_str(), m_finalScore, m_highestScore, minutes, seconds);
    writeBuffer.push_command(835, 0, statsText);

    // 4. Action Buttons
    float btnX = m_panelBounds.x + 40.0f;
    float btnW = m_panelBounds.z - 80.0f;
    float startY = m_panelBounds.y + 345.0f;

    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_SaveRecord, { btnX, startY, btnW, 44.0f }, "SAVE RECORD", 18.0f)) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
        return;
    }

    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_MainMenu, { btnX, startY + 54.0f, btnW, 44.0f }, "MAIN MENU", 18.0f)) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
    }
}