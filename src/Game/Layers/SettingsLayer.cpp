#include "SettingsLayer.hpp"

void SettingsMenuLayer::onAttach(EngineContext* ctx) {
    m_masterVol = ctx->audioEngine.getCategoryVolume(AudioCategory::Master);
    m_musicVol = ctx->audioEngine.getCategoryVolume(AudioCategory::Music);
    m_sfxVol = ctx->audioEngine.getCategoryVolume(AudioCategory::GameplaySFX);
}

void SettingsMenuLayer::onDetach(EngineContext* ctx) {}

void SettingsMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_ESCAPE) ctx->layerStack->deferDetach(this);
    }
}

void SettingsMenuLayer::update(double dt, EngineContext* ctx) {}

void SettingsMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Semi-transparent Backdrop Dimming
    writeBuffer.push_command(800, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.75f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Settings Modal Dialog Box (Plain Dark Slate with Border)
    writeBuffer.push_command(810, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.11f, 0.13f, 0.17f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    // Border Outlines
    glm::vec4 borderColor{ 0.28f, 0.32f, 0.42f, 1.0f };
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderColor, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderColor, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderColor, 2.0f });
    writeBuffer.push_command(815, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderColor, 2.0f });

    // 3. Header Title
    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f },
        .color = { 1.0f, 0.85f, 0.2f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "AUDIO SETTINGS");
    writeBuffer.push_command(820, 0, titleText);

    float labelX = m_panelBounds.x + 40.0f;
    float sliderX = m_panelBounds.x + 40.0f;
    float sliderW = m_panelBounds.z - 80.0f;
    float startY = m_panelBounds.y + 85.0f;

    // --- MASTER VOLUME ---
    TextPayload masterLabel{
        .position = { labelX, startY },
        .color = { 0.9f, 0.9f, 0.95f, 1.0f },
        .scale = 18.0f
    };
    std::snprintf(masterLabel.text_content, sizeof(masterLabel.text_content), "MASTER VOLUME: %d%%", static_cast<int>(m_masterVol * 100.0f));
    writeBuffer.push_command(820, 0, masterLabel);

    if (ctx->ui.Slider(writeBuffer, ctx, ID_SET_MasterSlider, { sliderX, startY + 24.0f, sliderW, 16.0f }, m_masterVol)) {
        ctx->audioEngine.setCategoryVolume(AudioCategory::Master, m_masterVol);
    }

    // --- MUSIC VOLUME ---
    TextPayload musicLabel{
        .position = { labelX, startY + 75.0f },
        .color = { 0.9f, 0.9f, 0.95f, 1.0f },
        .scale = 18.0f
    };
    std::snprintf(musicLabel.text_content, sizeof(musicLabel.text_content), "MUSIC VOLUME: %d%%", static_cast<int>(m_musicVol * 100.0f));
    writeBuffer.push_command(820, 0, musicLabel);

    if (ctx->ui.Slider(writeBuffer, ctx, ID_SET_MusicSlider, { sliderX, startY + 99.0f, sliderW, 16.0f }, m_musicVol)) {
        ctx->audioEngine.setCategoryVolume(AudioCategory::Music, m_musicVol);
    }

    // --- SFX VOLUME ---
    TextPayload sfxLabel{
        .position = { labelX, startY + 150.0f },
        .color = { 0.9f, 0.9f, 0.95f, 1.0f },
        .scale = 18.0f
    };
    std::snprintf(sfxLabel.text_content, sizeof(sfxLabel.text_content), "SFX VOLUME: %d%%", static_cast<int>(m_sfxVol * 100.0f));
    writeBuffer.push_command(820, 0, sfxLabel);

    if (ctx->ui.Slider(writeBuffer, ctx, ID_SET_SFXSlider, { sliderX, startY + 174.0f, sliderW, 16.0f }, m_sfxVol)) {
        ctx->audioEngine.setCategoryVolume(AudioCategory::GameplaySFX, m_sfxVol);
        ctx->audioEngine.setCategoryVolume(AudioCategory::InteractSFX, m_sfxVol);
    }

    // --- BACK BUTTON ---
    if (ctx->ui.Button(writeBuffer, ctx, ID_SET_Back, { sliderX, startY + 240.0f, sliderW, 45.0f }, "BACK", 20.0f)) {
        ctx->layerStack->deferDetach(this);
    }
}