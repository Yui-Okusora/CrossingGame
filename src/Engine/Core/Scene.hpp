#pragma once
#include "../Context/EngineContext.hpp"
#include "../Graphics/RenderStream.hpp"
#include "../Physics/CollisionEngine.hpp"
#include <vector>
#include <memory>
#include <algorithm>

class Entity2D {
public:
    uint32_t id = 0;
    glm::vec2 position{ 0.0f, 0.0f };
    glm::vec2 prevPosition{ 0.0f, 0.0f };
    glm::vec2 size{ 32.0f, 32.0f };
    glm::vec2 velocity{ 0.0f, 0.0f };

    uint32_t layer = CollisionLayer::Layer_None;
    uint32_t mask = CollisionLayer::Layer_All;

    bool isStatic = false;
    bool isTrigger = false;
    bool active = true;

    virtual ~Entity2D() = default;

    // --- Pure Event Callbacks ---
    virtual void onUpdate(float dt, EngineContext* ctx) {}
    virtual void onCollision(const CollisionInfo& collision, EngineContext* ctx) {}
    virtual void onTrigger(const CollisionInfo& trigger, EngineContext* ctx) {}
    virtual void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {}
};

class Scene2D {
private:
    std::vector<std::unique_ptr<Entity2D>> m_entities;
    std::vector<CollisionResult> m_rawQueryBuffer;
    uint32_t m_nextEntityId = 1000;

public:
    Scene2D() { m_rawQueryBuffer.reserve(32); }

    template<typename T, typename... Args>
    T* spawn(Args&&... args) {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        entity->id = m_nextEntityId++;
        T* ptr = entity.get();
        m_entities.push_back(std::move(entity));
        return ptr;
    }

    void fixedUpdate(double dt, EngineContext* ctx) {
        float fDt = static_cast<float>(dt);

        // 1. Advance Physics Movement
        for (auto& entity : m_entities) {
            if (!entity->active) continue;
            entity->prevPosition = entity->position;
            entity->onUpdate(fDt, ctx);

            if (!entity->isStatic) {
                entity->position += entity->velocity * fDt;
            }
        }

        // 2. Evaluate Spatial Collisions & Dispatch Callbacks
        for (auto& entity : m_entities) {
            if (!entity->active || entity->layer == CollisionLayer::Layer_None) continue;

            glm::vec4 currentBox{ entity->position.x, entity->position.y, entity->size.x, entity->size.y };
            m_rawQueryBuffer.clear();

            // Ignore self-intersection at lowest engine layer
            ctx->collisionWorld.query_aabb(currentBox, entity->layer, entity->mask, m_rawQueryBuffer, entity->id);

            for (const auto& rawHit : m_rawQueryBuffer) {
                PhysicsManifold manifold = CollisionEngine::CalculateManifold(currentBox, rawHit.targetBounds);
                if (!manifold.intersected) continue;

                CollisionInfo info{
                    .targetId = rawHit.targetId,
                    .targetLayer = rawHit.targetLayer,
                    .isTrigger = entity->isTrigger,
                    .manifold = manifold
                };

                if (entity->isTrigger) {
                    entity->onTrigger(info, ctx);
                }
                else {
                    entity->onCollision(info, ctx);
                }
            }
        }

        // 3. Clean up inactive entities
        std::erase_if(m_entities, [](const auto& e) { return !e->active; });
    }

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
        float alpha = writeBuffer.physicsAlpha;

        for (auto& entity : m_entities) {
            if (!entity->active) continue;

            if (entity->layer != CollisionLayer::Layer_None) {
                ctx->collisionWorld.register_collider(
                    entity->id,
                    glm::vec4{ entity->position.x, entity->position.y, entity->size.x, entity->size.y },
                    entity->layer,
                    entity->mask
                );
            }

            glm::vec2 lerpedPos = glm::mix(entity->prevPosition, entity->position, alpha);
            entity->onRender(writeBuffer, ctx, lerpedPos);
        }
    }

    void clear() { m_entities.clear(); }
};