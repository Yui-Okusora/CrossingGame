#pragma once
#include <Engine/Engine.hpp>

class BenchEntity : public Entity2D {
private:
    glm::vec2 m_renderSize{ 64.0f, 48.0f };   // Native 64x48 texture dimensions
    glm::vec2 m_visualOffset{ 0.0f, 8.0f };   // Vertically centers 48px sprite in 64px lane: (64 - 48) / 2 = 8px
    glm::vec4 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    int32_t m_renderDepth = 30;

public:
    BenchEntity(const glm::vec2& pos,
        const glm::vec2& entitySize = { 64.0f, 64.0f },
        const glm::vec2& renderSize = { 64.0f, 48.0f },
        const glm::vec2& visualOffset = { 0.0f, 8.0f },
        const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
        int32_t renderDepth = 30);

    void onAttach(EngineContext* ctx) override;
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;
};