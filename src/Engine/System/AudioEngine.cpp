#include "AudioEngine.hpp"
#include <stdexcept>
#include <algorithm>

AudioEngine::AudioEngine() {
    // Spin up miniaudio's internal high-priority device synchronization threads [cite: 79]
    if (ma_engine_init(nullptr, &m_engine) != MA_SUCCESS) {
        throw std::runtime_error("Miniaudio engine failed to initialize hardware layer context.");
    }

    // Allocate an empty fallback placeholder for the null handle at index 0
    m_sounds.push_back(nullptr);
    m_soundCategories.push_back(AudioCategory::Master);

    // Initialize default mixer volume baselines [cite: 79, 81]
    for (size_t i = 0; i < static_cast<size_t>(AudioCategory::Count); ++i) {
        m_categoryVolumes[i] = 1.0f;
    }
    m_categoryVolumes[static_cast<size_t>(AudioCategory::Music)] = 0.5f; // Match old configuration rules [cite: 13]
}

AudioEngine::~AudioEngine() {
    // Free all allocated runtime sound structures safely
    for (ma_sound* sound : m_sounds) {
        if (sound) {
            ma_sound_uninit(sound);
            delete sound;
        }
    }
    ma_engine_uninit(&m_engine); // Shut down audio threads [cite: 80]
}

AudioHandle AudioEngine::loadSound(const std::string& filePath, bool stream) {
    std::unique_lock<std::shared_mutex> lock(m_audioMutex);

    // Verify if the asset has already been loaded [cite: 84]
    auto it = m_pathCache.find(filePath);
    if (it != m_pathCache.end()) {
        return AudioHandle{ it->second };
    }

    ma_uint32 flags = stream ? MA_SOUND_FLAG_STREAM : 0; // Maintain streaming configurations [cite: 84]
    ma_sound* newSound = new ma_sound();

    if (ma_sound_init_from_file(&m_engine, filePath.c_str(), flags, nullptr, nullptr, newSound) != MA_SUCCESS) {
        delete newSound;
        return AudioHandle{ 0 }; // Fail gracefully by returning the silence fallback
    }

    uint32_t assignedId = static_cast<uint32_t>(m_sounds.size());
    m_sounds.push_back(newSound);
    m_soundCategories.push_back(AudioCategory::GameplaySFX);
    m_pathCache[filePath] = assignedId;

    return AudioHandle{ assignedId };
}

void AudioEngine::play(AudioHandle handle, AudioCategory category, bool loop) {
    std::unique_lock<std::shared_mutex> lock(m_audioMutex);

    if (handle.id == 0 || handle.id >= m_sounds.size()) return;

    ma_sound* target = m_sounds[handle.id];
    if (!target) return;

    m_soundCategories[handle.id] = category;

    // Apply category volume scaling before firing playback
    float masterVol = m_categoryVolumes[static_cast<size_t>(AudioCategory::Master)];
    float finalVolume = m_categoryVolumes[static_cast<size_t>(category)];
    if (category != AudioCategory::Master) {
        finalVolume *= masterVol;
    }
    ma_sound_set_volume(target, finalVolume); // Native lockless mixer adjustment [cite: 95]
    ma_sound_set_looping(target, loop ? MA_TRUE : MA_FALSE);

    if (!ma_sound_is_playing(target)) {
        ma_sound_start(target); // Fires off audio mixing asynchronously [cite: 88]
    }
}

void AudioEngine::stop(AudioHandle handle) {
    std::shared_lock<std::shared_mutex> lock(m_audioMutex);

    if (handle.id == 0 || handle.id >= m_sounds.size()) return;

    ma_sound* target = m_sounds[handle.id];
    if (target && ma_sound_is_playing(target)) {
        ma_sound_stop(target); // Stop playback safely [cite: 90]
    }
}

bool AudioEngine::isPlaying(AudioHandle handle) {
    std::shared_lock<std::shared_mutex> lock(m_audioMutex);

    if (handle.id == 0 || handle.id >= m_sounds.size()) return false;
    ma_sound* target = m_sounds[handle.id];

    return target && ma_sound_is_playing(target); // [cite: 93]
}

void AudioEngine::setCategoryVolume(AudioCategory cat, float volume) {
    std::unique_lock<std::shared_mutex> lock(m_audioMutex);

    // 1. Update the internal state volume cache
    m_categoryVolumes[static_cast<size_t>(cat)] = std::clamp(volume, 0.0f, 1.0f);

    float masterVol = m_categoryVolumes[static_cast<size_t>(AudioCategory::Master)];

    // 2. Continuous Sweep Pass: Push live mixer adjustments directly to miniaudio
    for (size_t i = 0; i < m_sounds.size(); ++i) {
        ma_sound* target = m_sounds[i];
        if (!target) continue;

        AudioCategory soundCat = m_soundCategories[i];

        // If the updated category matches this sound, OR if Master volume changed, push update
        if (soundCat == cat || cat == AudioCategory::Master) {
            float finalVolume = m_categoryVolumes[static_cast<size_t>(soundCat)];

            // Apply Master scaling modifier to sub-categories
            if (soundCat != AudioCategory::Master) {
                finalVolume *= masterVol;
            }

            // Instantly adjusts the running mixing graph without stuttering or stopping playback
            ma_sound_set_volume(target, finalVolume);
        }
    }
}

float AudioEngine::getCategoryVolume(AudioCategory cat) const {
    return m_categoryVolumes[static_cast<size_t>(cat)]; // [cite: 78]
}