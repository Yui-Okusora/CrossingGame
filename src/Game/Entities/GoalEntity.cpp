#include "GoalEntity.hpp"

GoalEntity::GoalEntity(const glm::vec2& pos,
    const glm::vec2& entitySize,
    const glm::vec4& bannerColor,
    int32_t renderDepth)
    : m_color(bannerColor), m_renderDepth(renderDepth) {
    position = pos;
    prevPosition = pos;
    size = entitySize;
    layer = CollisionLayer::Layer_TriggerVolume;
    mask = CollisionLayer::Layer_Player;
    isTrigger = true;
    isStatic = true;
}

void GoalEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    writeBuffer.push_command(m_renderDepth, 0, RectPayload{
        .dest_rect = { renderPos.x, renderPos.y, size.x, size.y },
        .color = m_color,
        .no_texture = true,
        .is_world_space = true
        });
}