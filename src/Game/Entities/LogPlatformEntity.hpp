#pragma once
#include <Engine/Engine.hpp>

class LogPlatformEntity : public Entity2D {
private:
    float m_minX = 0.0f;
    float m_maxX = 0.0f;
    float m_visualYOffset = 7.0f;
    glm::vec4 m_color{ 0.45f, 0.28f, 0.12f, 1.0f };
    int32_t m_renderDepth = 25;

public:
    LogPlatformEntity(const glm::vec2& pos, float speed, int dir,
        const glm::vec2& entitySize = { 170.0f, 50.0f },
        float worldWidth = 1200.0f,
        float offscreenPadding = 50.0f,
        float visualYOffset = 7.0f,
        const glm::vec4& color = { 0.45f, 0.28f, 0.12f, 1.0f },
        int32_t renderDepth = 25);

    void onUpdate(float dt, EngineContext* ctx) override;
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};