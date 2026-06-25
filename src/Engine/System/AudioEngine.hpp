#pragma once
#include <miniaudio.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <shared_mutex>

enum class AudioCategory : uint8_t {
    Master,
    Music,
    GameplaySFX,
    InteractSFX,
    Count
};

struct AudioHandle {
    uint32_t id = 0; // 0 = Null/Silence fallback
};

class AudioEngine {
private:
    ma_engine m_engine;

    // Contiguous cache arrays for audio sources
    std::vector<ma_sound*> m_sounds;
    std::vector<AudioCategory> m_soundCategories;
    std::unordered_map<std::string, uint32_t> m_pathCache;

    // Shared mutex allows parallel reads across threads, locking only during an asset load
    std::shared_mutex m_audioMutex;

    float m_categoryVolumes[static_cast<size_t>(AudioCategory::Count)];

public:
    AudioEngine();
    ~AudioEngine();

    // Disable copying to safeguard raw native pointer structures
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Safe to invoke from any thread context
    AudioHandle loadSound(const std::string& filePath, bool stream = false);

    void play(AudioHandle handle, AudioCategory category, bool loop = false);
    void stop(AudioHandle handle);
    bool isPlaying(AudioHandle handle);

    void setCategoryVolume(AudioCategory cat, float volume);
    float getCategoryVolume(AudioCategory cat) const;
};