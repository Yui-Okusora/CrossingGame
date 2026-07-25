#include "CollisionEngine.hpp"
#include "../Utils/Utils.hpp"

CollisionEngine::CollisionEngine() {
    m_colliders.reserve(256);
}

CollisionEngine::~CollisionEngine() = default;

void CollisionEngine::clear_world() noexcept {
    m_colliders.clear();
}

void CollisionEngine::register_collider(uint32_t id, const glm::vec4& bounds, uint32_t layer, uint32_t mask) {
    m_colliders.push_back({ id, bounds, layer, mask });
}

void CollisionEngine::query_point(const glm::vec2& point, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results) const {
    for (const auto& col : m_colliders) {
        if ((col.layer & target_mask) == 0) continue;
        if (source_layer != Layer_None && (source_layer & col.mask) == 0) continue;

        if (Utils::inRect(point, glm::vec2(col.bounds.x, col.bounds.y), glm::vec2(col.bounds.z, col.bounds.w))) {
            out_results.push_back({ col.id, col.layer, col.bounds });
        }
    }
}

void CollisionEngine::query_aabb(const glm::vec4& bounds, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results, uint32_t ignore_id) const {
    for (const auto& col : m_colliders) {
        if (col.id == ignore_id) continue; // Early-out self intersection check
        if ((col.layer & target_mask) == 0) continue;
        if (source_layer != Layer_None && (source_layer & col.mask) == 0) continue;

        if (intersects_aabb(bounds, col.bounds)) {
            out_results.push_back({ col.id, col.layer, col.bounds });
        }
    }
}