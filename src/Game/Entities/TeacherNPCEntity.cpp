#include "TeacherNPCEntity.hpp"
#include "StudentPlayerEntity.hpp"
#include <iostream>
#include <cmath>

TeacherNPCEntity::TeacherNPCEntity(const glm::vec2& pos, const glm::vec2& patrolB, TeacherBuffType buff)
    : m_buffType(buff) {
    position = pos;
    prevPosition = pos;
    m_patrolA = pos;
    m_patrolB = patrolB;
    size = glm::vec2(44.0f, 44.0f);

    switch (m_buffType) {
    case TeacherBuffType::SpeedBoost:     m_color = glm::vec4{ 0.2f, 0.85f, 0.3f, 1.0f }; break;
    case TeacherBuffType::DeadlineShield: m_color = glm::vec4{ 0.9f, 0.75f, 0.1f, 1.0f }; break;
    case TeacherBuffType::GpaMultiplier:  m_color = glm::vec4{ 0.85f, 0.2f, 0.95f, 1.0f }; break;
    }

    layer = CollisionLayer::Layer_TriggerVolume;
    mask = CollisionLayer::Layer_Player;
    isTrigger = true;
    isStatic = false;
}

void TeacherNPCEntity::onAttach(EngineContext* ctx) {
    if (ctx) {
        m_buffSFX = ctx->audioEngine.loadSound(SFX_PATH "buff_pickup.wav");
    }
}

void TeacherNPCEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_isBuffGiven) return;

    glm::vec2 target = m_movingToB ? m_patrolB : m_patrolA;
    glm::vec2 dir = target - position;
    float dist = glm::length(dir);

    if (dist < 4.0f) {
        position = target;
        m_movingToB = !m_movingToB;
    }
    else {
        velocity = glm::normalize(dir) * m_moveSpeed;
    }
}

void TeacherNPCEntity::onCollision(const CollisionInfo& collision, EngineContext* ctx) {
    onTrigger(collision, ctx);
}

void TeacherNPCEntity::onTrigger(const CollisionInfo& trigger, EngineContext* ctx) {
    if (m_isBuffGiven) return;

    if ((trigger.targetLayer & CollisionLayer::Layer_Player) != 0) {
        m_isBuffGiven = true;
        active = false; // Disappear from arena upon pickup

        // --- FIX: Apply buff to player on collision ---
        if (m_player) {
            m_player->applyBuff(m_buffType);
        }

        if (ctx) {
            ctx->audioEngine.play(m_buffSFX, AudioCategory::GameplaySFX);
        }

        std::cout << "[TeacherNPC] Granted Buff to Player: " << static_cast<int>(m_buffType) << "\n";
    }
}

void TeacherNPCEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    if (m_isBuffGiven) return;

    writeBuffer.push_command(m_renderDepth, 0, RectPayload{
        .dest_rect = { renderPos.x + 10.0f, renderPos.y + 10.0f, size.x, size.y },
        .color = m_color,
        .no_texture = true,
        .is_world_space = true
        });
}