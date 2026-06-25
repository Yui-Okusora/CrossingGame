#pragma once
#include <vector>
#include <atomic>
#include <cstdint>
#include <thread>
#include <condition_variable>

class EngineContext;
namespace gl2d { struct Renderer2D; }

struct TextureHandle { uint32_t id = 0; };

// Function pointer signature for stateless backend execution delegates
using RenderExecutionFn = void(*)(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx);

struct RenderCommand {
    int32_t layer_depth;         // Primary sort: Layer visibility stacking
    uint32_t sorting_key;        // Secondary sort: Material/Texture optimization key
    RenderExecutionFn execute;   // Stateless rendering function delegate
    uint32_t payload_offset;     // Offset position inside the flat byte arena buffer
};

struct RenderData {
    std::vector<RenderCommand> commands;
    std::vector<uint8_t> payload_arena; // Flat, contiguous variable-sized memory pool

    RenderData();
    ~RenderData();

    void reset();

    // ============================================================================
    // OVERLOAD 1: AUTOMATIC EXECUTION DEDUCTION (Cleanest API)
    // Passes the pre-initialized structure instance directly and deduces &T::Execute
    // ============================================================================
    template<typename T>
    void push_command(int32_t depth, uint32_t sorting_key, T&& payloadInstance) {
        using CleanType = std::remove_cvref_t<T>;

        size_t current_offset = payload_arena.size();
        size_t alignment = alignof(CleanType);

        // Calculate alignment relative to the absolute memory address of the vector's heap space
        uintptr_t base_address = reinterpret_cast<uintptr_t>(payload_arena.data());
        uintptr_t current_address = base_address + current_offset;

        size_t padding = (alignment - (current_address % alignment)) % alignment;
        current_offset += padding;

        size_t payload_size = sizeof(CleanType);
        payload_arena.resize(current_offset + payload_size);

        // Re-evaluate pointer since resize() can trigger vector reallocations
        CleanType* allocated_space = reinterpret_cast<CleanType*>(&payload_arena[current_offset]);
        new (allocated_space) CleanType(std::forward<T>(payloadInstance));

        RenderCommand cmd;
        cmd.layer_depth = depth;
        cmd.sorting_key = sorting_key;
        cmd.execute = &CleanType::Execute;
        cmd.payload_offset = static_cast<uint32_t>(current_offset);

        commands.push_back(cmd);
    }

    // ============================================================================
    // OVERLOAD 2: EXPLICIT DELEGATE OVERRIDE
    // Permits passing a custom function execution callback pointer if needed
    // ============================================================================
    template<typename T>
    void push_command(int32_t depth, uint32_t sorting_key, RenderExecutionFn execute, T&& payloadInstance) {
        using CleanType = std::remove_cvref_t<T>;

        size_t current_offset = payload_arena.size();
        size_t alignment = alignof(CleanType);

        // Calculate alignment relative to the absolute memory address of the vector's heap space
        uintptr_t base_address = reinterpret_cast<uintptr_t>(payload_arena.data());
        uintptr_t current_address = base_address + current_offset;

        size_t padding = (alignment - (current_address % alignment)) % alignment;
        current_offset += padding;

        size_t payload_size = sizeof(CleanType);
        payload_arena.resize(current_offset + payload_size);

        // Re-evaluate pointer since resize() can trigger vector reallocations
        CleanType* allocated_space = reinterpret_cast<CleanType*>(&payload_arena[current_offset]);
        new (allocated_space) CleanType(std::forward<T>(payloadInstance));

        RenderCommand cmd;
        cmd.layer_depth = depth;
        cmd.sorting_key = sorting_key;
        cmd.execute = execute;
        cmd.payload_offset = static_cast<uint32_t>(current_offset);

        commands.push_back(cmd);
    }
};

template<typename T>
class DoubleBuffer {
private:
    T m_buffers[2];
    uint32_t m_writeIndex{ 0 };
    bool m_isRenderReading{ false };

    std::mutex m_syncMutex;
    std::condition_variable m_frameCondition;

public:
    T& getWriteBuffer() noexcept {
        // Safe lockless write allocation target retrieval
        return m_buffers[m_writeIndex];
    }

    T& getReadBuffer() noexcept {
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_isRenderReading = true;
        return m_buffers[1 - m_writeIndex];
    }

    void releaseReadBuffer() noexcept {
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_isRenderReading = false;
        // Notify the logic thread that the render buffer is now completely vacant!
        m_frameCondition.notify_one();
    }

    void swap() noexcept {
        std::unique_lock<std::mutex> lock(m_syncMutex);

        // FIXED: Instead of an unpredictable spin-yield loop that causes frame lapping,
        // block the simulation thread gracefully until the render thread releases the front buffer.
        m_frameCondition.wait(lock, [this]() {
            return !m_isRenderReading;
            });

        // Safe structural index inversion
        m_writeIndex = 1 - m_writeIndex;
    }
};