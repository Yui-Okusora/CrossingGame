#pragma once
#include "../System/Window.hpp"
#include "../System/AudioEngine.hpp"
#include "../Graphics/RenderStream.hpp"
#include "../System/EventSystem.hpp"
#include "../Graphics/AssetManager.hpp"
#include "../Physics/CollisionEngine.hpp"
#include "../UI/UIContext.hpp"
#include "../Utils/Utils.hpp"
#include <atomic>
#include <any>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>

class Blackboard {
private:
    std::unordered_map<std::string, std::any> m_registry;
    mutable std::shared_mutex m_mutex;

public:
    Blackboard() = default;

    template<typename T>
    void set(const std::string& key, T&& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_registry[key] = std::make_any<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    template<typename T>
    std::optional<T> get(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_registry.find(key);
        if (it == m_registry.end()) return std::nullopt;
        try {
            return std::any_cast<T>(it->second);
        }
        catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }

    bool has(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_registry.find(key) != m_registry.end();
    }

    void remove(const std::string& key) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_registry.erase(key);
    }
};

class LayerStack;

struct InputState {
    uint64_t keys_mask[6] = { 0 };
    uint64_t keys_mask_prev[6] = { 0 }; // History buffer for edge tracking

    uint8_t mouse_mask = 0;
    uint8_t mouse_mask_prev = 0;

    glm::vec2 mousePos{ 0.0f, 0.0f };
    glm::vec2 scrollDelta{ 0.0f, 0.0f }; // Transient offset accumulated per frame

    uint32_t unicode_queue[16] = { 0 };
    uint8_t unicode_count = 0;

    // --- CONTINUOUS STATE ACTIONS ---
    [[nodiscard]] inline bool isKeyHeld(int key) const noexcept {
        if (key < 0 || key >= 384) return false;
        return (keys_mask[key >> 6] & (1ULL << (key & 63))) != 0;
    }

    [[nodiscard]] inline bool isMouseButtonHeld(int button) const noexcept {
        if (button < 0 || button >= 8) return false;
        return (mouse_mask & (1 << button)) != 0;
    }

    // --- EDGE-TRIGGERED TRANSITION ACTIONS ---
    [[nodiscard]] inline bool isKeyJustPressed(int key) const noexcept {
        if (key < 0 || key >= 384) return false;
        bool current = (keys_mask[key >> 6] & (1ULL << (key & 63))) != 0;
        bool previous = (keys_mask_prev[key >> 6] & (1ULL << (key & 63))) != 0;
        return current && !previous;
    }

    [[nodiscard]] inline bool isKeyJustReleased(int key) const noexcept {
        if (key < 0 || key >= 384) return false;
        bool current = (keys_mask[key >> 6] & (1ULL << (key & 63))) != 0;
        bool previous = (keys_mask_prev[key >> 6] & (1ULL << (key & 63))) != 0;
        return !current && previous;
    }

    [[nodiscard]] inline bool isMouseButtonJustPressed(int button) const noexcept {
        if (button < 0 || button >= 8) return false;
        bool current = (mouse_mask & (1 << button)) != 0;
        bool previous = (mouse_mask_prev & (1 << button)) != 0;
        return current && !previous;
    }

    // Returns true for exactly ONE simulation frame tick when released
    [[nodiscard]] inline bool isMouseButtonJustReleased(int button) const noexcept {
        if (button < 0 || button >= 8) return false;
        bool current = (mouse_mask & (1 << button)) != 0;
        bool previous = (mouse_mask_prev & (1 << button)) != 0;
        return !current && previous;
    }

    [[nodiscard]] inline glm::vec2 getMousePosition() const noexcept { return mousePos; }
    [[nodiscard]] inline glm::vec2 getScrollDelta() const noexcept { return scrollDelta; }

    // Manages history tracking shifts right before processing new frame events
    inline void advance_frame_history() noexcept {
        for (int i = 0; i < 6; ++i) {
            keys_mask_prev[i] = keys_mask[i];
        }
        mouse_mask_prev = mouse_mask;
        scrollDelta = glm::vec2(0.0f, 0.0f); // Flush scroll accumulation back to zero
        unicode_count = 0;
    }
};

class EngineContext {
public:
    std::shared_ptr<Window> window = nullptr;
    std::atomic<bool> isRunning{ true };

    EventSystem eventSystem;
    AssetManager assetManager;
    AudioEngine audioEngine;
    Blackboard blackboard;
    InputState input;
    CollisionEngine collisionWorld;
    UIContext ui;
    TripleBuffer<RenderData> renderBuffer;

    ViewportScale currentViewport;

    std::atomic<float> scaleFactorX{ 1.0f };
    std::atomic<float> scaleFactorY{ 1.0f };
    std::atomic<float> fbWidth{ 1200.0f };
    std::atomic<float> fbHeight{ 805.0f };
    std::atomic<float> physicsAlpha{ 0.0f };

    glm::vec2 cameraPos{ 0.0f, 0.0f };

    std::chrono::time_point<std::chrono::steady_clock> startTimePoint;

    gl2d::Font globalFont;
    std::unique_ptr<LayerStack> layerStack;

    EngineContext();
    ~EngineContext();

    [[nodiscard]] inline double getTime() const noexcept {
        auto currentTimePoint = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(currentTimePoint - startTimePoint).count();
    }
};