#include "BenchEntity.hpp"

BenchEntity::BenchEntity(const glm::vec2& pos,
    const glm::vec2& entitySize,
    const glm::vec2& renderSize,
    const glm::vec2& visualOffset,
    const glm::vec4& color,
    int32_t renderDepth)
    : m_renderSize(renderSize), m_visualOffset(visualOffset), m_color(color), m_renderDepth(renderDepth) {
    position = prevPosition = pos;
    size = entitySize; // 64x64 grid collision box
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
    // Draws native 64x48 uncompressed sprite vertically centered in the lane (+8px Y offset)
    animator.draw(
        writeBuffer,
        ctx,
        renderPos + m_visualOffset,
        m_renderSize,
        m_color,
        m_renderDepth,
        true
    );
}