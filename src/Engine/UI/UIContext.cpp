#include "UIContext.hpp"
#include "../Context/EngineContext.hpp"
#include "../Physics/CollisionEngine.hpp"
#include <vector>

void UIContext::update_system_states(EngineContext* ctx) {
    clicked_id = 0; // Clear single-frame click flags

    // 1. Query the unified collision database using the cursor coordinates
    static std::vector<CollisionResult> scratchpad;
    scratchpad.clear();

    glm::vec2 mousePos = ctx->input.getMousePosition();
    ctx->collisionWorld.query_point(mousePos, Layer_None, Layer_UI, scratchpad);

    // 2. Determine the hot element under the mouse cursor
    hot_id = scratchpad.empty() ? 0 : scratchpad[0].target_id;

    // 3. Update active states based on mouse clicks
    if (ctx->input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        active_id = hot_id;
        if (hot_id != focus_id) {
            focus_id = 0; // Clear input focus if clicking empty space
        }
    }

    if (ctx->input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
        // Trigger condition matches if the user releases the click inside the same component
        if (active_id != 0 && active_id == hot_id) {
            clicked_id = active_id; // Register single-frame execution click
            focus_id = active_id;   // Assign keyboard typing capture focus
        }
        active_id = 0;
    }
}