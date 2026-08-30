#include "DeadlinePopupLayer.hpp"

void DeadlinePopupLayer::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    m_alertSFX = ctx->audioEngine.loadSound(SFX_PATH "deadline_popup.wav");
    ctx->audioEngine.play(m_alertSFX, AudioCategory::GameplaySFX);
}

void DeadlinePopupLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}

void DeadlinePopupLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextureHandle deadlineTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/DeadlinePopup/DeadlinePopup.png");

    // 1. Fullscreen Dimming Backdrop (Depth: 750)
    writeBuffer.push_command(750, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.78f },
        .no_texture = true,
        .is_world_space = false
        });
        
    // 2. Render the Complete DeadlinePopup.png Texture Centered (Depth: 760)
    // Scaled to a clean window size of 520x280 pixels
    float popupW = 520.0f;
    float popupH = 280.0f;
    float popupX = (1200.0f - popupW) * 0.5f;
    float popupY = (805.0f - popupH) * 0.5f;

    writeBuffer.push_command(760, 0, RectPayload{
        .dest_rect = { popupX, popupY, popupW, popupH },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = deadlineTex,
        .no_texture = false,
        .is_world_space = false
        });
}

void DeadlinePopupLayer::update(double dt, EngineContext* ctx)
{
    m_lifetime -= static_cast<float>(dt);

    if (m_lifetime <= 0.0f) {
        ctx->layerStack->deferDetach(this);
    }
}