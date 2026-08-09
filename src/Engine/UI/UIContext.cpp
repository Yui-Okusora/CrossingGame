#include "UIContext.hpp"
#include "../Context/EngineContext.hpp"
#include "../Physics/CollisionEngine.hpp"
#include "../Graphics/RenderStream.hpp"
#include "../Utils/Utils.hpp"
#include <algorithm>
#include <vector>

void UIContext::update_system_states(EngineContext* ctx) {
    clicked_id = 0;
    static std::vector<CollisionResult> scratchpad;
    scratchpad.clear();

    const auto& vp = ctx->currentViewport;

    glm::vec2 rawMouse = ctx->input.getMousePosition();
    glm::vec2 virtualMouse{
        (rawMouse.x - vp.offset.x) / vp.scale,
        (rawMouse.y - vp.offset.y) / vp.scale
    };

    ctx->collisionWorld.query_point(virtualMouse, Layer_None, Layer_UI, scratchpad);

    // Pick the top-most registered collider (last element added)
    hot_id = scratchpad.empty() ? 0 : scratchpad.back().targetId;

    if (ctx->input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        active_id = hot_id;
        if (hot_id != focus_id) focus_id = 0;
    }

    if (ctx->input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
        if (active_id != 0 && active_id == hot_id) {
            clicked_id = active_id;
            focus_id = active_id;
        }
        active_id = 0;
    }
}

bool UIContext::Button(RenderData& writeBuffer, EngineContext* ctx, uint32_t id, const glm::vec4& bounds, const char* label, float textScale) {
    // Only register UI colliders if input is enabled for this layer
    if (input_enabled) {
        ctx->collisionWorld.register_collider(id, bounds, Layer_UI);
    }

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };

    glm::vec4 activeColor = state.pressed ? glm::vec4{ 0.1f, 0.45f, 0.75f, 1.0f } :
        state.hovered ? glm::vec4{ 0.25f, 0.28f, 0.35f, 1.0f } : glm::vec4{ 0.18f, 0.19f, 0.22f, 1.0f };

    writeBuffer.push_command(900, 0, RectPayload{ .dest_rect = bounds, .color = activeColor, .no_texture = true });

    if (label && label[0] != '\0') {
        glm::vec2 centerPoint{ bounds.x + (bounds.z * 0.5f), bounds.y + (bounds.w * 0.5f) };
        TextPayload txt{ .position = centerPoint, .color = {1, 1, 1, 1}, .scale = textScale, .showInCenter = true };
        snprintf(txt.text_content, sizeof(txt.text_content), "%s", label);
        writeBuffer.push_command(910, 0, txt);
    }

    if (state.clicked) {
        clicked_id = 0; // Consume click event so lower layers cannot receive it
    }

    return state.clicked;
}

bool UIContext::Slider(RenderData& writeBuffer, EngineContext* ctx, uint32_t id, const glm::vec4& trackBounds, float& value) {
    if (input_enabled) {
        ctx->collisionWorld.register_collider(id, trackBounds, Layer_UI);
    }

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };

    if (state.pressed) {
        float relativeX = ctx->input.getMousePosition().x - trackBounds.x;
        value = std::clamp(relativeX / trackBounds.z, 0.0f, 1.0f);
    }

    writeBuffer.push_command(900, 0, RectPayload{ .dest_rect = trackBounds, .color = {0.06f, 0.06f, 0.08f, 1.0f}, .no_texture = true });

    float knobX = trackBounds.x + (trackBounds.z * value) - 8.0f;
    float knobY = trackBounds.y + (trackBounds.w * 0.5f) - 15.0f;
    glm::vec4 knobColor = state.pressed ? glm::vec4{ 0.1f, 0.45f, 0.75f, 1.0f } : glm::vec4{ 0.45f, 0.45f, 0.5f, 1.0f };

    writeBuffer.push_command(910, 0, RectPayload{ .dest_rect = {knobX, knobY, 16.0f, 30.0f}, .color = knobColor, .no_texture = true });
    return state.pressed;
}

