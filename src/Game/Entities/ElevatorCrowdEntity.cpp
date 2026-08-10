#include "ElevatorCrowdEntity.hpp"
#include <algorithm>

ElevatorCrowdEntity::ElevatorCrowdEntity(const glm::vec2& pos, float speed, int dir,
    const glm::vec2& entitySize,
    float surgeMultiplier,
    float worldWidth,
    float offscreenPadding,
    float visualYOffset,
    const glm::vec4& colorYellow,
    const glm::vec4& colorGreen,
    int32_t renderDepth)
    : m_baseSpeed(speed), m_direction(dir), m_surgeMultiplier(surgeMultiplier),
    m_visualYOffset(visualYOffset), m_colorYellow(colorYellow), m_colorGreen(colorGreen),
    m_renderDepth(renderDepth), m_worldWidth(worldWidth) {
    size = entitySize;
    layer = CollisionLayer::Layer_Enemy;
    mask = CollisionLayer::Layer_Player;

    // Auto-compute dynamic off-screen bounds
    m_minX = -size.x - offscreenPadding;
    m_maxX = worldWidth + offscreenPadding;

    // Initialize parked off-screen
    position.x = (m_direction > 0) ? m_minX : m_maxX;
    position.y = pos.y;
    prevPosition = position;
    velocity = glm::vec2(0.0f);
    m_state = CrowdState::Idle;
}

void ElevatorCrowdEntity::setSignal(SignalState signal) {
    currentSignal = signal;

    // 1. Transition to RUSHING when light turns Green (only from Idle)
    if (currentSignal == SignalState::Green && m_state == CrowdState::Idle) {
        m_state = CrowdState::Rushing;
        velocity.x = m_baseSpeed * m_surgeMultiplier * static_cast<float>(m_direction);
    }
    // 2. Reset to IDLE when light returns to Red
    else if (currentSignal == SignalState::Red) {
        m_state = CrowdState::Idle;
        velocity = glm::vec2(0.0f);
        position.x = (m_direction > 0) ? m_minX : m_maxX;
        prevPosition.x = position.x;
    }
}

void ElevatorCrowdEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_state == CrowdState::Rushing) {
        // Detect when crowd completely exits the opposite side of the screen
        if ((m_direction > 0 && position.x > m_maxX) ||
            (m_direction < 0 && position.x < m_minX)) {
            m_state = CrowdState::Finished;
            velocity = glm::vec2(0.0f);
            position.x = (m_direction > 0) ? m_minX : m_maxX;
            prevPosition.x = position.x;
        }
    }
}

void ElevatorCrowdEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    // Render ONLY while actively rushing across
    if (m_state != CrowdState::Rushing) return;

    glm::vec4 crowdColor = (currentSignal == SignalState::Yellow) ? m_colorYellow : m_colorGreen;

    // Horizontally clip rectangle to visible arena bounds [0.0f, m_worldWidth]
    float renderLeft = renderPos.x;
    float renderRight = renderPos.x + size.x;

    float clipLeft = std::clamp(renderLeft, 0.0f, m_worldWidth);
    float clipRight = std::clamp(renderRight, 0.0f, m_worldWidth);

    if (clipRight > clipLeft) {
        writeBuffer.push_command(m_renderDepth, 0, RectPayload{
            .dest_rect = { clipLeft, renderPos.y + m_visualYOffset, clipRight - clipLeft, size.y },
            .color = crowdColor,
            .no_texture = true,
            .is_world_space = true
            });
    }
}