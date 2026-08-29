#include "StudentPlayerEntity.hpp"
#include "../Layers/Gameplay.hpp"
#include <algorithm>
#include <cmath>

void SpeedBoostEffect::onApply(StudentPlayerEntity* player) { player->setSpeedModifier(38.0f); }
void SpeedBoostEffect::onRemove(StudentPlayerEntity* player) { player->setSpeedModifier(22.0f); }
void GpaMultiplierEffect::onApply(StudentPlayerEntity* player) { player->setScoreMultiplier(2); }
void GpaMultiplierEffect::onRemove(StudentPlayerEntity* player) { player->setScoreMultiplier(1); }

void DeadlineShieldEffect::onRemove(StudentPlayerEntity* player) {
    if (m_consumed) {
        // Automatically grant invulnerability upon shield removal
        player->applyBuff(TeacherBuffType::Invincibility);
    }
}

StudentPlayerEntity::StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config)
    : m_gameplayLayer(layerPtr), m_config(config) {
    position = prevPosition = m_targetPosition = startPos;
    size = m_config.hitboxSize;
    layer = CollisionLayer::Layer_Player;
    mask = CollisionLayer::Layer_Enemy | CollisionLayer::Layer_Obstacle | CollisionLayer::Layer_TriggerVolume;
    isStatic = false;
}

void StudentPlayerEntity::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    TextureHandle catSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/Cat character rework - animation.png");
    animator.addAnimation("idle", AnimationClip{ catSheet, { 6, 4 }, 0, 1, 0.35f, true });
    animator.addAnimation("hop", AnimationClip{ catSheet, { 6, 4 }, 6, 11, 0.06f, true });
    animator.play("idle");
}

void StudentPlayerEntity::applyBuff(TeacherBuffType buffType) {
    switch (buffType) {
    case TeacherBuffType::SpeedBoost:
        m_buffs.addEffect(std::make_unique<SpeedBoostEffect>(), this);
        break;
    case TeacherBuffType::DeadlineShield:
        m_buffs.addEffect(std::make_unique<DeadlineShieldEffect>(), this);
        break;
    case TeacherBuffType::GpaMultiplier:
        m_buffs.addEffect(std::make_unique<GpaMultiplierEffect>(), this);
        break;
    case TeacherBuffType::Invincibility:
        m_buffs.addEffect(std::make_unique<InvincibilityEffect>(2.0f), this);
        break;
    }
}

bool StudentPlayerEntity::handleLethalDamage(EngineContext* ctx, const glm::vec2& rescuePos) {
    if (m_buffs.onTakeDamage(this)) {
        if (rescuePos != glm::vec2(0.0f, 0.0f)) {
            position = m_targetPosition = rescuePos;
        }
        return false; // Survived damage via status effect mitigation (shield/invincibility)
    }

    m_isDead = true;
    if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
    return true;
}

void StudentPlayerEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_isDead) return;
    m_touchingLogThisFrame = false;

    m_buffs.onUpdate(dt, this);
    m_buffs.syncBlackboard(ctx);

    glm::vec2 freshDir{ 0.0f, 0.0f };
    if (ctx->input.isKeyJustPressed(GLFW_KEY_W) || ctx->input.isKeyJustPressed(GLFW_KEY_UP))    freshDir.y -= m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_S) || ctx->input.isKeyJustPressed(GLFW_KEY_DOWN))  freshDir.y += m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_A) || ctx->input.isKeyJustPressed(GLFW_KEY_LEFT))  freshDir.x -= m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_D) || ctx->input.isKeyJustPressed(GLFW_KEY_RIGHT)) freshDir.x += m_config.gridSize;

    if (freshDir != glm::vec2(0.0f, 0.0f)) {
        m_inputBuffer = freshDir;
        m_bufferTimer = 0.18f;
    }
    else if (m_bufferTimer > 0.0f && (m_bufferTimer -= dt) <= 0.0f) {
        m_inputBuffer = glm::vec2(0.0f, 0.0f);
    }

    if (!m_isMoving) {
        glm::vec2 dir = m_inputBuffer;
        if (dir != glm::vec2(0.0f, 0.0f)) {
            glm::vec2 nextTarget = m_targetPosition + dir;
            nextTarget.x = std::clamp(nextTarget.x, m_config.minX, m_config.maxX);
            nextTarget.y = (std::min)(m_config.maxY, nextTarget.y);

            if (nextTarget != position) {
                m_targetPosition = nextTarget;
                m_isMoving = true;
                m_hasStartedFirstMove = true;
            }
            m_inputBuffer = glm::vec2(0.0f, 0.0f);
            m_bufferTimer = 0.0f;
        }
    }

    if (m_isMoving) {
        animator.play("hop");
        float factor = 1.0f - std::exp(-m_config.currentLerpSpeed * dt);
        position = glm::mix(position, m_targetPosition, factor);
        if (glm::distance(position, m_targetPosition) < 0.5f) {
            position = m_targetPosition;
            m_isMoving = false;
            animator.play("idle");
        }
    }
}

void StudentPlayerEntity::onCollision(const CollisionInfo& collision, EngineContext* ctx) {
    if (m_isDead) return;

    if ((collision.targetLayer & CollisionLayer::Layer_Enemy) != 0) {
        handleLethalDamage(ctx);
    }
    else if ((collision.targetLayer & CollisionLayer::Layer_Obstacle) != 0) {
        position = m_targetPosition = prevPosition;
        m_isMoving = false;
        m_inputBuffer = glm::vec2(0.0f, 0.0f);
    }
    else if ((collision.targetLayer & CollisionLayer::Layer_TriggerVolume) != 0) {
        m_touchingLogThisFrame = true;
    }
}

void StudentPlayerEntity::postPhysicsUpdate(float dt, EngineContext* ctx) {
    if (m_isDead || m_isMoving) return;

    if (m_touchingLogThisFrame) {
        position.x += currentWaterVel.x * dt;
        m_targetPosition.x = position.x;

        if (position.x < m_config.minX - 20.0f || position.x > m_config.maxX + 20.0f) {
            handleLethalDamage(ctx, { 576.0f, position.y });
        }
    }
    else if (isOnWaterLane) {
        handleLethalDamage(ctx);
    }
}

void StudentPlayerEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    animator.draw(
        writeBuffer,
        ctx,
        renderPos - m_config.spriteOffset,
        m_config.spriteRenderSize,
        glm::vec4(1.0f),
        m_config.renderDepth,
        true
    );

    if (m_buffs.hasEffect(TeacherBuffType::Invincibility)) {
        if (static_cast<int>(m_buffs.getRemainingTime(TeacherBuffType::Invincibility) * 12.0f) % 2 == 0) {
            writeBuffer.push_command(m_config.renderDepth + 1, 0, RectPayload{
                .dest_rect = { renderPos.x - 4.0f, renderPos.y - 4.0f, size.x + 8.0f, size.y + 8.0f },
                .color = { 0.2f, 0.9f, 1.0f, 0.6f },
                .no_texture = true,
                .is_world_space = true
                });
        }
    }
    else if (m_buffs.hasEffect(TeacherBuffType::DeadlineShield)) {
        writeBuffer.push_command(m_config.renderDepth + 1, 0, RectPayload{
            .dest_rect = { renderPos.x - 2.0f, renderPos.y - 2.0f, size.x + 4.0f, size.y + 4.0f },
            .color = { 1.0f, 0.85f, 0.1f, 0.45f },
            .no_texture = true,
            .is_world_space = true
            });
    }
}