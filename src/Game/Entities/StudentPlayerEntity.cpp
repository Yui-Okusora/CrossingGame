#include "StudentPlayerEntity.hpp"
#include "../Layers/Gameplay.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

StudentPlayerEntity::StudentPlayerEntity(const glm::vec2& startPos, GameplayLayer* layerPtr, const PlayerConfig& config)
    : m_gameplayLayer(layerPtr), m_config(config) {
    position = startPos;
    prevPosition = startPos;
    m_targetPosition = startPos;
    size = m_config.entitySize;
    layer = CollisionLayer::Layer_Player;
    mask = CollisionLayer::Layer_Enemy | CollisionLayer::Layer_Obstacle | CollisionLayer::Layer_TriggerVolume;
    isStatic = false;
}

void StudentPlayerEntity::onUpdate(float dt, EngineContext* ctx) {
    if (m_isDead) return;

    m_touchingLogThisFrame = false;

    // 1. CAPTURE INPUT: Queue fresh keypresses into the input buffer regardless of m_isMoving state
    glm::vec2 freshDir{ 0.0f, 0.0f };
    if (ctx->input.isKeyJustPressed(GLFW_KEY_W) || ctx->input.isKeyJustPressed(GLFW_KEY_UP))    freshDir.y -= m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_S) || ctx->input.isKeyJustPressed(GLFW_KEY_DOWN))  freshDir.y += m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_A) || ctx->input.isKeyJustPressed(GLFW_KEY_LEFT))  freshDir.x -= m_config.gridSize;
    else if (ctx->input.isKeyJustPressed(GLFW_KEY_D) || ctx->input.isKeyJustPressed(GLFW_KEY_RIGHT)) freshDir.x += m_config.gridSize;

    if (freshDir != glm::vec2(0.0f, 0.0f)) {
        m_inputBuffer = freshDir;
        m_bufferTimer = INPUT_BUFFER_DURATION;
    }
    else if (m_bufferTimer > 0.0f) {
        m_bufferTimer -= dt;
        if (m_bufferTimer <= 0.0f) {
            m_inputBuffer = glm::vec2(0.0f, 0.0f);
        }
    }

    // 2. PROCESS MOVEMENT: Execute buffered or held direction inputs as soon as tile landing completes
    if (!m_isMoving) {
        glm::vec2 dir = m_inputBuffer;

        // Fallback to held keys if the input buffer is empty
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

            // Initiate movement only if target coordinate is valid
            if (nextTarget != position) {
                m_targetPosition = nextTarget;
                m_isMoving = true;
            }

            // Clear buffer once consumed
            m_inputBuffer = glm::vec2(0.0f, 0.0f);
            m_bufferTimer = 0.0f;
        }
    }

    // 3. INTERPOLATION: Exponential decay movement interpolation
    if (m_isMoving) {
        float factor = 1.0f - std::exp(-m_config.lerpSpeed * dt);
        position = glm::mix(position, m_targetPosition, factor);
        if (glm::distance(position, m_targetPosition) < 0.5f) {
            position = m_targetPosition;
            m_isMoving = false; // Landing complete; ready to consume next buffered hop immediately
        }
    }
}

void StudentPlayerEntity::onCollision(const CollisionInfo& collision, EngineContext* ctx) {
    if (m_isDead) return;

    if ((collision.targetLayer & CollisionLayer::Layer_Enemy) != 0) {
        m_isDead = true;
        if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
    }
    else if ((collision.targetLayer & CollisionLayer::Layer_Obstacle) != 0) {
        position = prevPosition;
        m_targetPosition = prevPosition;
        m_isMoving = false;
        m_inputBuffer = glm::vec2(0.0f, 0.0f); // Clear buffer on obstacle collision
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
                m_isDead = true;
                if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
            }
        }
        else if (isOnWaterLane) {
            std::cout << "[Player] Landed in water without log! Drowned!\n";
            m_isDead = true;
            if (m_gameplayLayer) m_gameplayLayer->triggerGameOver(ctx);
        }
    }
}

void StudentPlayerEntity::onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {
    writeBuffer.push_command(m_config.renderDepth, 0, RectPayload{
        .dest_rect = { renderPos.x + m_config.visualOffset.x, renderPos.y + m_config.visualOffset.y, size.x, size.y },
        .color = m_config.color,
        .no_texture = true,
        .is_world_space = true
        });
}