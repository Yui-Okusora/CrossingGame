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
    if (input_enabled) ctx->collisionWorld.register_collider(id, bounds, Layer_UI);

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };
    glm::vec4 activeColor = state.pressed ? glm::vec4{ 0.1f, 0.45f, 0.75f, 1.0f } :
        state.hovered ? glm::vec4{ 0.25f, 0.28f, 0.35f, 1.0f } : glm::vec4{ 0.18f, 0.19f, 0.22f, 1.0f };

    writeBuffer.push_command(900, 0, RectPayload{
        .dest_rect = bounds,
        .color = activeColor,
        .no_texture = true,
        .is_world_space = false
        });

    if (label && label[0] != '\0') {
        glm::vec2 centerPoint{ bounds.x + (bounds.z * 0.5f), bounds.y + (bounds.w * 0.5f) };
        TextPayload txt{
            .position = centerPoint,
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = textScale,
            .showInCenter = true
        };
        snprintf(txt.text_content, sizeof(txt.text_content), "%s", label);
        writeBuffer.push_command(910, 0, txt);
    }

    if (state.clicked) {
        if (m_clickSFX.id == 0) {
            m_clickSFX = ctx->audioEngine.loadSound(SFX_PATH "button_click.wav");
        }
        ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX);
        clicked_id = 0;
    }
    return state.clicked;
}

bool UIContext::TexturedButton(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
    const glm::vec4& bounds, TextureHandle texture, const char* label, float textScale) {
    return TexturedButton(writeBuffer, ctx, id, bounds, texture, glm::uvec2{ 1, 1 }, label, textScale);
}

bool UIContext::TexturedButton(RenderData& writeBuffer, EngineContext* ctx, uint32_t id,
    const glm::vec4& bounds, TextureHandle texture, glm::uvec2 atlasDims, const char* label, float textScale) {
    if (input_enabled) ctx->collisionWorld.register_collider(id, bounds, Layer_UI);

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };

    uint32_t col = (atlasDims.x > 1 && (state.hovered || state.pressed)) ? 1 : 0;
    glm::vec4 tint = state.pressed ? glm::vec4{ 0.85f, 0.85f, 0.85f, 1.0f } : glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };

    writeBuffer.push_command(900, 0, RectPayload{
        .dest_rect = bounds,
        .color = tint,
        .texture = texture,
        .atlas_dimensions = atlasDims,
        .atlas_pos = { col, 0 },
        .no_texture = false,
        .is_world_space = false
        });

    if (label && label[0] != '\0') {
        glm::vec2 centerPoint{ bounds.x + (bounds.z * 0.5f), bounds.y + (bounds.w * 0.5f) };
        TextPayload txt{
            .position = centerPoint,
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .scale = textScale,
            .showInCenter = true
        };
        snprintf(txt.text_content, sizeof(txt.text_content), "%s", label);
        writeBuffer.push_command(910, 0, txt);
    }

    if (state.clicked) {
        if (m_clickSFX.id == 0) {
            m_clickSFX = ctx->audioEngine.loadSound(SFX_PATH "button_click.wav");
        }
        ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX);
        clicked_id = 0;
    }
    return state.clicked;
}

