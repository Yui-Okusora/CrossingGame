#pragma once
#include <Engine/Engine.hpp>
#include "StatusEffect.hpp"

class GameplayLayer;

struct PlayerConfig {
    glm::vec2 hitboxSize{ 28.0f, 36.0f };
    glm::vec2 spriteRenderSize{ 64.0f, 64.0f };
    glm::vec2 spriteOffset{ 18.0f, 14.0f };
    float gridSize = 64.0f;
    float defaultLerpSpeed = 22.0f;
    float currentLerpSpeed = 22.0f;
    float minX = 0.0f;
    float maxX = 1152.0f;
    float maxY = 741.0f;
    int32_t renderDepth = 50;
};

class StudentPlayerEntity : public Entity2D {
private:
    GameplayLayer* m_gameplayLayer = nullptr;
    PlayerConfig m_config;
    StatusEffectComposite m_buffs;

    glm::vec2 m_targetPosition{ 0.0f, 0.0f };
    bool m_isMoving = false;
    bool m_hasStartedFirstMove = false;
    bool m_isDead = false;
    bool m_touchingLogThisFrame = false;

    glm::vec2 m_inputBuffer{ 0.0f, 0.0f };
    float m_bufferTimer = 0.0f;
    int m_scoreMultiplier = 1;

public:
    bool isOnWaterLane = false;
    glm::vec2 currentWaterVel{ 0.0f, 0.0f };

    StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config = PlayerConfig{});

    void onAttach(EngineContext* ctx) override;
    void applyBuff(TeacherBuffType buffType);

    void setSpeedModifier(float speed) noexcept { m_config.currentLerpSpeed = speed; }
    void setScoreMultiplier(int mult) noexcept { m_scoreMultiplier = mult; }
    [[nodiscard]] int getScoreMultiplier() const noexcept { return m_scoreMultiplier; }
    [[nodiscard]] bool hasStartedFirstMove() const noexcept { return m_hasStartedFirstMove; }
    [[nodiscard]] bool isDead() const noexcept { return m_isDead; }

    void onUpdate(float dt, EngineContext* ctx) override;
    void onCollision(const CollisionInfo& collision, EngineContext* ctx) override;
    void postPhysicsUpdate(float dt, EngineContext* ctx);
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;

    bool handleLethalDamage(EngineContext* ctx, const glm::vec2& rescuePos = { 0.0f, 0.0f });
};