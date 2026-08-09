#include "WinLoseLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "MainMenu.hpp"
#include <iostream>

void WinLosePopupLayer::onAttach(EngineContext* ctx) { std::cout << "[Layer] Win/Lose Popup Opened -> Gameplay Input Disabled\n"; }
void WinLosePopupLayer::onDetach(EngineContext* ctx) {}
void WinLosePopupLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void WinLosePopupLayer::update(double dt, EngineContext* ctx) {}
void WinLosePopupLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(800, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0, 0, 0, 0.75f}, .no_texture = true });
    writeBuffer.push_command(810, 0, RectPayload{ .dest_rect = m_panelBounds, .color = {0.15f, 0.1f, 0.1f, 1.0f}, .no_texture = true });

    // ACT -> Push Save Overlay on top of Popup
    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_SaveRecord, { 450.0f, 320.0f, 300.0f, 45.0f }, "SAVE RECORD")) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
    }

    // RESET STACK -> Wipes all layers -> Main Menu
    if (ctx->ui.Button(writeBuffer, ctx, ID_POP_MainMenu, { 450.0f, 380.0f, 300.0f, 45.0f }, "MAIN MENU")) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
        return;
    }
}