#include "BenchEntity.hpp"

BenchEntity::BenchEntity(const glm::vec2& pos,
    const glm::vec2& entitySize,
    const glm::vec2& visualOffset,
    const glm::vec2& visualMargin,
    const glm::vec4& color,
    int32_t renderDepth)
    : m_visualOffset(visualOffset), m_visualMargin(visualMargin), m_color(color), m_renderDepth(renderDepth) {
    position = pos;
    prevPosition = pos;
    size = entitySize;
    layer = CollisionLayer::Layer_Obstacle;
    mask = CollisionLayer::Layer_Player;
    isStatic = true;
}

void BenchEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    writeBuffer.push_command(m_renderDepth, 0, RectPayload{
        .dest_rect = {
            renderPos.x + m_visualOffset.x,
            renderPos.y + m_visualOffset.y,
            size.x - m_visualMargin.x,
            size.y - m_visualMargin.y
        },
        .color = m_color,
        .no_texture = true,
        .is_world_space = true
        });
}