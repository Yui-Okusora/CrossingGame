#pragma once
#include <Engine/Engine.hpp>

class GoalEntity : public Entity2D {
private:
    glm::vec4 m_color{ 0.9f, 0.75f, 0.1f, 0.85f };
    int32_t m_renderDepth = 20;

public:
    GoalEntity(const glm::vec2& pos,
        const glm::vec2& entitySize = { 1200.0f, 64.0f },
        const glm::vec4& bannerColor = { 0.9f, 0.75f, 0.1f, 0.85f },
        int32_t renderDepth = 20);

    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};