#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <functional>

struct PhysicsManifold {
    bool intersected = false;
    glm::vec2 contact_normal{ 0.0f, 0.0f };
    float penetration_depth = 0.0f;
};

// Extended 32-bit Bitmask Groupings
enum CollisionLayer : uint32_t {
    Layer_None          = 0,
    Layer_UI            = 1 << 0,  // Interface elements
    Layer_Player        = 1 << 1,  // Player character entity
    Layer_Enemy         = 1 << 2,  // Enemy character entities
    Layer_Obstacle      = 1 << 3,  // Destructible objects/hazards
    Layer_StaticWall    = 1 << 4,  // Solid environment tilemaps / hard limits
    Layer_PlayerBullet  = 1 << 5,  // Projectiles fired by player
    Layer_EnemyBullet   = 1 << 6,  // Projectiles fired by enemies
    Layer_TriggerVolume = 1 << 7,  // Non-blocking areas (e.g., item pickups, level exits)

    Layer_All = 0xFFFFFFFF
};

// Compact 32-byte collider token fitting perfectly on CPU cache lines
struct Collider {
    uint32_t id;         // Foreign tracking key (EntityID or WidgetID)
    glm::vec4 bounds;    // Box configuration: { x, y, width, height }
    uint32_t layer;      // The layer this collider belongs to
    uint32_t mask;       // Layers this collider is allowed to interact with
};

struct CollisionResult {
    uint32_t target_id;
    uint32_t target_layer;
    glm::vec4 intersected_bounds;
};

class CollisionEngine {
private:
    std::vector<Collider> m_colliders;

public:
    CollisionEngine();
    ~CollisionEngine();

    void clear_world() noexcept;
    void register_collider(uint32_t id, const glm::vec4& bounds, uint32_t layer, uint32_t mask = Layer_All);

    // Context-sensitive queries that evaluate both source AND target masks
    void query_point(const glm::vec2& point, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results) const;
    void query_aabb(const glm::vec4& bounds, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results) const;

    static inline bool intersects_aabb(const glm::vec4& a, const glm::vec4& b) noexcept {
        return (a.x < b.x + b.z && a.x + a.z > b.x &&
            a.y < b.y + b.w && a.y + a.w > b.y);
    }

    static inline PhysicsManifold CalculateManifold(const glm::vec4& rectA, const glm::vec4& rectB) noexcept {
        PhysicsManifold manifold;
        float halfW_A = rectA.z * 0.5f; float halfH_A = rectA.w * 0.5f;
        float halfW_B = rectB.z * 0.5f; float halfH_B = rectB.w * 0.5f;

        glm::vec2 centerA{ rectA.x + halfW_A, rectA.y + halfH_A };
        glm::vec2 centerB{ rectB.x + halfW_B, rectB.y + halfH_B };
        glm::vec2 delta = centerB - centerA;

        float overlapX = (halfW_A + halfW_B) - std::abs(delta.x);
        float overlapY = (halfH_A + halfH_B) - std::abs(delta.y);

        if (overlapX > 0.0f && overlapY > 0.0f) {
            manifold.intersected = true;
            if (overlapX < overlapY) {
                manifold.contact_normal = glm::vec2(delta.x < 0.0f ? 1.0f : -1.0f, 0.0f);
                manifold.penetration_depth = overlapX;
            }
            else {
                manifold.contact_normal = glm::vec2(0.0f, delta.y < 0.0f ? 1.0f : -1.0f);
                manifold.penetration_depth = overlapY;
            }
        }
        return manifold;
    }

    static inline void ResolveKinematicBounce(glm::vec2& entityPos, glm::vec2& entityVelocity,
        const glm::vec2& entitySize, const glm::vec4& staticBox,
        float restitution = 0.6f) noexcept
    {
        glm::vec4 currentBox{ entityPos.x, entityPos.y, entitySize.x, entitySize.y };
        PhysicsManifold manifold = CalculateManifold(currentBox, staticBox);

        if (manifold.intersected) {
            // Resolve structural AABB penetration overlaps instantly
            entityPos += manifold.contact_normal * manifold.penetration_depth;

            // Apply elastic impulse force reflection down the velocity coordinates
            float relVelProj = glm::dot(entityVelocity, manifold.contact_normal);
            if (relVelProj < 0.0f) {
                entityVelocity = entityVelocity - (1.0f + restitution) * relVelProj * manifold.contact_normal;
            }
        }
    }
};