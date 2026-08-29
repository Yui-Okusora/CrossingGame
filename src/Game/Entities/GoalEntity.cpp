#include "GoalEntity.hpp"

GoalEntity::GoalEntity(const glm::vec2& pos,
    const glm::vec2& entitySize,
    const glm::vec4& bannerColor,
    int32_t renderDepth)
    : m_color(bannerColor), m_renderDepth(renderDepth) {
    position = prevPosition = pos;
    size = entitySize;
    layer = CollisionLayer::Layer_TriggerVolume;
    mask = CollisionLayer::Layer_Player;
    isTrigger = true;
    isStatic = true;
}

void GoalEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    TextureHandle goalTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/LevelUpLine.png");
    animator.addAnimation("static", AnimationClip{ goalTex, { 1, 1 }, 0, 0, 1.0f, false });
    animator.play("static");
}

void GoalEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    animator.draw(writeBuffer, ctx, renderPos, size, glm::vec4(1.0f), m_renderDepth, true);
}