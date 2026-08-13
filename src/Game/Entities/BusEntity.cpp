#include "BusEntity.hpp"

BusEntity::BusEntity(const glm::vec2& pos, float speed, int dir,
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
    layer = CollisionLayer::Layer_Enemy;
    mask = CollisionLayer::Layer_Player;

    m_minX = -size.x - offscreenPadding;
    m_maxX = worldWidth + offscreenPadding;

    animator.flipX = (dir < 0);
}

void BusEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;

    // 1. Load the bus spritesheet texture from assets
    TextureHandle busSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Bus/shortBus-sheet.png");

    // 2. Register driving animation clip (adjust atlasDimensions/frames to match your sheet layout)
    animator.addAnimation("drive", AnimationClip{
        .texture = busSheet,
        .atlasDimensions = { 4, 1 }, // {Columns, Rows}
        .startFrame = 0,
        .endFrame = 3,
        .frameDuration = 0.1f,
        .loop = true
        });

    animator.play("drive");
}

void BusEntity::onUpdate(float dt, EngineContext* ctx) {
    // 1. Align flip orientation with direction of velocity
    if (velocity.x != 0.0f) {
        animator.flipX = (velocity.x < 0.0f);
        animator.play("drive");
    }

    // 2. Screen wrapping logic
    if (velocity.x > 0.0f && position.x > m_maxX) {
        position.x = m_minX;
        prevPosition.x = m_minX;
    }
    if (velocity.x < 0.0f && position.x < m_minX) {
        position.x = m_maxX;
        prevPosition.x = m_maxX;
    }
}

void BusEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    // Render current animated frame using Animator2D
    animator.draw(
        writeBuffer,
        ctx,
        { renderPos.x, renderPos.y + m_visualYOffset },
        size,
        m_color,
        m_renderDepth,
        true // World space coordinates
    );
}