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
    // 1. TOP TELEMETRY RIBBON
    // ========================================================================
    writeBuffer.push_command(500, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 54.0f },
        .color = { 0.06f, 0.08f, 0.10f, 0.95f },
        .no_texture = true,
        .is_world_space = false
        });

    // Score & High Score
    TextPayload scoreText{
        .position = { 20.0f, 34.0f },
        .color = { 1.0f, 0.9f, 0.2f, 1.0f },
        .scale = 20.0f,
        .showInCenter = false
    };
    std::snprintf(scoreText.text_content, sizeof(scoreText.text_content), "SCORE: %d (HIGH: %d)", m_currentScore, m_highestScore);
    writeBuffer.push_command(510, 0, scoreText);

    // Mode / Wave Phase Indicator
    TextPayload levelText{
        .position = { 460.0f, 34.0f },
        .color = { 0.3f, 0.85f, 1.0f, 1.0f },
        .scale = 22.0f,
        .showInCenter = true
    };
    if (m_mode == GameMode::Endless) {
        std::snprintf(levelText.text_content, sizeof(levelText.text_content), "ENDLESS (WAVE %d)", m_currentLevel);
    }
    else {
        std::snprintf(levelText.text_content, sizeof(levelText.text_content), "LEVEL %d / 5", m_currentLevel);
    }
    writeBuffer.push_command(510, 0, levelText);

    // Time Counter
    TextPayload timeText{
        .position = { 700.0f, 34.0f },
        .color = { 0.2f, 1.0f, 0.4f, 1.0f },
        .scale = 22.0f,
        .showInCenter = true
    };
    std::snprintf(timeText.text_content, sizeof(timeText.text_content), "TIME: %02d:%04.1f", minutes, seconds);
    writeBuffer.push_command(510, 0, timeText);

    // ========================================================================
    // 2. STUDENT ID CARD (NewUI.png + PointMeter.png UV Fix)
    // ========================================================================
    float idX = 960.0f;
    float idY = 4.0f;
    float idW = 220.0f;
    float idH = 116.0f;

    // Base ID Badge
    writeBuffer.push_command(520, 0, RectPayload{
        .dest_rect = { idX, idY, idW, idH },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = idCardTex,
        .no_texture = false,
        .is_world_space = false
        });

    // PointMeter: 7-Column Spritesheet { 7, 1 }
    uint32_t meterFrame = std::clamp<uint32_t>(static_cast<uint32_t>(m_currentScore / 250), 0, 6);
    writeBuffer.push_command(525, 0, RectPayload{
        .dest_rect = { idX + 128.0f, idY + 50.0f, 64.0f, 16.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = pointMeter,
        .atlas_dimensions = { 11, 1 },
        .atlas_pos = { meterFrame, 0 },
        .no_texture = false,
        .is_world_space = false
        });

    // Expired Timer string positioned on top of the underscore line
    TextPayload expTimeText{
        .position = { idX + 144.0f, idY + 86.0f },
        .color = { 0.08f, 0.10f, 0.14f, 1.0f },
        .scale = 14.0f,
        .showInCenter = false
    };
    std::snprintf(expTimeText.text_content, sizeof(expTimeText.text_content), "%02d:%04.1f", minutes, seconds);
    writeBuffer.push_command(530, 0, expTimeText);

    // ========================================================================
    // 3. ACTIVE BUFF STATUS BADGES
    // ========================================================================
    float badgeX = 20.0f, badgeY = 62.0f, badgeHeight = 28.0f, badgeGap = 8.0f;

    if (m_buffInvincibleTimer > 0.0f) {
        writeBuffer.push_command(540, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, 28.0f, 28.0f },
            .color = { 0.2f, 0.9f, 1.0f, 1.0f },
            .texture = buffIconTex,
            .no_texture = false,
            .is_world_space = false
            });
        writeBuffer.push_command(545, 0, RectPayload{
            .dest_rect = { badgeX + 32.0f, badgeY, 130.0f, badgeHeight },
            .color = { 0.1f, 0.75f, 0.9f, 0.9f },
            .no_texture = true,
            .is_world_space = false
            });
        TextPayload txt{
            .position = { badgeX + 97.0f, badgeY + 19.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = 14.0f,
            .showInCenter = true
        };
        std::snprintf(txt.text_content, sizeof(txt.text_content), "INVINCIBLE: %.1fs", m_buffInvincibleTimer);
        writeBuffer.push_command(550, 0, txt);
        badgeX += 170.0f + badgeGap;
    }
    else if (m_buffShieldActive) {
        writeBuffer.push_command(540, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, 28.0f, 28.0f },
            .color = { 1.0f, 0.85f, 0.1f, 1.0f },
            .texture = buffIconTex,
            .no_texture = false,
            .is_world_space = false
            });
        writeBuffer.push_command(545, 0, RectPayload{
            .dest_rect = { badgeX + 32.0f, badgeY, 120.0f, badgeHeight },
            .color = { 0.8f, 0.65f, 0.1f, 0.9f },
            .no_texture = true,
            .is_world_space = false
            });
        TextPayload txt{
            .position = { badgeX + 92.0f, badgeY + 19.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = 14.0f,
            .showInCenter = true
        };
        std::snprintf(txt.text_content, sizeof(txt.text_content), "[SHIELD: READY]");
        writeBuffer.push_command(550, 0, txt);
        badgeX += 160.0f + badgeGap;
    }

    if (m_buffSpeedTimer > 0.0f) {
        writeBuffer.push_command(540, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, 28.0f, 28.0f },
            .color = { 0.2f, 0.85f, 0.3f, 1.0f },
            .texture = buffIconTex,
            .no_texture = false,
            .is_world_space = false
            });
        writeBuffer.push_command(545, 0, RectPayload{
            .dest_rect = { badgeX + 32.0f, badgeY, 115.0f, badgeHeight },
            .color = { 0.15f, 0.7f, 0.25f, 0.9f },
            .no_texture = true,
            .is_world_space = false
            });
        TextPayload txt{
            .position = { badgeX + 89.0f, badgeY + 19.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = 14.0f,
            .showInCenter = true
        };
        std::snprintf(txt.text_content, sizeof(txt.text_content), "SPEED: %.1fs", m_buffSpeedTimer);
        writeBuffer.push_command(550, 0, txt);
        badgeX += 155.0f + badgeGap;
    }

    if (m_buffGpaTimer > 0.0f) {
        writeBuffer.push_command(540, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, 28.0f, 28.0f },
            .color = { 0.85f, 0.2f, 0.95f, 1.0f },
            .texture = buffIconTex,
            .no_texture = false,
            .is_world_space = false
            });
        writeBuffer.push_command(545, 0, RectPayload{
            .dest_rect = { badgeX + 32.0f, badgeY, 115.0f, badgeHeight },
            .color = { 0.65f, 0.15f, 0.75f, 0.9f },
            .no_texture = true,
            .is_world_space = false
            });
        TextPayload txt{
            .position = { badgeX + 89.0f, badgeY + 19.0f },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = 14.0f,
            .showInCenter = true
        };
        std::snprintf(txt.text_content, sizeof(txt.text_content), "GPA 2X: %.1fs", m_buffGpaTimer);
        writeBuffer.push_command(550, 0, txt);
    }
}