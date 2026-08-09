#include "SettingsLayer.hpp"
#include <iostream>

void SettingsMenuLayer::onAttach(EngineContext* ctx) { std::cout << "[Layer] Settings Overlay Opened\n"; }
void SettingsMenuLayer::onDetach(EngineContext* ctx) { std::cout << "[Layer] Settings Overlay Closed -> Input Resumed\n"; }
void SettingsMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.key == GLFW_KEY_ESCAPE && ev.action == GLFW_PRESS) {
            ctx->layerStack->deferDetach(this); // REMOVE OVERLAY -> RESUME LOWER LAYER INPUT
        }
    }
}

void SettingsMenuLayer::update(double dt, EngineContext* ctx) {}
void SettingsMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // Dark semi-transparent backdrop
    writeBuffer.push_command(800, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0, 0, 0, 0.75f}, .no_texture = true });
    writeBuffer.push_command(810, 0, RectPayload{ .dest_rect = m_panelBounds, .color = {0.12f, 0.13f, 0.17f, 1.0f}, .no_texture = true });

    TextPayload title;
    title.color = { 1, 0.85f, 0.2f, 1 }; title.scale = 28.0f;
    title.position = { m_panelBounds.x + 200.0f, m_panelBounds.y + 40.0f }; title.showInCenter = true;
    snprintf(title.text_content, sizeof(title.text_content), "SETTINGS");
    writeBuffer.push_command(820, 0, title);

    ctx->ui.Slider(writeBuffer, ctx, ID_SET_VolumeSlider, { m_panelBounds.x + 50.0f, m_panelBounds.y + 150.0f, 300.0f, 20.0f }, m_volCache);

    if (ctx->ui.Button(writeBuffer, ctx, ID_SET_Back, { m_panelBounds.x + 100.0f, m_panelBounds.y + 280.0f, 200.0f, 45.0f }, "BACK")) {
        ctx->layerStack->deferDetach(this); // REMOVE OVERLAY -> RESUME LOWER LAYER INPUT
    }
}