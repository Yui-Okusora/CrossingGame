#include "AssetManager.hpp"
#include <gl2d/gl2d.h>
#include <stdexcept>

AssetManager::AssetManager() {
    m_textures.emplace_back(); // Slot index 0 is permanently bound to default missing asset fallback
}

AssetManager::~AssetManager() = default;

TextureHandle AssetManager::loadTexture(const std::string& path, bool stream) {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);

    auto it = m_pathCache.find(path);
    if (it != m_pathCache.end()) {
        return TextureHandle{ it->second };
    }

    gl2d::Texture tex;
    tex.loadFromFile(path.c_str(), !stream);

    // If texture failed to load or OpenGL texture ID is invalid, fallback to null handle 0
    if (tex.id == 0) {
        m_pathCache[path] = 0;
        return TextureHandle{ 0 };
    }

    uint32_t newId = static_cast<uint32_t>(m_textures.size());
    m_textures.push_back(tex);
    m_pathCache[path] = newId;
    return TextureHandle{ newId };
}

gl2d::Texture AssetManager::getTexture(TextureHandle handle) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    if (handle.id >= m_textures.size()) {
        return m_textures[0];
    }
    return m_textures[handle.id];
}