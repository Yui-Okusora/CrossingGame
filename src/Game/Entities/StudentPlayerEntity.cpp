#include "StudentPlayerEntity.hpp"
#include "../Layers/Gameplay.hpp"
#include <algorithm>
#include <cmath>

void SpeedBoostEffect::onApply(StudentPlayerEntity* player) { player->setSpeedBoostActive(true); }
void SpeedBoostEffect::onRemove(StudentPlayerEntity* player) { player->setSpeedBoostActive(false); }
void GpaMultiplierEffect::onApply(StudentPlayerEntity* player) { player->setScoreMultiplier(2); }
void GpaMultiplierEffect::onRemove(StudentPlayerEntity* player) { player->setScoreMultiplier(1); }

void DeadlineShieldEffect::onRemove(StudentPlayerEntity* player) {
    if (m_consumed) {
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

    // Row 0: Down Movement (Frames 0 - 5)
    animator.addAnimation("hop_down", AnimationClip{ catSheet, { 6, 4 }, 0, 5, 0.05f, true });

    // Row 1: Up Movement (Frames 6 - 11)
    animator.addAnimation("hop_up", AnimationClip{ catSheet, { 6, 4 }, 6, 11, 0.05f, true });

    // Row 2: Side Movement (Frames 12 - 17, flipX handles Left)
    animator.addAnimation("hop_side", AnimationClip{ catSheet, { 6, 4 }, 12, 17, 0.05f, true });

    // Row 3: Universal Idle (Frames 18 - 21)
    animator.addAnimation("idle", AnimationClip{ catSheet, { 6, 4 }, 18, 21, 0.28f, true });

    playIdleAnimation();
}

void StudentPlayerEntity::playHopAnimation() {
    switch (m_facing) {
    case PlayerDirection::Down:
        animator.flipX = false;
        animator.play("hop_down");
        break;
    case PlayerDirection::Up:
        animator.flipX = false;
        animator.play("hop_up");
        break;
    case PlayerDirection::Right:
        animator.flipX = false;
        animator.play("hop_side");
        break;
    case PlayerDirection::Left:
        animator.flipX = true;
        animator.play("hop_side");
        break;
    }
}

void StudentPlayerEntity::playIdleAnimation() {
    animator.flipX = false;
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
        return false;
    }

    m_isDead = true;
    if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
    return true;
}

void StudentPlayerEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_isDead) return;
    m_touchingLogThisFrame = false;
    m_auraAnimTimer += dt;

    m_buffs.onUpdate(dt, this);
    m_buffs.syncBlackboard(ctx);

    // Tightened input buffer window: prevents high-speed lerp from consuming a residual press as a 2nd step
    const bool isSpeedBoosted = m_buffs.hasEffect(TeacherBuffType::SpeedBoost);
    const float maxBufferDuration = isSpeedBoosted ? 0.08f : 0.12f;

    glm::vec2 freshDir{ 0.0f, 0.0f };
    if (ctx->input.isKeyJustPressed(GLFW_KEY_W) || ctx->input.isKeyJustPressed(GLFW_KEY_UP)) {
        freshDir.y -= m_config.gridSize;
        m_facing = PlayerDirection::Up;
    }
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_S) || ctx->input.isKeyJustPressed(GLFW_KEY_DOWN)) {
        freshDir.y += m_config.gridSize;
        m_facing = PlayerDirection::Down;
    }
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_A) || ctx->input.isKeyJustPressed(GLFW_KEY_LEFT)) {
        freshDir.x -= m_config.gridSize;
        m_facing = PlayerDirection::Left;
    }
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_D) || ctx->input.isKeyJustPressed(GLFW_KEY_RIGHT)) {
        freshDir.x += m_config.gridSize;
        m_facing = PlayerDirection::Right;
    }

    if (freshDir != glm::vec2(0.0f, 0.0f)) {
        m_inputBuffer = freshDir;
        m_bufferTimer = maxBufferDuration;
    }
    else if (m_bufferTimer > 0.0f) {
        m_bufferTimer -= dt;
        if (m_bufferTimer <= 0.0f) {
            m_inputBuffer = glm::vec2(0.0f, 0.0f);
        }
    }

    if (!m_isMoving) {
        glm::vec2 dir = m_inputBuffer;
        if (dir != glm::vec2(0.0f, 0.0f)) {
            glm::vec2 nextTarget = m_targetPosition + dir;
            nextTarget.x = std::clamp(nextTarget.x, m_config.minX, m_config.maxX);
            nextTarget.y = (std::min)(m_config.maxY, nextTarget.y);

            // Pre-check static obstacle obstruction
            static std::vector<CollisionResult> obstacleHits;
            obstacleHits.clear();
            glm::vec4 testBox{ nextTarget.x, nextTarget.y, size.x, size.y };
            ctx->collisionWorld.query_aabb(testBox, CollisionLayer::Layer_Player, CollisionLayer::Layer_Obstacle, obstacleHits, id);

            if (obstacleHits.empty() && nextTarget != position) {
                m_targetPosition = nextTarget;
                m_isMoving = true;
                m_hasStartedFirstMove = true;
                playHopAnimation();
            }
            else {
                playIdleAnimation();
            }

            // Immediately flush buffer to prevent consecutive runaway steps
            m_inputBuffer = glm::vec2(0.0f, 0.0f);
            m_bufferTimer = 0.0f;
        }
    }

    if (m_isMoving) {
        float factor = 1.0f - std::exp(-m_config.currentLerpSpeed * dt);
        position = glm::mix(position, m_targetPosition, factor);
        if (glm::distance(position, m_targetPosition) < 0.5f) {
            position = m_targetPosition;
            m_isMoving = false;
            playIdleAnimation();
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
        m_bufferTimer = 0.0f;
        playIdleAnimation();
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
    bool hasSpeed = m_buffs.hasEffect(TeacherBuffType::SpeedBoost);
    bool hasGpa = m_buffs.hasEffect(TeacherBuffType::GpaMultiplier);
    bool hasShield = m_buffs.hasEffect(TeacherBuffType::DeadlineShield);
    bool hasInvincible = m_buffs.hasEffect(TeacherBuffType::Invincibility);

    if (hasSpeed || hasGpa || hasShield || hasInvincible) {
        float pulse = 0.5f + 0.5f * std::sin(m_auraAnimTimer * 8.0f);
        glm::vec4 auraColor = hasInvincible ? glm::vec4{ 0.2f, 0.9f, 1.0f, 0.45f + 0.25f * pulse }
            : hasShield ? glm::vec4{ 1.0f, 0.85f, 0.2f, 0.40f + 0.20f * pulse }
            : hasGpa ? glm::vec4{ 0.85f, 0.2f, 0.95f, 0.40f + 0.20f * pulse }
        : glm::vec4{ 0.2f, 0.95f, 0.4f, 0.40f + 0.20f * pulse };

        writeBuffer.push_command(m_config.renderDepth - 1, 0, RectPayload{
            .dest_rect = { renderPos.x - 8.0f, renderPos.y - 8.0f, size.x + 16.0f, size.y + 16.0f },
            .color = auraColor,
            .no_texture = true,
            .is_world_space = true
            });
    }

    animator.draw(
        writeBuffer,
        ctx,
        renderPos - m_config.spriteOffset,
        m_config.spriteRenderSize,
        glm::vec4(1.0f),
        m_config.renderDepth,
        true
    );
}