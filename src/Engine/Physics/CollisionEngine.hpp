#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cmath>

struct PhysicsManifold {
    bool intersected = false;
    glm::vec2 contact_normal{ 0.0f, 0.0f };
    float penetration_depth = 0.0f;
};

enum CollisionLayer : uint32_t {
    Layer_None = 0,
    Layer_UI = 1 << 0,
    Layer_Player = 1 << 1,
    Layer_Enemy = 1 << 2,
    Layer_Obstacle = 1 << 3,
    Layer_StaticWall = 1 << 4,
    Layer_PlayerBullet = 1 << 5,
    Layer_EnemyBullet = 1 << 6,
    Layer_TriggerVolume = 1 << 7,
    Layer_All = 0xFFFFFFFF
};

struct Collider {
    uint32_t id = 0;
    glm::vec4 bounds{ 0.0f };
    uint32_t layer = Layer_None;
    uint32_t mask = Layer_All;
    uint32_t queryToken = 0; // Prevents duplicate testing across overlapping grid cells
};

struct CollisionResult {
    uint32_t targetId = 0;
    uint32_t targetLayer = 0;
    glm::vec4 targetBounds{ 0.0f };
};

struct CollisionInfo {
    uint32_t targetId = 0;
    uint32_t targetLayer = 0;
    bool isTrigger = false;
    PhysicsManifold manifold;
};

class PhysicsResponse {
public:
    static inline void Slide(glm::vec2& position, glm::vec2& velocity, const PhysicsManifold& manifold) noexcept {
        if (!manifold.intersected) return;
        position += manifold.contact_normal * manifold.penetration_depth;
        float velProj = glm::dot(velocity, manifold.contact_normal);
        if (velProj < 0.0f) velocity -= velProj * manifold.contact_normal;
    }

    static inline void Bounce(glm::vec2& position, glm::vec2& velocity, const PhysicsManifold& manifold, float restitution = 0.75f) noexcept {
        if (!manifold.intersected) return;
        position += manifold.contact_normal * manifold.penetration_depth;
        float relVelProj = glm::dot(velocity, manifold.contact_normal);
        if (relVelProj < 0.0f) velocity -= (1.0f + restitution) * relVelProj * manifold.contact_normal;
    }
};

class CollisionEngine {
private:
    std::vector<Collider> m_colliders;

    // Spatial Hash Grid Storage
    constexpr static float CELL_SIZE = 128.0f;
    std::unordered_map<uint64_t, std::vector<uint32_t>> m_grid;
    mutable uint32_t m_currentQueryToken = 0;

    [[nodiscard]] static inline uint64_t hashCell(int x, int y) noexcept {
        return (static_cast<uint64_t>(x) << 32) | (static_cast<uint32_t>(y) & 0xFFFFFFFF);
    }

public:
    CollisionEngine();
    ~CollisionEngine();

    void clear_world() noexcept;
    void register_collider(uint32_t id, const glm::vec4& bounds, uint32_t layer, uint32_t mask = Layer_All);

    void query_point(const glm::vec2& point, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results) const;
    void query_aabb(const glm::vec4& bounds, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results, uint32_t ignore_id = 0) const;

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
};