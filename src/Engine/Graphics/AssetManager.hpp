#pragma once
#include "../Graphics/RenderCommands.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <shared_mutex>

namespace gl2d {
    struct Texture;
}

class AssetManager {
private:
    std::vector<gl2d::Texture> m_textures;
    std::unordered_map<std::string, uint32_t> m_pathCache;
    std::shared_mutex m_rwMutex;

public:
    AssetManager();
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    TextureHandle loadTexture(const std::string& path, bool stream = false);
    gl2d::Texture getTexture(TextureHandle handle);
};