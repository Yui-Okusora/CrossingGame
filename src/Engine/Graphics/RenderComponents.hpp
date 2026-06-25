#pragma once
#include <glm/glm.hpp>
#include "../Context/EngineContext.hpp"

struct ClearScreenPayload {
    glm::vec4 color;

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const ClearScreenPayload*>(payload);
        renderer.clearScreen(data.color);
    }
};

struct CameraPayload {
    glm::vec2 position;
    float zoom;

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const CameraPayload*>(payload);

        // FIX: Pass the positions raw to match our 1:1 pixel coordinate space 
        renderer.currentCamera.position = data.position;
        renderer.currentCamera.zoom = data.zoom;
    }
};

struct RectPayload {
    glm::vec4 dest_rect;
    glm::vec4 color;
    TextureHandle texture;

    // Atlas Configurations (Dynamically driven by individual logic layers)
    glm::uvec2 atlas_dimensions; // e.g., { 15, 1 } or { 8, 8 }
    glm::uvec2 atlas_pos;        // Grid coordinates: { x, y }

    glm::vec2 origin;            // Center point of rotation/scaling transformation
    float rotation;              // Angle of rotation in radians or degrees

    bool flip_x;
    bool flip_y;
    bool no_texture;
    bool is_disabled;

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const RectPayload*>(payload);

        if (data.no_texture) {
            renderer.renderRectangle(data.dest_rect, data.color, data.origin, data.rotation);
        }
        else {
            gl2d::Texture tex = ctx->assetManager.getTexture(data.texture);

            // Dynamic atlas construction from payload definitions
            gl2d::TextureAtlas customAtlas(data.atlas_dimensions.x, data.atlas_dimensions.y);

            renderer.renderRectangle(
                data.dest_rect,
                tex,
                data.color,
                data.origin,
                data.rotation,
                customAtlas.get(data.atlas_pos.x, data.atlas_pos.y, data.flip_x)
            );
        }
    }
};

struct LinePayload {
    glm::vec2 start;
    glm::vec2 end;
    glm::vec4 color;
    float thickness;

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const LinePayload*>(payload);
        renderer.renderLine(data.start, data.end, data.color, data.thickness);
    }
};

struct TextPayload {
    glm::vec2 position = {};
    glm::vec4 color = {};
    float scale = 64.0f;
    float spacing = 4.0f;
    float line_spacePixels = 3.0f;
    bool showInCenter = false;
    bool is_disabled;

    // Inline text cache buffer to ensure safe multi-threaded snapshot lifetimes
    char text_content[128];

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const TextPayload*>(payload);

        // Render characters securely out of the continuous command memory track
        renderer.renderText(data.position, data.text_content, ctx->globalFont, data.color, data.scale, data.spacing, data.line_spacePixels, data.showInCenter);
    }
};

struct ButtonPayload {
    glm::vec4 dest_rect;
    glm::vec4 idle_color;
    glm::vec4 hover_color;
    glm::vec4 click_color;
    glm::vec4 text_color;

    TextureHandle texture;
    float text_scale;
    bool is_hovered;
    bool is_pressed;
    bool no_texture;
    bool is_disabled;

    // Fixed char array prevents multi-threaded pointer invalidation
    char label[32];

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const ButtonPayload*>(payload);

        // 1. Determine active color state based on interactive logic indicators
        glm::vec4 activeColor = data.idle_color;
        if (data.is_pressed)      activeColor = data.click_color;
        else if (data.is_hovered) activeColor = data.hover_color;

        // 2. Render background container geometry
        if (data.no_texture) {
            renderer.renderRectangle(data.dest_rect, activeColor, {}, 0.0f);
        }
        else {
            gl2d::Texture tex = ctx->assetManager.getTexture(data.texture);
            renderer.renderRectangle(data.dest_rect, tex, activeColor, {}, 0.0f);
        }

        // 3. Render centered UI label text string
        if (data.label[0] != '\0') {
            // Compute centered text offset approximations within the button window box
            glm::vec2 textPos = glm::vec2(
                data.dest_rect.x + 15.0f,
                data.dest_rect.y + (data.dest_rect.w * 0.25f)
            );
            renderer.renderText(textPos, data.label, ctx->globalFont, data.text_color, data.text_scale);
        }
    }
};

struct SliderPayload {
    glm::vec4 track_rect;
    glm::vec4 track_color;
    glm::vec4 knob_color;
    glm::vec2 knob_size;

    TextureHandle track_texture;
    TextureHandle knob_texture;
    float normalized_value; // Exact value percentage from 0.0f to 1.0f
    bool use_textures;
    bool is_disabled;

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const SliderPayload*>(payload);

