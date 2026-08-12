#pragma once
#include <Engine/Engine.hpp>
#include "TeacherNPCEntity.hpp"

class GameplayLayer;

struct PlayerConfig {
    glm::vec2 entitySize{ 44.0f, 44.0f };
    float gridSize = 64.0f;
    float lerpSpeed = 22.0f;
    float minX = 0.0f;
    float maxX = 1152.0f;
    float maxY = 700.0f;
    glm::vec2 visualOffset{ 10.0f, 10.0f };
    glm::vec4 color{ 0.0f, 0.85f, 1.0f, 1.0f };
    int32_t renderDepth = 50;
};

class StudentPlayerEntity : public Entity2D {
private:
    GameplayLayer* m_gameplayLayer = nullptr;
    PlayerConfig m_config;

    glm::vec2 m_targetPosition{ 0.0f, 0.0f };
    bool m_isMoving = false;

    // Input Buffering
    glm::vec2 m_inputBuffer{ 0.0f, 0.0f };
    float m_bufferTimer = 0.0f;
    static constexpr float INPUT_BUFFER_DURATION = 0.18f;

    bool m_touchingLogThisFrame = false;
    bool m_isDead = false;

    // --- BUFF SYSTEM TIMERS & CHARGES ---
    bool m_hasShield = false;            // 1-time hit protection shield
    float m_speedBoostTimer = 0.0f;      // Speed boost duration timer
    float m_gpaMultiplierTimer = 0.0f;   // 2X GPA multiplier duration timer
    float m_invincibilityTimer = 0.0f;   // Invincibility duration timer (granted on shield pop)
    int m_scoreMultiplier = 1;

    static constexpr float SHIELD_INVINCIBILITY_DURATION = 2.0f; // 2 seconds i-frames

public:
    bool isOnWaterLane = false;
    glm::vec2 currentWaterVel{ 0.0f, 0.0f };

    StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config = PlayerConfig{});

    [[nodiscard]] bool isMoving() const noexcept { return m_isMoving; }
    [[nodiscard]] bool hasShield() const noexcept { return m_hasShield; }
    [[nodiscard]] bool isInvincible() const noexcept { return m_invincibilityTimer > 0.0f; }
    [[nodiscard]] int getScoreMultiplier() const noexcept { return m_scoreMultiplier; }

    void applyBuff(TeacherBuffType buffType);

    void onUpdate(float dt, EngineContext* ctx) override;
    void onCollision(const CollisionInfo& collision, EngineContext* ctx) override;
    void postPhysicsUpdate(float dt, EngineContext* ctx);
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};