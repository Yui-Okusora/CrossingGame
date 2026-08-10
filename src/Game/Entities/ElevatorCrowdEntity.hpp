#pragma once
#include <Engine/Engine.hpp>
#include "../Layers/LaneData.hpp"

enum class CrowdState : uint8_t {
    Idle,       // Parked off-screen during Red/Yellow phase
    Rushing,    // Moving across the hallway during Green phase
    Finished    // Reached opposite off-screen side, waiting for light to reset to Red
};

class ElevatorCrowdEntity : public Entity2D {
private:
    float m_baseSpeed = 0.0f;
    int m_direction = 1;
    float m_surgeMultiplier = 5.7f;
    float m_visualYOffset = 10.0f;

    float m_worldWidth = 1200.0f;
    float m_minX = 0.0f;
    float m_maxX = 0.0f;

    CrowdState m_state = CrowdState::Idle;

    glm::vec4 m_colorYellow{ 0.9f, 0.7f, 0.2f, 0.6f };
    glm::vec4 m_colorGreen{ 0.95f, 0.35f, 0.1f, 1.0f };
    int32_t m_renderDepth = 40;

public:
    SignalState currentSignal = SignalState::Red;

    ElevatorCrowdEntity(const glm::vec2& pos, float speed, int dir,
        const glm::vec2& entitySize = { 600.0f, 44.0f },
        float surgeMultiplier = 5.7f,
        float worldWidth = 1200.0f,
        float offscreenPadding = 50.0f,
        float visualYOffset = 10.0f,
        const glm::vec4& colorYellow = { 0.9f, 0.7f, 0.2f, 0.6f },
        const glm::vec4& colorGreen = { 0.95f, 0.35f, 0.1f, 1.0f },
        int32_t renderDepth = 40);

    void setSignal(SignalState signal);
    void onUpdate(float dt, EngineContext* ctx) override;
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;

    [[nodiscard]] bool hasFinishedCrossing() const noexcept { return m_state == CrowdState::Finished; }
};