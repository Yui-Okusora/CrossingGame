#pragma once
#include <Engine/Engine.hpp>

class MovingHazardEntity : public Entity2D {
private:
    float m_minX = 0.0f;
    float m_maxX = 0.0f;
    float m_visualYOffset = 0.0f;
    glm::vec4 m_tint{ 1.0f, 1.0f, 1.0f, 1.0f };
    int32_t m_renderDepth = 40;

public:
    MovingHazardEntity(const glm::vec2& pos, float speed, int dir, const glm::vec2& entitySize,
        uint32_t colLayer, uint32_t colMask, bool trigger = false,
        float visualYOffset = 0.0f, const glm::vec4& tint = glm::vec4(1.0f),
        int32_t renderDepth = 40, float worldWidth = 1200.0f, float padding = 80.0f)
        : m_visualYOffset(visualYOffset), m_tint(tint), m_renderDepth(renderDepth) {
        position = prevPosition = pos;
        size = entitySize;
        velocity = glm::vec2(speed * static_cast<float>(dir), 0.0f);
        layer = colLayer;
        mask = colMask;
        isTrigger = trigger;

        m_minX = -size.x - padding;
        m_maxX = worldWidth + padding;

        animator.flipX = (dir < 0);
    }

    void onUpdate(float dt, EngineContext* ctx) override {
        if (velocity.x != 0.0f) {
            animator.flipX = (velocity.x < 0.0f);
        }

        if (velocity.x > 0.0f && position.x > m_maxX) {
            position.x = prevPosition.x = m_minX;
        }
        else if (velocity.x < 0.0f && position.x < m_minX) {
            position.x = prevPosition.x = m_maxX;
        }
    }

    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override {
        if (animator.getCurrentAnimationName().empty()) {
            writeBuffer.push_command(m_renderDepth, 0, RectPayload{
                .dest_rect = { renderPos.x, renderPos.y + m_visualYOffset, size.x, size.y },
                .color = m_tint,
                .no_texture = true,
                .is_world_space = true
                });
        }
        else {
            animator.draw(writeBuffer, ctx, { renderPos.x, renderPos.y + m_visualYOffset }, size, m_tint, m_renderDepth, true);
        }
    }
};