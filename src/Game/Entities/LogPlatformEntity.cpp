#include "LogPlatformEntity.hpp"

LogPlatformEntity::LogPlatformEntity(const glm::vec2& pos, float speed, int dir,
    const glm::vec2& entitySize,
    float worldWidth,
    float offscreenPadding,
    float visualYOffset,
    const glm::vec4& color,
    int32_t renderDepth)
    : m_visualYOffset(visualYOffset), m_color(color), m_renderDepth(renderDepth) {
    position = pos;
    prevPosition = pos;
    size = entitySize;
    velocity = glm::vec2(speed * static_cast<float>(dir), 0.0f);
    layer = CollisionLayer::Layer_TriggerVolume;
    mask = CollisionLayer::Layer_Player;
    isTrigger = true;

    m_minX = -size.x - offscreenPadding;
    m_maxX = worldWidth + offscreenPadding;
}

void LogPlatformEntity::onUpdate(float dt, EngineContext* ctx) {
    if (velocity.x > 0.0f && position.x > m_maxX) {
        position.x = m_minX;
        prevPosition.x = m_minX;
    }
    if (velocity.x < 0.0f && position.x < m_minX) {
        position.x = m_maxX;
        prevPosition.x = m_maxX;
    }
}

void LogPlatformEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    writeBuffer.push_command(m_renderDepth, 0, RectPayload{
        .dest_rect = { renderPos.x, renderPos.y + m_visualYOffset, size.x, size.y },
        .color = m_color,
        .no_texture = true,
        .is_world_space = true
        });
}