#include "StudentPlayerEntity.hpp"
#include "../Layers/Gameplay.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

StudentPlayerEntity::StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config)
    : m_gameplayLayer(layerPtr), m_config(config) {
    position = prevPosition = m_targetPosition = startPos;
    size = m_config.entitySize;
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
        m_speedBoostTimer = 10.0f;
        m_config.lerpSpeed = 38.0f;
        break;
    case TeacherBuffType::DeadlineShield:
        m_hasShield = true;
        break;
    case TeacherBuffType::GpaMultiplier:
        m_scoreMultiplier = 2;
        m_gpaMultiplierTimer = 12.0f;
        break;
    }
}

void StudentPlayerEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_isDead) return;
    m_touchingLogThisFrame = false;

    if (m_speedBoostTimer > 0.0f && (m_speedBoostTimer -= dt) <= 0.0f) {
        m_speedBoostTimer = 0.0f;
        m_config.lerpSpeed = 22.0f;
    }
    if (m_gpaMultiplierTimer > 0.0f && (m_gpaMultiplierTimer -= dt) <= 0.0f) {
        m_gpaMultiplierTimer = 0.0f;
        m_scoreMultiplier = 1;
    }
    if (m_invincibilityTimer > 0.0f && (m_invincibilityTimer -= dt) <= 0.0f) {
        m_invincibilityTimer = 0.0f;
    }

    if (ctx) {
        ctx->blackboard.set("buffShield", m_hasShield);
        ctx->blackboard.set("buffSpeedTimer", m_speedBoostTimer);
        ctx->blackboard.set("buffGpaTimer", m_gpaMultiplierTimer);
        ctx->blackboard.set("buffInvincibleTimer", m_invincibilityTimer);
    }

    glm::vec2 freshDir{ 0.0f, 0.0f };
    if (ctx->input.isKeyJustPressed(GLFW_KEY_W) || ctx->input.isKeyJustPressed(GLFW_KEY_UP))    freshDir.y -= m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_S) || ctx->input.isKeyJustPressed(GLFW_KEY_DOWN))  freshDir.y += m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_A) || ctx->input.isKeyJustPressed(GLFW_KEY_LEFT))  freshDir.x -= m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_D) || ctx->input.isKeyJustPressed(GLFW_KEY_RIGHT)) freshDir.x += m_config.gridSize;

    if (freshDir != glm::vec2(0.0f, 0.0f)) {
        m_inputBuffer = freshDir;
        m_bufferTimer = INPUT_BUFFER_DURATION;
    }
    else if (m_bufferTimer > 0.0f && (m_bufferTimer -= dt) <= 0.0f) {
        m_inputBuffer = glm::vec2(0.0f, 0.0f);
    }

    if (!m_isMoving) {
        glm::vec2 dir = m_inputBuffer;
        if (dir == glm::vec2(0.0f, 0.0f)) {
            if (ctx->input.isKeyJustPressed(GLFW_KEY_W) || ctx->input.isKeyJustPressed(GLFW_KEY_UP))    dir.y -= m_config.gridSize;
            else if (ctx->input.isKeyJustPressed(GLFW_KEY_S) || ctx->input.isKeyJustPressed(GLFW_KEY_DOWN))  dir.y += m_config.gridSize;
            else if (ctx->input.isKeyJustPressed(GLFW_KEY_A) || ctx->input.isKeyJustPressed(GLFW_KEY_LEFT))  dir.x -= m_config.gridSize;
            else if (ctx->input.isKeyJustPressed(GLFW_KEY_D) || ctx->input.isKeyJustPressed(GLFW_KEY_RIGHT)) dir.x += m_config.gridSize;
        }

        if (dir != glm::vec2(0.0f, 0.0f)) {
            glm::vec2 nextTarget = m_targetPosition + dir;
            nextTarget.x = std::clamp(nextTarget.x, m_config.minX, m_config.maxX);
            nextTarget.y = (std::min)(m_config.maxY, nextTarget.y);

            if (nextTarget != position) {
                m_targetPosition = nextTarget;
                m_isMoving = true;
                m_hasStartedFirstMove = true; // Signal first move initiated
            }
            m_inputBuffer = glm::vec2(0.0f, 0.0f);
            m_bufferTimer = 0.0f;
        }
    }

    if (m_isMoving) {
        animator.play("hop");
        float factor = 1.0f - std::exp(-m_config.lerpSpeed * dt);
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
        if (m_invincibilityTimer > 0.0f) return;

        if (m_hasShield) {
            m_hasShield = false;
            m_invincibilityTimer = SHIELD_INVINCIBILITY_DURATION;
            if (ctx) {
                ctx->blackboard.set("buffShield", false);
                ctx->blackboard.set("buffInvincibleTimer", m_invincibilityTimer);
            }
        }
        else {
            m_isDead = true;
            if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
        }
    }
    else if ((collision.targetLayer & CollisionLayer::Layer_Obstacle) != 0) {
        position = prevPosition;
        m_targetPosition = prevPosition;
        m_isMoving = false;
        m_inputBuffer = glm::vec2(0.0f, 0.0f);
    }
    else if ((collision.targetLayer & CollisionLayer::Layer_TriggerVolume) != 0) {
        m_touchingLogThisFrame = true;
    }
}

void StudentPlayerEntity::postPhysicsUpdate(float dt, EngineContext* ctx) {
    if (m_isDead) return;

    if (!m_isMoving) {
        if (m_touchingLogThisFrame) {
            position.x += currentWaterVel.x * dt;
            m_targetPosition.x = position.x;

            if (position.x < m_config.minX - 20.0f || position.x > m_config.maxX + 20.0f) {
                if (m_invincibilityTimer <= 0.0f) {
                    if (m_hasShield) {
                        m_hasShield = false;
                        m_invincibilityTimer = SHIELD_INVINCIBILITY_DURATION;
                        position.x = m_targetPosition.x = 576.0f;
                    }
                    else {
                        m_isDead = true;
                        if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
                    }
                }
            }
        }
        else if (isOnWaterLane) {
            if (m_invincibilityTimer <= 0.0f) {
                if (m_hasShield) {
                    m_hasShield = false;
                    m_invincibilityTimer = SHIELD_INVINCIBILITY_DURATION;
                }
                else {
                    m_isDead = true;
                    if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
                }
            }
        }
    }
}

void StudentPlayerEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    animator.draw(writeBuffer, ctx, renderPos + m_config.visualOffset, size, glm::vec4(1.0f), m_config.renderDepth, true);

    if (m_invincibilityTimer > 0.0f) {
        if (static_cast<int>(m_invincibilityTimer * 12.0f) % 2 == 0) {
            writeBuffer.push_command(m_config.renderDepth + 1, 0, RectPayload{
                .dest_rect = { renderPos.x + 2.0f, renderPos.y + 2.0f, size.x + 16.0f, size.y + 16.0f },
                .color = { 0.2f, 0.9f, 1.0f, 0.6f },
                .no_texture = true,
                .is_world_space = true
                });
        }
    }
    else if (m_hasShield) {
        writeBuffer.push_command(m_config.renderDepth + 1, 0, RectPayload{
            .dest_rect = { renderPos.x + 4.0f, renderPos.y + 4.0f, size.x + 12.0f, size.y + 12.0f },
            .color = { 1.0f, 0.85f, 0.1f, 0.45f },
            .no_texture = true,
            .is_world_space = true
            });
    }
}