bool UIContext::Slider(RenderData& writeBuffer, EngineContext* ctx, uint32_t id, const glm::vec4& trackBounds, float& value) {
    if (input_enabled) {
        glm::vec4 hitBounds{ trackBounds.x, trackBounds.y - 8.0f, trackBounds.z, trackBounds.w + 16.0f };
        ctx->collisionWorld.register_collider(id, hitBounds, Layer_UI);
    }

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };

    if (state.pressed) {
        const auto& vp = ctx->currentViewport;
        glm::vec2 virtualMouse{
            (ctx->input.getMousePosition().x - vp.offset.x) / vp.scale,
            (ctx->input.getMousePosition().y - vp.offset.y) / vp.scale
        };
        float relativeX = virtualMouse.x - trackBounds.x;
        value = std::clamp(relativeX / trackBounds.z, 0.0f, 1.0f);
    }

    writeBuffer.push_command(900, 0, RectPayload{
        .dest_rect = trackBounds,
        .color = { 0.05f, 0.06f, 0.08f, 1.0f },
        .no_texture = true,
        .is_world_space = false
        });

    if (value > 0.005f) {
        writeBuffer.push_command(905, 0, RectPayload{
            .dest_rect = { trackBounds.x, trackBounds.y, trackBounds.z * value, trackBounds.w },
            .color = { 0.95f, 0.65f, 0.15f, 1.0f },
            .no_texture = true,
            .is_world_space = false
            });
    }

    float knobWidth = 14.0f;
    float knobHeight = trackBounds.w + 12.0f;
    float knobX = trackBounds.x + (trackBounds.z * value) - (knobWidth * 0.5f);
    float knobY = trackBounds.y - 6.0f;

    glm::vec4 knobColor = state.pressed ? glm::vec4{ 1.0f, 0.85f, 0.3f, 1.0f }
        : state.hovered ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }
    : glm::vec4{ 0.82f, 0.85f, 0.9f, 1.0f };

    writeBuffer.push_command(910, 0, RectPayload{
        .dest_rect = { knobX, knobY, knobWidth, knobHeight },
        .color = knobColor,
        .no_texture = true,
        .is_world_space = false
        });

    return state.pressed;
}

void UIContext::TextBox(RenderData& writeBuffer, EngineContext* ctx, uint32_t id, const glm::vec4& bounds, std::string& text, uint32_t& cursor, float textScale) {
    if (input_enabled) ctx->collisionWorld.register_collider(id, bounds, Layer_UI);

    UIState state{ is_hot(id), is_active(id), is_clicked(id), is_focused(id) };
    static uint32_t lastFocusedId = 0;
    static double nextBackspaceTime = 0.0;

    if (focus_id != lastFocusedId) {
        lastFocusedId = focus_id;
        nextBackspaceTime = 0.0;
    }

    if (state.focused) {
        if (text == "Student Name" || text == "Type Here...") { text.clear(); cursor = 0; }
        for (uint8_t i = 0; i < ctx->input.unicode_count; ++i) {
            uint32_t cp = ctx->input.unicode_queue[i];
            if (cp >= 32 && cp <= 126 && text.size() < 30) {
                text.insert(text.begin() + cursor++, static_cast<char>(cp));
            }
        }

        if (ctx->input.isKeyJustPressed(GLFW_KEY_BACKSPACE)) {
            if (cursor > 0) text.erase(text.begin() + (--cursor));
            nextBackspaceTime = ctx->getTime() + 0.40;
        }
        else if (ctx->input.isKeyHeld(GLFW_KEY_BACKSPACE)) {
            if (ctx->getTime() >= nextBackspaceTime) {
                if (cursor > 0) text.erase(text.begin() + (--cursor));
                nextBackspaceTime = ctx->getTime() + 0.04;
            }
        }
    }

    glm::vec4 borderColor = state.focused ? glm::vec4{ 0.1f, 0.45f, 0.75f, 1.0f } : glm::vec4{ 0.2f, 0.22f, 0.26f, 1.0f };
    writeBuffer.push_command(900, 0, RectPayload{
        .dest_rect = bounds,
        .color = { 0.02f, 0.02f, 0.03f, 0.9f },
        .no_texture = true,
        .is_world_space = false
        });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x, bounds.y}, {bounds.x + bounds.z, bounds.y}, borderColor, 2.0f });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x, bounds.y}, {bounds.x, bounds.y + bounds.w}, borderColor, 2.0f });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x + bounds.z, bounds.y}, {bounds.x + bounds.z, bounds.y + bounds.w}, borderColor, 2.0f });
    writeBuffer.push_command(905, 0, LinePayload{ {bounds.x, bounds.y + bounds.w}, {bounds.x + bounds.z, bounds.y + bounds.w}, borderColor, 2.0f });

    float baselineY = bounds.y + bounds.w - ((bounds.w - textScale) * 0.5f);
    glm::vec2 textPos{ bounds.x + 12.0f, baselineY };

    TextPayload txt{
        .position = textPos,
        .color = { 0.95f, 0.95f, 1.0f, 1.0f },
        .scale = textScale
    };
    snprintf(txt.text_content, sizeof(txt.text_content), "%s", text.c_str());
    writeBuffer.push_command(910, 0, txt);
}