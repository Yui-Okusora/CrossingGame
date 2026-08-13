#pragma once
#include "../Context/EngineContext.hpp"
#include "../Graphics/RenderStream.hpp"
#include "../Physics/CollisionEngine.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <string>
#include <unordered_map>

// ============================================================================
// SPRITESHEET ANIMATION CLIP
// ============================================================================
struct AnimationClip {
    TextureHandle texture{};
    glm::uvec2 atlasDimensions{ 1, 1 }; // {Columns, Rows} in spritesheet
    int startFrame = 0;
    int endFrame = 0;
    float frameDuration = 0.1f;         // Seconds per frame
    bool loop = true;                   // Loop upon reaching endFrame

    // Runtime State
    float timer = 0.0f;
    int currentFrame = 0;
    bool finished = false;

    void reset() {
        currentFrame = startFrame;
        timer = 0.0f;
        finished = false;
    }

    void update(float dt) {
        if (finished && !loop) return;

        timer += dt;
        if (timer >= frameDuration) {
            timer -= frameDuration;
            currentFrame++;
            if (currentFrame > endFrame) {
                if (loop) {
                    currentFrame = startFrame;
                }
                else {
                    currentFrame = endFrame;
                    finished = true;
                }
            }
        }
    }

    [[nodiscard]] bool isFinished() const noexcept { return finished; }

    // Convert 1D frame index to 2D (Column, Row) coordinates for UV mapping
    [[nodiscard]] glm::uvec2 getAtlasPos() const noexcept {
        if (atlasDimensions.x == 0) return { 0, 0 };
        uint32_t col = currentFrame % atlasDimensions.x;
        uint32_t row = currentFrame / atlasDimensions.x;
        return { col, row };
    }
};

// ============================================================================
// ANIMATOR 2D COMPONENT
// ============================================================================
class Animator2D {
private:
    std::unordered_map<std::string, AnimationClip> m_animations;
    std::string m_currentAnimKey;
    AnimationClip* m_currentClip = nullptr;

public:
    bool flipX = false; // Horizontal mirror toggle for left/right facing sprites

    Animator2D() = default;

    // Register a named animation clip into the animator state machine
    void addAnimation(const std::string& name, const AnimationClip& clip) {
        m_animations[name] = clip;
        m_animations[name].reset();
        if (!m_currentClip) {
            play(name);
        }
    }

    // Switch active playing animation
    void play(const std::string& name, bool forceRestart = false) {
        if (m_currentAnimKey == name && !forceRestart) return;

        auto it = m_animations.find(name);
        if (it != m_animations.end()) {
            m_currentAnimKey = name;
            m_currentClip = &it->second;
            m_currentClip->reset();
        }
    }

    // Advance sprite animation timeline
    void update(float dt) {
        if (m_currentClip) {
            m_currentClip->update(dt);
        }
    }

    [[nodiscard]] const std::string& getCurrentAnimationName() const noexcept { return m_currentAnimKey; }
    [[nodiscard]] bool isCurrentAnimationFinished() const noexcept {
        return m_currentClip ? m_currentClip->isFinished() : true;
    }

    // Directly emit textured sprite command into RenderData
    void draw(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos,
        const glm::vec2& drawSize, const glm::vec4& tint = { 1.0f, 1.0f, 1.0f, 1.0f },
        int32_t depth = 40, bool isWorldSpace = true) {
        if (!m_currentClip) return;

        writeBuffer.push_command(depth, 0, RectPayload{
            .dest_rect = { renderPos.x, renderPos.y, drawSize.x, drawSize.y },
            .color = tint,
            .texture = m_currentClip->texture,
            .atlas_dimensions = m_currentClip->atlasDimensions,
            .atlas_pos = m_currentClip->getAtlasPos(),
            .origin = { 0.0f, 0.0f },
            .rotation = 0.0f,
            .flip_x = flipX,
            .no_texture = false, // Enable GPU Spritesheet Sampling
            .is_world_space = isWorldSpace
            });
    }
};

// ============================================================================
// BASE 2D ENTITY
// ============================================================================
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

    Animator2D animator;                        // Embedded Spritesheet Animation Component

    virtual ~Entity2D() = default;

    // --- Pure Event Callbacks ---
    virtual void onUpdate(float dt, EngineContext* ctx) {}
    virtual void onCollision(const CollisionInfo& collision, EngineContext* ctx) {}
    virtual void onTrigger(const CollisionInfo& trigger, EngineContext* ctx) {}
    virtual void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) {}
};

// ============================================================================
// 2D SCENE GRAPH & SIMULATION CORE
// ============================================================================
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

        // 1. Advance Physics & Animation State Machine
        for (auto& entity : m_entities) {
            if (!entity->active) continue;
            entity->prevPosition = entity->position;

            // Advance entity animator tick
            entity->animator.update(fDt);

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