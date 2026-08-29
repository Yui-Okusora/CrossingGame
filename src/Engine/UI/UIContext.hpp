#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>
#include "../Graphics/RenderStream.hpp"

class EngineContext;
struct RenderData;

struct UIState {
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;
    bool focused = false;
};

class UIContext {
public:
    uint32_t hot_id = 0;
    uint32_t active_id = 0;
    uint32_t focus_id = 0;
    uint32_t clicked_id = 0;

    bool input_enabled = true;

    void update_system_states(EngineContext* ctx);

    bool Button(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& bounds, const char* label, float textScale = 24.0f);

    bool TexturedButton(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& bounds, TextureHandle texture, const char* label = nullptr, float textScale = 20.0f);

    bool TexturedButton(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& bounds, TextureHandle texture, glm::uvec2 atlasDims, const char* label = nullptr, float textScale = 20.0f);

    // Clean plain-color slider with responsive track groove, fill, and interactive thumb
    bool Slider(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& trackBounds, float& value);

    void TextBox(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& bounds, std::string& text, uint32_t& cursor, float textScale = 24.0f);

    [[nodiscard]] inline bool is_hot(uint32_t id) const noexcept { return id != 0 && hot_id == id; }
    [[nodiscard]] inline bool is_active(uint32_t id) const noexcept { return id != 0 && active_id == id; }
    [[nodiscard]] inline bool is_focused(uint32_t id) const noexcept { return id != 0 && focus_id == id; }
    [[nodiscard]] inline bool is_clicked(uint32_t id) const noexcept { return id != 0 && clicked_id == id; }
};