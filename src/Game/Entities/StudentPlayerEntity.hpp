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
    float maxY = 741.0f;
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
    bool m_hasStartedFirstMove = false; // Tracks first player movement

    glm::vec2 m_inputBuffer{ 0.0f, 0.0f };
    float m_bufferTimer = 0.0f;
    static constexpr float INPUT_BUFFER_DURATION = 0.18f;

    bool m_touchingLogThisFrame = false;
    bool m_isDead = false;

    bool m_hasShield = false;
    float m_speedBoostTimer = 0.0f;
    float m_gpaMultiplierTimer = 0.0f;
    float m_invincibilityTimer = 0.0f;
    int m_scoreMultiplier = 1;
    static constexpr float SHIELD_INVINCIBILITY_DURATION = 2.0f;

public:
    bool isOnWaterLane = false;
    glm::vec2 currentWaterVel{ 0.0f, 0.0f };

    StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config = PlayerConfig{});

    void onAttach(EngineContext* ctx) override;
    [[nodiscard]] bool isMoving() const noexcept { return m_isMoving; }
    [[nodiscard]] bool hasStartedFirstMove() const noexcept { return m_hasStartedFirstMove; }
    [[nodiscard]] bool hasShield() const noexcept { return m_hasShield; }
    [[nodiscard]] bool isInvincible() const noexcept { return m_invincibilityTimer > 0.0f; }
    [[nodiscard]] int getScoreMultiplier() const noexcept { return m_scoreMultiplier; }

    void applyBuff(TeacherBuffType buffType);
    void onUpdate(float dt, EngineContext* ctx) override;
    void onCollision(const CollisionInfo& collision, EngineContext* ctx) override;
    void postPhysicsUpdate(float dt, EngineContext* ctx);
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};