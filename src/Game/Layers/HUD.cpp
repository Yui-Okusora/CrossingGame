#include "HUD.hpp"
#include <cstdio>

void HUDLayer::onAttach(EngineContext* ctx) {
    // Read baseline blackboard metrics on layer initialization
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) {
        m_playerName = *nameOpt;
    }
}

void HUDLayer::onDetach(EngineContext* ctx) {
    // No dynamic memory to release
}

void HUDLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    // HUD is purely visual; pass events through
}

void HUDLayer::update(double dt, EngineContext* ctx) {
    // Pull active telemetry from Blackboard every frame
    if (auto scoreOpt = ctx->blackboard.get<int>("currentScore")) {
        m_currentScore = *scoreOpt;
    }
    if (auto highScoreOpt = ctx->blackboard.get<int>("highestScore")) {
        m_highestScore = *highScoreOpt;
    }
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) {
        m_currentLevel = *levelOpt;
    }
}

void HUDLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // Top HUD Bar Background (Dark Semi-Transparent Ribbon)
    writeBuffer.push_command(500, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 50.0f },
        .color = { 0.05f, 0.07f, 0.09f, 0.75f },
        .no_texture = true,
        .is_world_space = false // Screen space fixed UI
        });

    // 1. Left Telemetry: Score & High Score
    TextPayload scoreText;
    scoreText.color = { 1.0f, 0.9f, 0.2f, 1.0f }; // Gold accent
    scoreText.scale = 22.0f;
    scoreText.position = { 20.0f, 32.0f };
    scoreText.showInCenter = false;
    std::snprintf(scoreText.text_content, sizeof(scoreText.text_content),
        "SCORE: %d | HIGH: %d", m_currentScore, m_highestScore);
    writeBuffer.push_command(510, 0, scoreText);

    // 2. Center Telemetry: Current Level Phase
    TextPayload levelText;
    levelText.color = { 0.3f, 0.85f, 1.0f, 1.0f }; // Cyan accent
    levelText.scale = 24.0f;
    levelText.position = { 600.0f, 32.0f };
    levelText.showInCenter = true;
    std::snprintf(levelText.text_content, sizeof(levelText.text_content),
        "LEVEL %d", m_currentLevel);
    writeBuffer.push_command(510, 0, levelText);

    // 3. Right Telemetry: Student Profile Identity
    TextPayload playerText;
    playerText.color = { 0.9f, 0.95f, 1.0f, 1.0f }; // Off-white accent
    playerText.scale = 20.0f;
    playerText.position = { 1180.0f, 32.0f };
    playerText.showInCenter = false;

    // Render right-aligned text by accounting for string length
    std::snprintf(playerText.text_content, sizeof(playerText.text_content),
        "STUDENT: %s", m_playerName.c_str());
    writeBuffer.push_command(510, 0, playerText);
}