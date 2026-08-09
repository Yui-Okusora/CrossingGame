#include "CollisionEngine.hpp"
#include "../Utils/Utils.hpp"

CollisionEngine::CollisionEngine() {
    m_colliders.reserve(512);
    m_grid.reserve(128);
}

CollisionEngine::~CollisionEngine() = default;

void CollisionEngine::clear_world() noexcept {
    m_colliders.clear();
    m_grid.clear();
}

void CollisionEngine::register_collider(uint32_t id, const glm::vec4& bounds, uint32_t layer, uint32_t mask) {
    uint32_t index = static_cast<uint32_t>(m_colliders.size());
    m_colliders.push_back({ id, bounds, layer, mask, 0 });

    // Map collider into grid cells
    int minX = static_cast<int>(std::floor(bounds.x / CELL_SIZE));
    int maxX = static_cast<int>(std::floor((bounds.x + bounds.z) / CELL_SIZE));
    int minY = static_cast<int>(std::floor(bounds.y / CELL_SIZE));
    int maxY = static_cast<int>(std::floor((bounds.y + bounds.w) / CELL_SIZE));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            m_grid[hashCell(x, y)].push_back(index);
        }
    }
}

void CollisionEngine::query_point(const glm::vec2& point, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results) const {
    int cellX = static_cast<int>(std::floor(point.x / CELL_SIZE));
    int cellY = static_cast<int>(std::floor(point.y / CELL_SIZE));

    auto it = m_grid.find(hashCell(cellX, cellY));
    if (it == m_grid.end()) return;

    for (uint32_t idx : it->second) {
        const auto& col = m_colliders[idx];
        if ((col.layer & target_mask) == 0) continue;
        if (source_layer != Layer_None && (source_layer & col.mask) == 0) continue;

        if (Utils::inRect(point, glm::vec2(col.bounds.x, col.bounds.y), glm::vec2(col.bounds.z, col.bounds.w))) {
            out_results.push_back({ col.id, col.layer, col.bounds });
        }
    }
}

void CollisionEngine::query_aabb(const glm::vec4& bounds, uint32_t source_layer, uint32_t target_mask, std::vector<CollisionResult>& out_results, uint32_t ignore_id) const {
    m_currentQueryToken++; // Deduplication key for this query pass

    int minX = static_cast<int>(std::floor(bounds.x / CELL_SIZE));
    int maxX = static_cast<int>(std::floor((bounds.x + bounds.z) / CELL_SIZE));
    int minY = static_cast<int>(std::floor(bounds.y / CELL_SIZE));
    int maxY = static_cast<int>(std::floor((bounds.y + bounds.w) / CELL_SIZE));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            auto it = m_grid.find(hashCell(x, y));
            if (it == m_grid.end()) continue;

            for (uint32_t idx : it->second) {
                auto& col = const_cast<Collider&>(m_colliders[idx]);

                // Early out if already tested or matches ignore ID
                if (col.queryToken == m_currentQueryToken || col.id == ignore_id) continue;
                col.queryToken = m_currentQueryToken;

                if ((col.layer & target_mask) == 0) continue;
                if (source_layer != Layer_None && (source_layer & col.mask) == 0) continue;

                if (intersects_aabb(bounds, col.bounds)) {
                    out_results.push_back({ col.id, col.layer, col.bounds });
                }
            }
        }
    }
}