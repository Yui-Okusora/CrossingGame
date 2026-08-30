#include "TeacherNPCEntity.hpp"
#include "StudentPlayerEntity.hpp"

TeacherNPCEntity::TeacherNPCEntity(const glm::vec2& pos, const glm::vec2& patrolB, TeacherBuffType buff)
    : m_buffType(buff) {
    position = prevPosition = m_patrolA = pos;
    m_patrolB = patrolB;
    size = glm::vec2(44.0f, 44.0f);

    layer = CollisionLayer::Layer_TriggerVolume;
    mask = CollisionLayer::Layer_Player;
    isTrigger = true;
    isStatic = false;
}

void TeacherNPCEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    m_buffSFX = ctx->audioEngine.loadSound(SFX_PATH "buff_pickup.wav");

    const char* sheetPath = (m_buffType == TeacherBuffType::GpaMultiplier)
        ? RESOURCES_PATH "Sprite/Teacher/Teacher Tinh - animation.png"
        : RESOURCES_PATH "Sprite/Teacher/Teacher Quan - animation.png";

    TextureHandle teacherSheet = ctx->assetManager.loadTexture(sheetPath);

    // Row 2: Side Walk Animation (Frames 12 - 17)
    animator.addAnimation("patrol_side", AnimationClip{ teacherSheet, { 6, 4 }, 12, 17, 0.12f, true });
    animator.play("patrol_side");
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

    animator.flipX = (velocity.x < 0.0f);
}

void TeacherNPCEntity::onTrigger(const CollisionInfo& trigger, EngineContext* ctx) {
    if (m_isBuffGiven) return;

    if ((trigger.targetLayer & CollisionLayer::Layer_Player) != 0) {
        m_isBuffGiven = true;
        active = false;

        if (m_player) m_player->applyBuff(m_buffType);
        if (ctx) ctx->audioEngine.play(m_buffSFX, AudioCategory::GameplaySFX);
    }
}

void TeacherNPCEntity::onCollision(const CollisionInfo& collision, EngineContext* ctx) {
    onTrigger(collision, ctx);
}

void TeacherNPCEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    if (m_isBuffGiven) return;
    animator.draw(writeBuffer, ctx, renderPos - glm::vec2(10.0f, 10.0f), glm::vec2(64.0f, 64.0f), glm::vec4(1.0f), m_renderDepth, true);
}