void UIContext::TextBox(RenderData& writeBuffer, EngineContext* ctx, uint32_t id, const glm::vec4& bounds, std::string& text, uint32_t& cursor, float textScale) {
    if (input_enabled) {
        ctx->collisionWorld.register_collider(id, bounds, Layer_UI);
    }

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };

    static uint32_t lastFocusedId = 0;
    static double nextBackspaceTime = 0.0;

    if (focus_id != lastFocusedId) {
        lastFocusedId = focus_id;
        nextBackspaceTime = 0.0;
    }

    if (state.focused) {
        if (text == "Type Here...") { text.clear(); cursor = 0; }
        for (uint8_t i = 0; i < ctx->input.unicode_count; ++i) {
            uint32_t cp = ctx->input.unicode_queue[i];
            if (cp >= 32 && cp <= 126 && text.size() < 30) {
                text.insert(text.begin() + cursor++, static_cast<char>(cp));
            }
        }

        constexpr double INITIAL_DELAY = 0.40;
        constexpr double REPEAT_RATE = 0.04;

        if (ctx->input.isKeyJustPressed(GLFW_KEY_BACKSPACE)) {
            if (cursor > 0) {
                text.erase(text.begin() + (--cursor));
            }
            nextBackspaceTime = ctx->getTime() + INITIAL_DELAY;
        }
        else if (ctx->input.isKeyHeld(GLFW_KEY_BACKSPACE)) {
            double currentTime = ctx->getTime();
            if (currentTime >= nextBackspaceTime) {
                if (cursor > 0) {
                    text.erase(text.begin() + (--cursor));
                }
                nextBackspaceTime = currentTime + REPEAT_RATE;
            }
        }
    }

    glm::vec4 borderColor = state.focused ? glm::vec4{ 0.1f, 0.45f, 0.75f, 1.0f } : glm::vec4{ 0.2f, 0.22f, 0.26f, 1.0f };
    writeBuffer.push_command(900, 0, RectPayload{ .dest_rect = bounds, .color = {0.02f, 0.02f, 0.03f, 1.0f}, .no_texture = true });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x, bounds.y}, {bounds.x + bounds.z, bounds.y}, borderColor, 2.0f });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x, bounds.y}, {bounds.x, bounds.y + bounds.w}, borderColor, 2.0f });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x + bounds.z, bounds.y}, {bounds.x + bounds.z, bounds.y + bounds.w}, borderColor, 2.0f });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x, bounds.y + bounds.w}, {bounds.x + bounds.z, bounds.y + bounds.w}, borderColor, 2.0f });

    float baselineY = bounds.y + bounds.w - ((bounds.w - textScale) * 0.5f);
    glm::vec2 textPos{ bounds.x + 12.0f, baselineY };

    TextPayload txt{ .position = textPos, .color = {0.95f, 0.95f, 1.0f, 1.0f}, .scale = textScale };
    snprintf(txt.text_content, sizeof(txt.text_content), "%s", text.c_str());
    writeBuffer.push_command(910, 0, txt);

    if (state.focused && ctx->globalFont.packedCharsBuffer) {
        const auto* packedChars = static_cast<const stbtt_packedchar*>(ctx->globalFont.packedCharsBuffer);

        constexpr float baseBakedFontHeight = 96.0f;
        float fontScale = textScale / baseBakedFontHeight;
        constexpr float fontSpacing = 4.0f;

        float cursorOffset = 0.0f;
        size_t sampleLength = std::min<size_t>(cursor, text.size());

        for (size_t i = 0; i < sampleLength; ++i) {
            char c = text[i];
            if (c == ' ') {
                float spaceAdvance = (ctx->globalFont.spaceSize > 0.0f) ? ctx->globalFont.spaceSize : packedChars[0].xadvance;
                cursorOffset += (spaceAdvance * fontScale) + fontSpacing;
            }
            else if (c >= ' ' && c <= '~') {
                float advance = packedChars[c - 32].xadvance;
                cursorOffset += (advance * fontScale) + fontSpacing;
            }
        }

        float cursorX = textPos.x + cursorOffset;

        if (static_cast<int>(ctx->getTime() * 4.0) % 2 == 0) {
            writeBuffer.push_command(920, 0, LinePayload{
                {cursorX, baselineY + 2.0f},
                {cursorX, baselineY - textScale + 2.0f},
                {0.1f, 0.45f, 0.75f, 1.0f},
                2.0f
                });
        }
    }
}