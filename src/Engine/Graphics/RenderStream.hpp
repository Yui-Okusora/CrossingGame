#pragma once
#include <vector>
#include <mutex>
#include <cstdint>
#include <glm/glm.hpp>
#include "../Utils/Utils.hpp"

class EngineContext;
namespace gl2d { struct Renderer2D; }

struct TextureHandle { uint32_t id = 0; }; 

using RenderExecutionFn = void(*)(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx); 

struct RenderCommand {
    int32_t stack_index;
    int32_t layer_depth; 
    uint32_t sorting_key; 
    uint32_t payload_offset; 
    RenderExecutionFn execute; 
};

struct RenderData {
    std::vector<RenderCommand> commands; 
    std::vector<uint8_t> payload_arena; 
    float physicsAlpha = 0.0f;
    int32_t current_stack_index = 0;

    RenderData();
    ~RenderData();
    void reset(); 

    template<typename T>
    void push_command(int32_t depth, uint32_t sorting_key, T&& payloadInstance) {
        using CleanType = std::remove_cvref_t<T>;
        size_t current_offset = payload_arena.size(); 
        size_t alignment = alignof(CleanType); 

        uintptr_t base_address = reinterpret_cast<uintptr_t>(payload_arena.data()); 
        uintptr_t current_address = base_address + current_offset; 

        size_t padding = (alignment - (current_address % alignment)) % alignment; 
        current_offset += padding; 

        payload_arena.resize(current_offset + sizeof(CleanType)); 

        CleanType* allocated_space = reinterpret_cast<CleanType*>(&payload_arena[current_offset]); 
        new (allocated_space) CleanType(std::forward<T>(payloadInstance)); 

        commands.push_back(RenderCommand{
            current_stack_index,
            depth,
            sorting_key,
            static_cast<uint32_t>(current_offset),
            &CleanType::Execute
            });
    }
};

// Primitive Composed Command Packets 
struct ClearScreenPayload {
    glm::vec4 color;
    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx); 
};

struct CameraPayload {
    glm::vec2 position;
    float zoom;
    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx); 
};

struct RectPayload {
    glm::vec4 dest_rect;
    glm::vec4 color;
    TextureHandle texture;
    glm::uvec2 atlas_dimensions{ 1, 1 }; 
    glm::uvec2 atlas_pos{ 0, 0 }; 
    glm::vec2 origin{ 0.0f, 0.0f }; 
    float rotation = 0.0f; 
    bool flip_x = false; 
    bool no_texture = true;
    bool is_world_space = false;

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx); 
};

struct LinePayload {
    glm::vec2 start;
    glm::vec2 end;
    glm::vec4 color;
    float thickness;
    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx); 
};

struct TextPayload {
    glm::vec2 position = {};
    glm::vec4 color = {};
    float scale = 24.0f; 
    float spacing = 4.0f; 
    float line_spacePixels = 3.0f; 
    bool showInCenter = false; 
    char text_content[128] = { 0 }; 

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx); 
};

// Triple Buffer Lockless Sync Framework 
template<typename T>
class TripleBuffer {
private:
    T m_buffers[3]; 
        uint32_t m_logicIdx = 0; 
        uint32_t m_renderIdx = 1; 
        uint32_t m_sharedIdx = 2; 
        bool m_hasNewData = false; 
        std::mutex m_exchangeMutex; 

public:
    T& getWriteBuffer() noexcept { return m_buffers[m_logicIdx]; }

        T& getReadBuffer() noexcept {
        std::unique_lock<std::mutex> lock(m_exchangeMutex); 
            if (m_hasNewData) {
                
                std::swap(m_renderIdx, m_sharedIdx); 
                    m_hasNewData = false; 
            }
        return m_buffers[m_renderIdx]; 
    }

    void swap() noexcept {
        std::unique_lock<std::mutex> lock(m_exchangeMutex); 
            std::swap(m_logicIdx, m_sharedIdx); 
            m_hasNewData = true; 
    }
};