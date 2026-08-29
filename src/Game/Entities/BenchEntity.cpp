#include "BenchEntity.hpp"

BenchEntity::BenchEntity(const glm::vec2& pos,
    const glm::vec2& entitySize,
    const glm::vec2& visualOffset,
    const glm::vec2& visualMargin,
    const glm::vec4& color,
    int32_t renderDepth)
    : m_visualOffset(visualOffset), m_visualMargin(visualMargin), m_color(color), m_renderDepth(renderDepth) {
    position = prevPosition = pos;
    size = entitySize;
    layer = CollisionLayer::Layer_Obstacle;
    mask = CollisionLayer::Layer_Player;
    isStatic = true;
}

void BenchEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    TextureHandle benchTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/Stone Bench.png");
    animator.addAnimation("static", AnimationClip{ benchTex, { 1, 1 }, 0, 0, 1.0f, false });
    animator.play("static");
}

void BenchEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    animator.draw(writeBuffer, ctx, renderPos + m_visualOffset, size - m_visualMargin, glm::vec4(1.0f), m_renderDepth, true);
}