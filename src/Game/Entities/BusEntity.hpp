#pragma once
#include <Engine/Engine.hpp>

class BusEntity : public Entity2D {
private:
    float m_minX = 0.0f;
    float m_maxX = 0.0f;
    float m_visualYOffset = 8.0f;
    glm::vec4 m_color{ 0.85f, 0.2f, 0.2f, 1.0f };
    int32_t m_renderDepth = 40;

public:
    BusEntity(const glm::vec2& pos, float speed, int dir,
        const glm::vec2& entitySize = { 130.0f, 48.0f },
        float worldWidth = 1200.0f,
        float offscreenPadding = 50.0f,
        float visualYOffset = 8.0f,
        const glm::vec4& color = { 0.85f, 0.2f, 0.2f, 1.0f },
        int32_t renderDepth = 40);

    void onUpdate(float dt, EngineContext* ctx) override;
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};