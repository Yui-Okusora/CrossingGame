#include "ElevatorCrowdEntity.hpp"
#include <algorithm>

ElevatorCrowdEntity::ElevatorCrowdEntity(const glm::vec2& pos, float speed, int dir,
    const glm::vec2& entitySize, float surgeMultiplier, float worldWidth,
    float offscreenPadding, float visualYOffset, const glm::vec4& colorYellow,
    const glm::vec4& colorGreen, int32_t renderDepth)
    : m_baseSpeed(speed), m_direction(dir), m_surgeMultiplier(surgeMultiplier),
    m_visualYOffset(visualYOffset), m_colorYellow(colorYellow), m_colorGreen(colorGreen),
    m_renderDepth(renderDepth), m_worldWidth(worldWidth) {
    size = entitySize;
    layer = CollisionLayer::Layer_Enemy;
    mask = CollisionLayer::Layer_Player;

    m_minX = -size.x - offscreenPadding;
    m_maxX = worldWidth + offscreenPadding;
    position = prevPosition = glm::vec2((m_direction > 0) ? m_minX : m_maxX, pos.y);
    velocity = glm::vec2(0.0f);
    m_state = CrowdState::Idle;
}

void ElevatorCrowdEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    TextureHandle crowdTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/WaitingLine.png");
    animator.addAnimation("rush", AnimationClip{ crowdTex, { 1, 1 }, 0, 0, 1.0f, false });
    animator.play("rush");
}

void ElevatorCrowdEntity::setSignal(SignalState signal) {
    currentSignal = signal;
    if (currentSignal == SignalState::Green && m_state == CrowdState::Idle) {
        m_state = CrowdState::Rushing;
        velocity.x = m_baseSpeed * m_surgeMultiplier * static_cast<float>(m_direction);
        animator.flipX = (m_direction < 0);
    }
    else if (currentSignal == SignalState::Red) {
        m_state = CrowdState::Idle;
        velocity = glm::vec2(0.0f);
        position.x = prevPosition.x = (m_direction > 0) ? m_minX : m_maxX;
    }
}

void ElevatorCrowdEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_state == CrowdState::Rushing) {
        if ((m_direction > 0 && position.x > m_maxX) || (m_direction < 0 && position.x < m_minX)) {
            m_state = CrowdState::Finished;
            velocity = glm::vec2(0.0f);
            position.x = prevPosition.x = (m_direction > 0) ? m_minX : m_maxX;
        }
    }
}

void ElevatorCrowdEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    if (m_state != CrowdState::Rushing) return;

    float renderLeft = renderPos.x;
    float renderRight = renderPos.x + size.x;
    float clipLeft = std::clamp(renderLeft, 0.0f, m_worldWidth);
    float clipRight = std::clamp(renderRight, 0.0f, m_worldWidth);

    if (clipRight > clipLeft) {
        animator.draw(writeBuffer, ctx, { clipLeft, renderPos.y + m_visualYOffset }, { clipRight - clipLeft, size.y }, glm::vec4(1.0f), m_renderDepth, true);
    }
}