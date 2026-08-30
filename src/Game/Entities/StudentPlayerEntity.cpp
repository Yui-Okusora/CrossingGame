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
    TextureHandle winSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/winAnimation.png");
    TextureHandle loseSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/loseAnimation.png");
    TextureHandle fallWaterSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/Fallwater.png");
    TextureHandle auraSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/MainChar/auraBuff.png");

    // Directional Movement & Idle (4 rows of 64x64 cat spritesheet)
    animator.addAnimation("hop_down", AnimationClip{ catSheet, { 6, 4 }, 0,  5,  0.05f, true });
    animator.addAnimation("hop_up", AnimationClip{ catSheet, { 6, 4 }, 6,  11, 0.05f, true });
    animator.addAnimation("hop_side", AnimationClip{ catSheet, { 6, 4 }, 12, 17, 0.05f, true });
    animator.addAnimation("idle", AnimationClip{ catSheet, { 6, 4 }, 18, 21, 0.28f, true });

    // Outcome Animations (Set win to non-looping false)
    animator.addAnimation("win", AnimationClip{ winSheet,       { 9, 1 }, 0, 8, 0.09f, false });
    animator.addAnimation("lose", AnimationClip{ loseSheet,      { 9, 1 }, 0, 8, 0.09f, false });
    animator.addAnimation("fall_water", AnimationClip{ fallWaterSheet, { 9, 1 }, 0, 8, 0.09f, false });

    // 4-Frame Looping Flame Aura Sprite
    m_auraAnimator.addAnimation("aura_loop", AnimationClip{ auraSheet, { 4, 1 }, 0, 3, 0.08f, true });
    m_auraAnimator.play("aura_loop");

    // Audio SFX
    m_hopSFX = ctx->audioEngine.loadSound(SFX_PATH "player_hop.wav");
    m_deathSFX = ctx->audioEngine.loadSound(SFX_PATH "player_death.wav");
    m_waterSplashSFX = ctx->audioEngine.loadSound(SFX_PATH "water_splash.wav");
    m_shieldBreakSFX = ctx->audioEngine.loadSound(SFX_PATH "shield_break.wav");

    playIdleAnimation();
}

void StudentPlayerEntity::playHopAnimation() {
    if (m_isDead || m_isWon) return;
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
    if (m_isDead || m_isWon) return;
    animator.flipX = false;
    animator.play("idle");
}

void StudentPlayerEntity::playWinAnimation() {
    m_isWon = true;
    m_isMoving = false;
    animator.flipX = false;
    animator.play("win", true);
}

void StudentPlayerEntity::playDeathAnimation(bool isWater) {
    m_isDead = true;
    m_isMoving = false;
    animator.flipX = false;
    if (isWater) {
        animator.play("fall_water", true);
    }
    else {
        animator.play("lose", true);
    }
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
    if (m_isDead || m_isWon) return false;

    if (m_buffs.onTakeDamage(this)) {
        if (ctx) ctx->audioEngine.play(m_shieldBreakSFX, AudioCategory::GameplaySFX);
        if (rescuePos != glm::vec2(0.0f, 0.0f)) {
            position = m_targetPosition = rescuePos;
        }
        return false;
    }

    bool isDrowning = isOnWaterLane;
    playDeathAnimation(isDrowning);

    if (ctx) {
        if (isDrowning) {
            ctx->audioEngine.play(m_waterSplashSFX, AudioCategory::GameplaySFX);
        }
        else {
            ctx->audioEngine.play(m_deathSFX, AudioCategory::GameplaySFX);
        }
    }

    if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
    return true;
}

void StudentPlayerEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_isDead || m_isWon) {
        return;
    }

    m_touchingLogThisFrame = false;
    m_auraAnimator.update(dt);

    m_buffs.onUpdate(dt, this);
    m_buffs.syncBlackboard(ctx);

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

            static std::vector<CollisionResult> obstacleHits;
            obstacleHits.clear();
            glm::vec4 testBox{ nextTarget.x, nextTarget.y, size.x, size.y };
            ctx->collisionWorld.query_aabb(testBox, CollisionLayer::Layer_Player, CollisionLayer::Layer_Obstacle, obstacleHits, id);

            if (obstacleHits.empty() && nextTarget != position) {
                m_targetPosition = nextTarget;
                m_isMoving = true;
                m_hasStartedFirstMove = true;
                playHopAnimation();

                if (ctx) ctx->audioEngine.play(m_hopSFX, AudioCategory::GameplaySFX);
            }
            else {
                playIdleAnimation();
            }

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
    if (m_isDead || m_isWon) return;

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
    if (m_isDead || m_isWon || m_isMoving) return;

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

    // 1. Render Animated Aura Sprite behind cat (Replaces solid rectangular box silhouette)
    if (!m_isDead && (hasSpeed || hasGpa || hasShield || hasInvincible)) {
        glm::vec4 auraTint = hasInvincible ? glm::vec4{ 0.2f, 0.9f, 1.0f, 0.90f }
            : hasShield ? glm::vec4{ 1.0f, 0.85f, 0.2f, 0.90f }
            : hasGpa ? glm::vec4{ 0.85f, 0.2f, 0.95f, 0.90f }
        : glm::vec4{ 0.2f, 0.95f, 0.4f, 0.90f };

        m_auraAnimator.draw(
            writeBuffer,
            ctx,
            renderPos - m_config.spriteOffset,
            m_config.spriteRenderSize,
            auraTint,
            m_config.renderDepth - 1,
            true
        );
    }

    // 2. Render Main Character Animation (Idle, Hop, Win, Lose, Fallwater)
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