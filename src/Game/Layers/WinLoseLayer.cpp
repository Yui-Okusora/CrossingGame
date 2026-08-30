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
    if (auto modeOpt = ctx->blackboard.get<int>("gameMode")) m_mode = static_cast<GameMode>(*modeOpt);

    if (m_finalScore > m_highestScore) {
        m_highestScore = m_finalScore;
        ctx->blackboard.set("highestScore", m_highestScore);
    }

    if (ctx) {
        const char* resultSfxPath = m_isVictory ? SFX_PATH "game_win.wav" : SFX_PATH "game_over.wav";
        AudioHandle resultSFX = ctx->audioEngine.loadSound(resultSfxPath);
        ctx->audioEngine.play(resultSFX, AudioCategory::GameplaySFX);
    }
}

void WinLosePopupLayer::onDetach(EngineContext* ctx) {
    const char* resultSfxPath = m_isVictory ? SFX_PATH "game_win.wav" : SFX_PATH "game_over.wav";
    AudioHandle resultSFX = ctx->audioEngine.loadSound(resultSfxPath);
    ctx->audioEngine.stop(resultSFX);
}

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
    // 1. Semi-transparent Backdrop Dimming (Depth: 800)
    writeBuffer.push_command(800, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.78f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Modal Slate Panel (Depth: 810)
    writeBuffer.push_command(810, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    // Modal Border Lines (Depth: 815)
    glm::vec4 borderCol = m_isVictory ? glm::vec4{ 0.2f, 0.85f, 0.4f, 1.0f } : glm::vec4{ 0.85f, 0.25f, 0.25f, 1.0f };
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });

    // 3. Header Title Banner (Depth: 820)
    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 36.0f },
        .color = m_isVictory ? glm::vec4{ 0.2f, 0.95f, 0.35f, 1.0f } : glm::vec4{ 0.95f, 0.25f, 0.25f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), m_isVictory ? "CONGRATULATIONS!" : "GAME OVER");
    writeBuffer.push_command(820, 0, titleText);

    // 4. Resolve Academic Badge & Degree Title
    AcademicBadgeInfo badgeInfo = GetAcademicBadgeInfo(m_mode, m_currentLevel, m_finalScore);
    TextureHandle badgeTex = ctx->assetManager.loadTexture(badgeInfo.badgePath);

    if (badgeTex.id != 0) {
        writeBuffer.push_command(825, 0, RectPayload{
            .dest_rect = { m_panelBounds.x + (m_panelBounds.z * 0.5f) - 34.0f, m_panelBounds.y + 60.0f, 68.0f, 68.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = badgeTex,
            .no_texture = false,
            .is_world_space = false
            });
    }

    TextPayload degreeText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 145.0f },
        .color = badgeInfo.color,
        .scale = 16.0f,
        .showInCenter = true
    };
    std::snprintf(degreeText.text_content, sizeof(degreeText.text_content), "TITLE: %s", badgeInfo.title);
    writeBuffer.push_command(830, 0, degreeText);

    // 5. Performance Stats Block (Depth: 835)
    int minutes = static_cast<int>(m_elapsedTime) / 60;
    float seconds = std::fmod(m_elapsedTime, 60.0f);

    TextPayload statsText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 195.0f },
        .color = { 0.9f, 0.93f, 0.98f, 1.0f },
        .scale = 16.0f,
        .showInCenter = true
    };
    std::snprintf(statsText.text_content, sizeof(statsText.text_content),
        "STUDENT: %s\nFINAL SCORE: %d | HIGH SCORE: %d\nTIME: %02d:%04.1f (%s)\n\nPRESS 'Y' TO RESTART",
        m_playerName.c_str(), m_finalScore, m_highestScore, minutes, seconds,
        (m_mode == GameMode::Endless ? "ENDLESS" : "CAMPAIGN"));
    writeBuffer.push_command(835, 0, statsText);

    // 6. Action Buttons (Depth: 900)
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