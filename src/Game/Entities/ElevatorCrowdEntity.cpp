#include "ElevatorCrowdEntity.hpp"
#include <gl2d/gl2d.h>
#include <algorithm>

ElevatorCrowdEntity::ElevatorCrowdEntity(const glm::vec2& pos, float speed, int dir,
    const glm::vec2& entitySize, float surgeMultiplier,
    float visualYOffset, int32_t renderDepth)
    : m_baseSpeed(speed), m_direction(dir), m_surgeMultiplier(surgeMultiplier),
    m_visualYOffset(visualYOffset), m_renderDepth(renderDepth) {
    size = entitySize;
    layer = CollisionLayer::Layer_Enemy;
    mask = CollisionLayer::Layer_Player;

    // Start completely hidden inside the origin elevator door
    position = prevPosition = glm::vec2((m_direction > 0) ? (m_doorLeft - size.x) : m_doorRight, pos.y);
    velocity = glm::vec2(0.0f);
    m_state = CrowdState::Idle;
}

void ElevatorCrowdEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    m_crowdTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/WaitingLine.png");
}

void ElevatorCrowdEntity::setSignal(SignalState signal) {
    currentSignal = signal;
    if (currentSignal == SignalState::Green && m_state == CrowdState::Idle) {
        m_state = CrowdState::Rushing;
        velocity.x = m_baseSpeed * m_surgeMultiplier * static_cast<float>(m_direction);
    }
    else if (currentSignal == SignalState::Red) {
        m_state = CrowdState::Idle;
        velocity = glm::vec2(0.0f);
        position.x = prevPosition.x = (m_direction > 0) ? (m_doorLeft - size.x) : m_doorRight;
    }
}

void ElevatorCrowdEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_state == CrowdState::Rushing) {
        if (m_direction > 0 && position.x >= m_doorRight) {
            m_state = CrowdState::Finished;
            velocity = glm::vec2(0.0f);
            position.x = prevPosition.x = m_doorRight;
        }
        else if (m_direction < 0 && position.x + size.x <= m_doorLeft) {
            m_state = CrowdState::Finished;
            velocity = glm::vec2(0.0f);
            position.x = prevPosition.x = m_doorLeft - size.x;
        }
    }
}

void ElevatorCrowdEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    if (m_state != CrowdState::Rushing) return;

    float unclippedLeft = renderPos.x;
    float unclippedRight = renderPos.x + size.x;

    // Clip rendering strictly between the left and right door centers (32px to 1168px)
    float clippedLeft = (std::max)(unclippedLeft, m_doorLeft);
    float clippedRight = (std::min)(unclippedRight, m_doorRight);
    float clippedWidth = clippedRight - clippedLeft;

    if (clippedWidth <= 0.0f) return;

    // Normalized horizontal sub-range [t0, t1] within [0, 1]
    float t0 = (clippedLeft - unclippedLeft) / size.x;
    float t1 = (clippedRight - unclippedLeft) / size.x;

    // WaitingLine.png faces left by default.
    // Moving Right (m_direction > 0): invert UVs so characters face right
    // Moving Left (m_direction < 0): default UVs so characters face left
    bool flipX = (m_direction > 0);

    float u0 = flipX ? (1.0f - t0) : t0;
    float u1 = flipX ? (1.0f - t1) : t1;

    // Acquire native texture atlas vertical bounds
    gl2d::TextureAtlas atlas(1, 1);
    glm::vec4 baseUV = atlas.get(0, 0, false);

    writeBuffer.push_command(m_renderDepth, 0, RectPayload{
        .dest_rect = { clippedLeft, renderPos.y + m_visualYOffset, clippedWidth, size.y },
        .color = glm::vec4(1.0f),
        .texture = m_crowdTex,
        .custom_uv = { u0, baseUV.y, u1, baseUV.w },
        .flip_x = false,
        .no_texture = false,
        .is_world_space = true,
        .use_custom_uv = true
        });
}