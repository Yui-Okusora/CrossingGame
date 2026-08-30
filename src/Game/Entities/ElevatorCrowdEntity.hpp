#pragma once
#include <Engine/Engine.hpp>
#include "../Layers/LaneData.hpp"

enum class CrowdState : uint8_t {
    Idle,       // Tucked inside elevator during Red/Yellow phase
    Rushing,    // Moving across hallway during Green phase
    Finished    // Reached destination elevator, waiting for next cycle
};

class ElevatorCrowdEntity : public Entity2D {
private:
    float m_baseSpeed = 0.0f;
    int m_direction = 1;
    float m_surgeMultiplier = 5.7f;
    float m_visualYOffset = 10.0f;

    // Exact door center coordinates
    float m_doorLeft = 32.0f;
    float m_doorRight = 1168.0f;

    CrowdState m_state = CrowdState::Idle;
    TextureHandle m_crowdTex{};

    int32_t m_renderDepth = 40;

public:
    SignalState currentSignal = SignalState::Red;

    ElevatorCrowdEntity(const glm::vec2& pos, float speed, int dir,
        const glm::vec2& entitySize = { 600.0f, 44.0f },
        float surgeMultiplier = 5.7f,
        float visualYOffset = 10.0f,
        int32_t renderDepth = 40);

    void setSignal(SignalState signal);
    void onAttach(EngineContext* ctx) override;
    void onUpdate(float dt, EngineContext* ctx) override;
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;

    [[nodiscard]] bool hasFinishedCrossing() const noexcept { return m_state == CrowdState::Finished; }
};