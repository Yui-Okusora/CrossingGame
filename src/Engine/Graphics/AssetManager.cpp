#include "AssetManager.hpp"
#include <gl2d/gl2d.h>
#include <stdexcept>

AssetManager::AssetManager() {
    m_textures.emplace_back(); // Slot index 0 is permanently bound to default missing asset fallback [cite: 126, 127]
}

AssetManager::~AssetManager() = default;

TextureHandle AssetManager::loadTexture(const std::string& path, bool stream) {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex); // Exclusive write lock boundary [cite: 127]

    auto it = m_pathCache.find(path);
    if (it != m_pathCache.end()) {
        return TextureHandle{ it->second }; // Cache hit asset reuse [cite: 128]
    }

    gl2d::Texture tex;
    tex.loadFromFile(path.c_str(), !stream);

    uint32_t newId = static_cast<uint32_t>(m_textures.size());
    m_textures.push_back(tex);
    m_pathCache[path] = newId;
    return TextureHandle{ newId };
}

gl2d::Texture AssetManager::getTexture(TextureHandle handle) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex); // Concurrent shared read lock [cite: 132]
    if (handle.id >= m_textures.size()) {
        return m_textures[0]; // Return safe placeholder fallback if out of bounds bounds [cite: 133]
    }
    return m_textures[handle.id];
}