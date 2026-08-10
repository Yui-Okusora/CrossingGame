#include "SettingsLayer.hpp"
#include <iostream>
#include <cstdio>

void SettingsMenuLayer::onAttach(EngineContext* ctx) {
    std::cout << "[SettingsMenuLayer] Audio settings overlay opened.\n";

    // Hydrate local cache values from AudioEngine mixing channels
    m_masterVol = ctx->audioEngine.getCategoryVolume(AudioCategory::Master);
    m_musicVol = ctx->audioEngine.getCategoryVolume(AudioCategory::Music);
    m_sfxVol = ctx->audioEngine.getCategoryVolume(AudioCategory::GameplaySFX);
}

void SettingsMenuLayer::onDetach(EngineContext* ctx) {
    std::cout << "[SettingsMenuLayer] Audio settings overlay closed.\n";
}

void SettingsMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_ESCAPE) {
            ctx->layerStack->deferDetach(this);
        }
    }
}

void SettingsMenuLayer::update(double dt, EngineContext* ctx) {
    // Logic updates remain frozen while modal overlay is active
}

void SettingsMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Semi-transparent backdrop dimming
    writeBuffer.push_command(800, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.75f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Settings Dialog Box Container
    writeBuffer.push_command(810, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.13f, 0.17f, 0.96f },
        .no_texture = true,
        .is_world_space = false
        });

    // 3. Header Title
    TextPayload titleText;
    titleText.color = { 1.0f, 0.85f, 0.2f, 1.0f }; // Gold accent
    titleText.scale = 26.0f;
    titleText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f };
    titleText.showInCenter = true;
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "AUDIO SETTINGS");
    writeBuffer.push_command(820, 0, titleText);

    float labelX = m_panelBounds.x + 40.0f;
    float sliderX = m_panelBounds.x + 40.0f;
    float sliderW = m_panelBounds.z - 80.0f;
    float startY = m_panelBounds.y + 85.0f;

    // --- MASTER VOLUME SLIDER ---
    TextPayload masterLabel;
    masterLabel.color = { 0.9f, 0.9f, 0.95f, 1.0f };
    masterLabel.scale = 18.0f;
    masterLabel.position = { labelX, startY };
    masterLabel.showInCenter = false;
    std::snprintf(masterLabel.text_content, sizeof(masterLabel.text_content),
        "MASTER VOLUME: %d%%", static_cast<int>(m_masterVol * 100.0f));
    writeBuffer.push_command(820, 0, masterLabel);

    if (ctx->ui.Slider(writeBuffer, ctx, ID_SET_MasterSlider, { sliderX, startY + 22.0f, sliderW, 20.0f }, m_masterVol)) {
        ctx->audioEngine.setCategoryVolume(AudioCategory::Master, m_masterVol);
    }

    // --- MUSIC VOLUME SLIDER ---
    TextPayload musicLabel;
    musicLabel.color = { 0.9f, 0.9f, 0.95f, 1.0f };
    musicLabel.scale = 18.0f;
    musicLabel.position = { labelX, startY + 75.0f };
    musicLabel.showInCenter = false;
    std::snprintf(musicLabel.text_content, sizeof(musicLabel.text_content),
        "MUSIC VOLUME: %d%%", static_cast<int>(m_musicVol * 100.0f));
    writeBuffer.push_command(820, 0, musicLabel);

    if (ctx->ui.Slider(writeBuffer, ctx, ID_SET_MusicSlider, { sliderX, startY + 97.0f, sliderW, 20.0f }, m_musicVol)) {
        ctx->audioEngine.setCategoryVolume(AudioCategory::Music, m_musicVol);
    }

    // --- SFX VOLUME SLIDER ---
    TextPayload sfxLabel;
    sfxLabel.color = { 0.9f, 0.9f, 0.95f, 1.0f };
    sfxLabel.scale = 18.0f;
    sfxLabel.position = { labelX, startY + 150.0f };
    sfxLabel.showInCenter = false;
    std::snprintf(sfxLabel.text_content, sizeof(sfxLabel.text_content),
        "SFX VOLUME: %d%%", static_cast<int>(m_sfxVol * 100.0f));
    writeBuffer.push_command(820, 0, sfxLabel);

    if (ctx->ui.Slider(writeBuffer, ctx, ID_SET_SFXSlider, { sliderX, startY + 172.0f, sliderW, 20.0f }, m_sfxVol)) {
        ctx->audioEngine.setCategoryVolume(AudioCategory::GameplaySFX, m_sfxVol);
        ctx->audioEngine.setCategoryVolume(AudioCategory::InteractSFX, m_sfxVol);
    }

    // --- BACK BUTTON ---
    if (ctx->ui.Button(writeBuffer, ctx, ID_SET_Back, { sliderX, startY + 250.0f, sliderW, 45.0f }, "BACK")) {
        ctx->layerStack->deferDetach(this);
    }
}