#include "RenderStream.hpp"
#include "../Context/EngineContext.hpp"
#include <gl2d/gl2d.h>

RenderData::RenderData() {
    commands.reserve(256); 
        payload_arena.reserve(4096); 
}
RenderData::~RenderData() = default;

void RenderData::reset() {
    commands.clear(); 
    payload_arena.clear(); 
    physicsAlpha = 0.0f;
}

void ClearScreenPayload::Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
    renderer.clearScreen(reinterpret_cast<const ClearScreenPayload*>(payload)->color); 
}

void CameraPayload::Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
    const auto& data = *reinterpret_cast<const CameraPayload*>(payload); 
        renderer.currentCamera.position = data.position; 
        renderer.currentCamera.zoom = data.zoom; 
}

void RectPayload::Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
    const auto& data = *reinterpret_cast<const RectPayload*>(payload);
    const auto& vp = ctx->currentViewport;

    // 1. Convert world coordinates to camera coordinates if needed
    glm::vec2 worldPos{ data.dest_rect.x, data.dest_rect.y };
    if (data.is_world_space) {
        worldPos -= ctx->cameraPos;
    }

    // 2. Apply viewport scale and letterbox offset
    glm::vec4 scaledRect{
        worldPos.x * vp.scale + vp.offset.x,
        worldPos.y * vp.scale + vp.offset.y,
        data.dest_rect.z * vp.scale,
        data.dest_rect.w * vp.scale
    };
    glm::vec2 scaledOrigin = data.origin * vp.scale;

    if (data.no_texture) {
        renderer.renderRectangle(scaledRect, data.color, scaledOrigin, data.rotation);
    }
    else {
        gl2d::Texture tex = ctx->assetManager.getTexture(data.texture);
        gl2d::TextureAtlas customAtlas(data.atlas_dimensions.x, data.atlas_dimensions.y);
        renderer.renderRectangle(scaledRect, tex, data.color, scaledOrigin, data.rotation,
            customAtlas.get(data.atlas_pos.x, data.atlas_pos.y, data.flip_x));
    }
}

void LinePayload::Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
    const auto& data = *reinterpret_cast<const LinePayload*>(payload);
    const auto& vp = ctx->currentViewport;

    glm::vec2 scaledStart = data.start * vp.scale + vp.offset;
    glm::vec2 scaledEnd = data.end * vp.scale + vp.offset;
    float scaledThickness = data.thickness * vp.scale;

    renderer.renderLine(scaledStart, scaledEnd, data.color, scaledThickness);
}

void TextPayload::Execute(gl2d::Renderer2D& renderer, const void* payload, EngineContext* ctx) {
    const auto& data = *reinterpret_cast<const TextPayload*>(payload);
    const auto& vp = ctx->currentViewport;

    glm::vec2 scaledPos = data.position * vp.scale + vp.offset;
    float scaledSize = data.scale * vp.scale;

    // gl2d's native renderText spacing parameter behaves in absolute screen pixels, 
    // so we scale spacing alongside the typography boundaries
    renderer.renderText(scaledPos, data.text_content, ctx->globalFont, data.color,
        scaledSize, data.spacing * vp.scale, data.line_spacePixels * vp.scale, data.showInCenter);
}