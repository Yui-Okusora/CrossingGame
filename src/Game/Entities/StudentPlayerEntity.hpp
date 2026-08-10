#pragma once
#include <Engine/Engine.hpp>

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

    // Input Buffering (prevents dropped keypresses when spamming)
    glm::vec2 m_inputBuffer{ 0.0f, 0.0f };
    float m_bufferTimer = 0.0f;
    static constexpr float INPUT_BUFFER_DURATION = 0.18f; // 180ms queue window

    bool m_touchingLogThisFrame = false;
    bool m_isDead = false;

public:
    bool isOnWaterLane = false;
    glm::vec2 currentWaterVel{ 0.0f, 0.0f };

    StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config = PlayerConfig{});

    [[nodiscard]] bool isMoving() const noexcept { return m_isMoving; }

    void onUpdate(float dt, EngineContext* ctx) override;
    void onCollision(const CollisionInfo& collision, EngineContext* ctx) override;
    void postPhysicsUpdate(float dt, EngineContext* ctx);
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};