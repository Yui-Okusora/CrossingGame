#include "HUD.hpp"
#include <cstdio>

void HUDLayer::onAttach(EngineContext* ctx) {
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) {
        m_playerName = *nameOpt;
    }
}

void HUDLayer::onDetach(EngineContext* ctx) {}

void HUDLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}

void HUDLayer::update(double dt, EngineContext* ctx) {
    if (auto scoreOpt = ctx->blackboard.get<int>("currentScore")) {
        m_currentScore = *scoreOpt;
    }
    if (auto highScoreOpt = ctx->blackboard.get<int>("highestScore")) {
        m_highestScore = *highScoreOpt;
    }
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) {
        m_currentLevel = *levelOpt;
    }

    if (auto shieldOpt = ctx->blackboard.get<bool>("buffShield")) {
        m_buffShieldActive = *shieldOpt;
    }
    if (auto speedOpt = ctx->blackboard.get<float>("buffSpeedTimer")) {
        m_buffSpeedTimer = *speedOpt;
    }
    if (auto gpaOpt = ctx->blackboard.get<float>("buffGpaTimer")) {
        m_buffGpaTimer = *gpaOpt;
    }
    if (auto invOpt = ctx->blackboard.get<float>("buffInvincibleTimer")) {
        m_buffInvincibleTimer = *invOpt;
    }
}

void HUDLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Top HUD Ribbon Background
    writeBuffer.push_command(500, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 50.0f },
        .color = { 0.05f, 0.07f, 0.09f, 0.85f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Left Telemetry
    TextPayload scoreText;
    scoreText.color = { 1.0f, 0.9f, 0.2f, 1.0f };
    scoreText.scale = 22.0f;
    scoreText.position = { 20.0f, 32.0f };
    scoreText.showInCenter = false;
    std::snprintf(scoreText.text_content, sizeof(scoreText.text_content),
        "SCORE: %d | HIGH: %d", m_currentScore, m_highestScore);
    writeBuffer.push_command(510, 0, scoreText);

    // 3. Center Telemetry
    TextPayload levelText;
    levelText.color = { 0.3f, 0.85f, 1.0f, 1.0f };
    levelText.scale = 24.0f;
    levelText.position = { 600.0f, 32.0f };
    levelText.showInCenter = true;
    std::snprintf(levelText.text_content, sizeof(levelText.text_content),
        "LEVEL %d", m_currentLevel);
    writeBuffer.push_command(510, 0, levelText);

    // 4. Right Telemetry
    TextPayload playerText;
    playerText.color = { 0.9f, 0.95f, 1.0f, 1.0f };
    playerText.scale = 20.0f;
    playerText.position = { 1180.0f, 32.0f };
    playerText.showInCenter = false;
    std::snprintf(playerText.text_content, sizeof(playerText.text_content),
        "STUDENT: %s", m_playerName.c_str());
    writeBuffer.push_command(510, 0, playerText);

    // ============================================================================
    // ACTIVE BUFF STATUS BADGES DISPLAY (POSITIONED AT Y = 56.0f)
    // ============================================================================
    float badgeX = 20.0f;
    float badgeY = 56.0f;
    float badgeHeight = 32.0f;
    float badgeGap = 10.0f;

    // BADGE 1: ACTIVE INVINCIBILITY TIMER (TRIGGERED SHIELD)
    if (m_buffInvincibleTimer > 0.0f) {
        float badgeW = 180.0f;

        writeBuffer.push_command(520, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, badgeW, badgeHeight },
            .color = { 0.1f, 0.75f, 0.9f, 0.85f }, // Cyan background
            .no_texture = true,
            .is_world_space = false
            });

        TextPayload invText;
        invText.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        invText.scale = 16.0f;
        invText.position = { badgeX + (badgeW * 0.5f), badgeY + 22.0f };
        invText.showInCenter = true;
        std::snprintf(invText.text_content, sizeof(invText.text_content), "INVINCIBLE: %.1fs", m_buffInvincibleTimer);
        writeBuffer.push_command(530, 0, invText);

        badgeX += badgeW + badgeGap;
    }
    // BADGE 1 ALT: DEADLINE SHIELD READY
    else if (m_buffShieldActive) {
        float badgeW = 180.0f;

        writeBuffer.push_command(520, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, badgeW, badgeHeight },
            .color = { 0.8f, 0.65f, 0.1f, 0.85f }, // Gold background
            .no_texture = true,
            .is_world_space = false
            });

        TextPayload shieldText;
        shieldText.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        shieldText.scale = 16.0f;
        shieldText.position = { badgeX + (badgeW * 0.5f), badgeY + 22.0f };
        shieldText.showInCenter = true;
        std::snprintf(shieldText.text_content, sizeof(shieldText.text_content), "[SHIELD: READY]");
        writeBuffer.push_command(530, 0, shieldText);

        badgeX += badgeW + badgeGap;
    }

    // BADGE 2: SPEED BOOST
    if (m_buffSpeedTimer > 0.0f) {
        float badgeW = 180.0f;

        writeBuffer.push_command(520, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, badgeW, badgeHeight },
            .color = { 0.15f, 0.7f, 0.25f, 0.85f }, // Green background
            .no_texture = true,
            .is_world_space = false
            });

        TextPayload speedText;
        speedText.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        speedText.scale = 16.0f;
        speedText.position = { badgeX + (badgeW * 0.5f), badgeY + 22.0f };
        speedText.showInCenter = true;
        std::snprintf(speedText.text_content, sizeof(speedText.text_content), "SPEED: %.1fs", m_buffSpeedTimer);
        writeBuffer.push_command(530, 0, speedText);

        badgeX += badgeW + badgeGap;
    }

    // BADGE 3: 2X GPA MULTIPLIER
    if (m_buffGpaTimer > 0.0f) {
        float badgeW = 180.0f;

        writeBuffer.push_command(520, 0, RectPayload{
            .dest_rect = { badgeX, badgeY, badgeW, badgeHeight },
            .color = { 0.65f, 0.15f, 0.75f, 0.85f }, // Purple background
            .no_texture = true,
            .is_world_space = false
            });

        TextPayload gpaText;
        gpaText.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        gpaText.scale = 16.0f;
        gpaText.position = { badgeX + (badgeW * 0.5f), badgeY + 22.0f };
        gpaText.showInCenter = true;
        std::snprintf(gpaText.text_content, sizeof(gpaText.text_content), "GPA 2X: %.1fs", m_buffGpaTimer);
        writeBuffer.push_command(530, 0, gpaText);
    }
}