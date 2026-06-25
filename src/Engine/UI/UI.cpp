#include "UI.hpp"
#include "UIContext.hpp"
#include "../Context/EngineContext.hpp"
#include "../Physics/CollisionEngine.hpp"
#include <algorithm>

UIState UI::Button(EngineContext* ctx, uint32_t id, const glm::vec4& bounds, bool enabled) {
    if (!enabled) return UIState{ .hovered = false, .pressed = false, .clicked = false, .focused = false };

    ctx->collisionWorld.register_collider(id, bounds, Layer_UI);
    return UIState{ .hovered = ctx->ui.is_hot(id), .pressed = ctx->ui.is_active(id), .clicked = ctx->ui.is_clicked(id) };
}

UIState UI::Slider(EngineContext* ctx, uint32_t id, const glm::vec4& trackBounds, float& outValue, bool enabled) {
    if (!enabled) return UIState{ .hovered = false, .pressed = false, .clicked = false, .focused = false };

    ctx->collisionWorld.register_collider(id, trackBounds, Layer_UI);

    if (ctx->ui.is_active(id)) {
        float relativeX = ctx->input.getMousePosition().x - trackBounds.x;
        outValue = std::clamp(relativeX / trackBounds.z, 0.0f, 1.0f);
    }
    return UIState{ .hovered = ctx->ui.is_hot(id), .pressed = ctx->ui.is_active(id) };
}

UIState UI::TextBox(EngineContext* ctx, uint32_t id, const glm::vec4& bounds, std::string& targetString, uint32_t& outCursor, bool enabled) {
    if (!enabled) return UIState{ .hovered = false, .pressed = false, .clicked = false, .focused = false };

    ctx->collisionWorld.register_collider(id, bounds, Layer_UI);
    bool focused = ctx->ui.is_focused(id);

    if (focused) {
        if (targetString == "Type Here...") { targetString.clear(); outCursor = 0; }

        for (uint8_t i = 0; i < ctx->input.unicode_count; ++i) {
            uint32_t cp = ctx->input.unicode_queue[i];
            if (cp >= 32 && cp <= 126 && targetString.size() < 30) {
                targetString.insert(targetString.begin() + outCursor++, static_cast<char>(cp));
            }
        }
        if (ctx->input.isKeyJustPressed(GLFW_KEY_BACKSPACE) && outCursor > 0) {
            targetString.erase(targetString.begin() + (--outCursor));
        }
    }
    return UIState{ .hovered = ctx->ui.is_hot(id), .focused = focused };
}