        // 1. Render base slider track track background
        if (!data.use_textures) {
            renderer.renderRectangle(data.track_rect, data.track_color, {}, 0.0f);
        }
        else {
            gl2d::Texture trackTex = ctx->assetManager.getTexture(data.track_texture);
            renderer.renderRectangle(data.track_rect, trackTex, data.track_color, {}, 0.0f);
        }

        // 2. Compute dynamic layout offset placement metrics for the handle knob center
        float knobX = data.track_rect.x + (data.track_rect.z * data.normalized_value) - (data.knob_size.x * 0.5f);
        float knobY = data.track_rect.y + (data.track_rect.w * 0.5f) - (data.knob_size.y * 0.5f);
        glm::vec4 knobRect = glm::vec4(knobX, knobY, data.knob_size.x, data.knob_size.y);

        // 3. Render the interactive handle knob element
        if (!data.use_textures) {
            renderer.renderRectangle(knobRect, data.knob_color, {}, 0.0f);
        }
        else {
            gl2d::Texture knobTex = ctx->assetManager.getTexture(data.knob_texture);
            renderer.renderRectangle(knobRect, knobTex, data.knob_color, {}, 0.0f);
        }
    }
};

struct TextBoxPayload {
    glm::vec4 bounds;
    glm::vec4 background_color;
    glm::vec4 border_color;
    glm::vec4 text_color;

    uint32_t cursor_index;
    float text_scale;
    bool is_focused;
    bool is_disabled;

    char content[64]; // Stores raw snapshot characters securely across threads

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const TextBoxPayload*>(payload);

        // 1. Render baseline backplate text frame container background
        renderer.renderRectangle(data.bounds, data.background_color, {}, 0.0f);

        // 2. Draw styled structural box outline border lines
        // Left line, top line, right line, bottom line
        renderer.renderLine({ data.bounds.x, data.bounds.y }, { data.bounds.x + data.bounds.z, data.bounds.y }, data.border_color, 2.0f);
        renderer.renderLine({ data.bounds.x, data.bounds.y }, { data.bounds.x, data.bounds.y + data.bounds.w }, data.border_color, 2.0f);
        renderer.renderLine({ data.bounds.x + data.bounds.z, data.bounds.y }, { data.bounds.x + data.bounds.z, data.bounds.y + data.bounds.w }, data.border_color, 2.0f);
        renderer.renderLine({ data.bounds.x, data.bounds.y + data.bounds.w }, { data.bounds.x + data.bounds.z, data.bounds.y + data.bounds.w }, data.border_color, 2.0f);

        // 3. Render strings content layout out of local memory tracks
        glm::vec2 textPos = glm::vec2(data.bounds.x + 10.0f, data.bounds.y + (data.bounds.w * 0.25f));
        renderer.renderText(textPos, data.content, ctx->globalFont, data.text_color, data.text_scale);

        // 4. Render blinking caret vertical input line indicator if input box is active
        if (data.is_focused) {
            // basic dynamic offset character sizing approximation (e.g., 14 pixels per character scaling width)
            float characterWidthOffset = static_cast<float>(data.cursor_index) * (28.0f * data.text_scale);
            float cursorLineX = textPos.x + characterWidthOffset;

            // Draw a blinking cursor tracking element (using a fast sine loop check)
            if (static_cast<int>(glfwGetTime() * 4.0) % 2 == 0) {
                glm::vec2 cursorStart = glm::vec2(cursorLineX, data.bounds.y + 6.0f);
                glm::vec2 cursorEnd = glm::vec2(cursorLineX, data.bounds.y + data.bounds.w - 6.0f);
                renderer.renderText(cursorStart, "|", ctx->globalFont, data.text_color, data.text_scale);
            }
        }
    }
};

struct DropdownHeaderPayload {
    glm::vec4 bounds;
    glm::vec4 color;
    glm::vec4 text_color;
    float text_scale;
    bool is_disabled;
    char selected_text[32];

    static void Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
        const auto& data = *reinterpret_cast<const DropdownHeaderPayload*>(payload);

        // 1. Draw header background box frame
        renderer.renderRectangle(data.bounds, data.color, {}, 0.0f);

        // 2. Render chosen item textual description
        glm::vec2 textPos = glm::vec2(data.bounds.x + 10.0f, data.bounds.y + (data.bounds.w * 0.25f));
        renderer.renderText(textPos, data.selected_text, ctx->globalFont, data.text_color, data.text_scale);

        // 3. Render dropdown arrow indicator indicator primitive glyph on the edge boundary
        glm::vec2 arrowPos = glm::vec2(data.bounds.x + data.bounds.z - 30.0f, data.bounds.y + (data.bounds.w * 0.25f));
        renderer.renderText(arrowPos, "v", ctx->globalFont, data.text_color, data.text_scale);
    }
};