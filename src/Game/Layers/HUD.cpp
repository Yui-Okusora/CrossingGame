#include "HUD.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>

void HUDLayer::onAttach(EngineContext* ctx) {
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) {
        m_playerName = *nameOpt;
    }
}

void HUDLayer::update(double dt, EngineContext* ctx) {
    if (auto scoreOpt = ctx->blackboard.get<int>("currentScore")) m_currentScore = *scoreOpt;
    if (auto highScoreOpt = ctx->blackboard.get<int>("highestScore")) m_highestScore = *highScoreOpt;
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) m_currentLevel = *levelOpt;
    if (auto maxLvlOpt = ctx->blackboard.get<int>("maxLevel")) m_maxLevel = *maxLvlOpt;
    if (auto timeOpt = ctx->blackboard.get<float>("elapsedTime")) m_elapsedTime = *timeOpt;
    if (auto modeOpt = ctx->blackboard.get<int>("gameMode")) m_mode = static_cast<GameMode>(*modeOpt);

    if (auto shieldOpt = ctx->blackboard.get<bool>("buffShield")) m_buffShieldActive = *shieldOpt;
    if (auto speedOpt = ctx->blackboard.get<float>("buffSpeedTimer")) m_buffSpeedTimer = *speedOpt;
    if (auto gpaOpt = ctx->blackboard.get<float>("buffGpaTimer")) m_buffGpaTimer = *gpaOpt;
    if (auto invOpt = ctx->blackboard.get<float>("buffInvincibleTimer")) m_buffInvincibleTimer = *invOpt;
}

void HUDLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextureHandle idCardTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/GameHUD/NewUI.png");
    TextureHandle pointMeter = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/GameHUD/PointMeter/PointMeter.png");
    TextureHandle buffIconTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/GameHUD/iconBuff.png");

    int minutes = static_cast<int>(m_elapsedTime) / 60;
    float seconds = std::fmod(m_elapsedTime, 60.0f);

    // ========================================================================
    // 1. STUDENT ID CARD (Positioned on the LEFT, Scaled up)
    // ========================================================================
    float idX = 18.0f;
    float idY = 8.0f;
    float idW = 260.0f;
    float idH = 138.0f;

    if (m_mode == GameMode::Campaign) {
        writeBuffer.push_command(515, 0, RectPayload{
            .dest_rect = { idX, idY, idW, idH },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = idCardTex,
            .no_texture = false,
            .is_world_space = false
            });

        uint32_t meterFrame = std::clamp<uint32_t>(
            static_cast<uint32_t>((m_currentLevel * 10) / (std::max)(1, m_maxLevel)),
            0, 10
        );

        writeBuffer.push_command(520, 0, RectPayload{
            .dest_rect = { idX + 150.0f, idY + 60.0f, 92.0f, 22.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = pointMeter,
            .atlas_dimensions = { 11, 1 },
            .atlas_pos = { meterFrame, 0 },
            .no_texture = false,
            .is_world_space = false
            });

        TextPayload expTimeText{
            .position = { idX + 170.0f, idY + 104.0f },
            .color = { 0.08f, 0.10f, 0.14f, 1.0f },
            .scale = 16.0f,
            .showInCenter = false
        };
        std::snprintf(expTimeText.text_content, sizeof(expTimeText.text_content), "%02d:%04.1f", minutes, seconds);
        writeBuffer.push_command(530, 0, expTimeText);
    }

    // ========================================================================
    // 2. TOP TELEMETRY RIBBON
    // ========================================================================
    float ribbonX = (m_mode == GameMode::Campaign) ? (idX + idW + 16.0f) : 0.0f;
    float ribbonW = (m_mode == GameMode::Campaign) ? (1200.0f - ribbonX) : 1200.0f;

    writeBuffer.push_command(500, 0, RectPayload{
        .dest_rect = { ribbonX, 0.0f, ribbonW, 54.0f },
        .color = { 0.06f, 0.08f, 0.10f, 0.95f },
        .no_texture = true,
        .is_world_space = false
        });

    TextPayload scoreText{
        .position = { ribbonX + 24.0f, 34.0f },
        .color = { 1.0f, 0.9f, 0.2f, 1.0f },
        .scale = 20.0f,
        .showInCenter = false
    };
    std::snprintf(scoreText.text_content, sizeof(scoreText.text_content), "SCORE: %d (HIGH: %d)", m_currentScore, m_highestScore);
    writeBuffer.push_command(510, 0, scoreText);

    if (m_mode == GameMode::Endless) {
        TextPayload levelText{
            .position = { 600.0f, 34.0f },
            .color = { 0.3f, 0.85f, 1.0f, 1.0f },
            .scale = 22.0f,
            .showInCenter = true
        };
        std::snprintf(levelText.text_content, sizeof(levelText.text_content), "ENDLESS MODE");
        writeBuffer.push_command(510, 0, levelText);

        TextPayload timeText{
            .position = { 1080.0f, 34.0f },
            .color = { 0.2f, 1.0f, 0.4f, 1.0f },
            .scale = 22.0f,
            .showInCenter = true
        };
        std::snprintf(timeText.text_content, sizeof(timeText.text_content), "TIME: %02d:%04.1f", minutes, seconds);
        writeBuffer.push_command(510, 0, timeText);
    }

    // ========================================================================
    // 3. VERTICAL DROPDOWN STATUS BUFF BADGES (Under ID card)
    // ========================================================================
    float badgeX = 18.0f;
    float currentBadgeY = (m_mode == GameMode::Campaign) ? (idY + idH + 12.0f) : 64.0f;
    float badgeH = 30.0f;
    float badgeGap = 8.0f;

    auto renderBadge = [&](glm::vec4 boxColor, glm::vec4 borderColor, glm::vec4 iconColor, const char* textStr) {
        float boxW = 140.0f;
        float boxX = badgeX + 32.0f;
        float centerX = boxX + (boxW * 0.5f);
        float centerY = currentBadgeY + (badgeH * 0.5f) + 5.0f;

        writeBuffer.push_command(520, 0, RectPayload{
            .dest_rect = { boxX, currentBadgeY, boxW, badgeH },
            .color = boxColor,
            .no_texture = true,
            .is_world_space = false
            });

        writeBuffer.push_command(525, 0, LinePayload{ {boxX, currentBadgeY}, {boxX + boxW, currentBadgeY}, borderColor, 1.5f });
        writeBuffer.push_command(525, 0, LinePayload{ {boxX, currentBadgeY}, {boxX, currentBadgeY + badgeH}, borderColor, 1.5f });
        writeBuffer.push_command(525, 0, LinePayload{ {boxX + boxW, currentBadgeY}, {boxX + boxW, currentBadgeY + badgeH}, borderColor, 1.5f });
        writeBuffer.push_command(525, 0, LinePayload{ {boxX, currentBadgeY + badgeH}, {boxX + boxW, currentBadgeY + badgeH}, borderColor, 1.5f });

        writeBuffer.push_command(530, 0, RectPayload{
            .dest_rect = { badgeX, currentBadgeY, 30.0f, 30.0f },
            .color = iconColor,
            .texture = buffIconTex,
            .no_texture = false,
            .is_world_space = false
            });

        TextPayload shadowTxt{
            .position = { centerX + 1.0f, centerY + 1.0f },
            .color = { 0.05f, 0.05f, 0.05f, 0.9f },
            .scale = 15.0f,
            .showInCenter = true
        };
        std::snprintf(shadowTxt.text_content, sizeof(shadowTxt.text_content), "%s", textStr);
        writeBuffer.push_command(545, 0, shadowTxt);

        TextPayload frontTxt{
            .position = { centerX, centerY },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = 15.0f,
            .showInCenter = true
        };
        std::snprintf(frontTxt.text_content, sizeof(frontTxt.text_content), "%s", textStr);
        writeBuffer.push_command(550, 0, frontTxt);

        currentBadgeY += badgeH + badgeGap;
        };

    if (m_buffInvincibleTimer > 0.0f) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "INVINCIBLE: %.1fs", m_buffInvincibleTimer);
        renderBadge({ 0.08f, 0.50f, 0.70f, 0.95f }, { 0.3f, 0.85f, 1.0f, 1.0f }, { 0.2f, 0.9f, 1.0f, 1.0f }, buf);
    }
    else if (m_buffShieldActive) {
        renderBadge({ 0.70f, 0.55f, 0.08f, 0.95f }, { 1.0f, 0.85f, 0.2f, 1.0f }, { 1.0f, 0.85f, 0.1f, 1.0f }, "[SHIELD: READY]");
    }

    if (m_buffSpeedTimer > 0.0f) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "SPEED: %.1fs", m_buffSpeedTimer);
        renderBadge({ 0.12f, 0.55f, 0.22f, 0.95f }, { 0.3f, 0.95f, 0.4f, 1.0f }, { 0.2f, 0.85f, 0.3f, 1.0f }, buf);
    }

    if (m_buffGpaTimer > 0.0f) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "GPA 2X: %.1fs", m_buffGpaTimer);
        renderBadge({ 0.55f, 0.15f, 0.65f, 0.95f }, { 0.9f, 0.35f, 1.0f, 1.0f }, { 0.85f, 0.2f, 0.95f, 1.0f }, buf);
    }
}