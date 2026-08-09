#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

// Forward declarations to break circular header dependencies safely
class EngineContext;
struct RenderData;

// Lightweight structural data packet returned to interface queries 
struct UIState {
    bool hovered = false;  // Cursor is positioned over the component 
    bool pressed = false;  // Left mouse click is currently held down on the component 
    bool clicked = false;  // Left mouse click was cleanly released inside the component boundaries this frame 
    bool focused = false;  // Component currently holds character keyboard typing focus 
};

class UIContext {
public:
    uint32_t hot_id = 0;     // Primitive component index directly under the pointer 
    uint32_t active_id = 0;  // Primitive component index targeted by a mouse down event 
    uint32_t focus_id = 0;   // Primitive component index capturing text input focus 
    uint32_t clicked_id = 0; // Primitive component index registering a valid action click trigger this frame 

    bool input_enabled = true;

    // Core internal system lifecycles
    void update_system_states(EngineContext* ctx); 


    // ============================================================================
    // DECLARATIVE IMMEDIATE-MODE UI METHODS
    // ============================================================================
    bool Button(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& bounds, const char* label, float textScale = 24.0f);

    bool Slider(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& trackBounds, float& value);

    void TextBox(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
        const glm::vec4& bounds, std::string& text, uint32_t& cursor, float textScale = 24.0f);

    // Inline boolean property access evaluation states 
    [[nodiscard]] inline bool is_hot(uint32_t id) const noexcept { return id != 0 && hot_id == id; }
    [[nodiscard]] inline bool is_active(uint32_t id) const noexcept { return id != 0 && active_id == id; }
    [[nodiscard]] inline bool is_focused(uint32_t id) const noexcept { return id != 0 && focus_id == id; }
    [[nodiscard]] inline bool is_clicked(uint32_t id) const noexcept { return id != 0 && clicked_id == id; }
};