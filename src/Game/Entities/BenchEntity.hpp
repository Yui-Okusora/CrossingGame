#pragma once
#include <Engine/Engine.hpp>

class BenchEntity : public Entity2D {
private:
    glm::vec2 m_visualOffset{ 4.0f, 12.0f };
    glm::vec2 m_visualMargin{ 8.0f, 24.0f };
    glm::vec4 m_color{ 0.55f, 0.55f, 0.58f, 1.0f };
    int32_t m_renderDepth = 30;

public:
    BenchEntity(const glm::vec2& pos,
        const glm::vec2& entitySize = { 64.0f, 64.0f },
        const glm::vec2& visualOffset = { 4.0f, 12.0f },
        const glm::vec2& visualMargin = { 8.0f, 24.0f },
        const glm::vec4& color = { 0.55f, 0.55f, 0.58f, 1.0f },
        int32_t renderDepth = 30);

    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};