#pragma once
#include <cstdint>

class EngineContext;

// Lightweight control state packet returned to the user
struct UIState {
    bool hovered = false; // Cursor is over the widget
    bool pressed = false; // Left mouse is holding down on the widget
    bool clicked = false; // Left mouse was cleanly released on the widget this frame
    bool focused = false; // Widget has active text/keyboard focus
};

class UIContext {
public:
    uint32_t hot_id = 0;
    uint32_t active_id = 0;
    uint32_t focus_id = 0;
    uint32_t clicked_id = 0;

    void update_system_states(EngineContext* ctx);

    [[nodiscard]] inline bool is_hot(uint32_t id) const noexcept { return id != 0 && hot_id == id; }
    [[nodiscard]] inline bool is_active(uint32_t id) const noexcept { return id != 0 && active_id == id; }
    [[nodiscard]] inline bool is_focused(uint32_t id) const noexcept { return id != 0 && focus_id == id; }
    [[nodiscard]] inline bool is_clicked(uint32_t id) const noexcept { return id != 0 && clicked_id == id; }
};