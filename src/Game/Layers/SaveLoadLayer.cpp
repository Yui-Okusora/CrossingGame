#include "SaveLoadLayer.hpp"
#include "HUD.hpp"
#include "Gameplay.hpp"
#include "MainMenu.hpp"
#include <iostream>

void SaveMenuLayer::onAttach(EngineContext* ctx) { std::cout << "[Layer] Save Menu Opened\n"; }
void SaveMenuLayer::onDetach(EngineContext* ctx) { std::cout << "[Layer] Save Menu Closed -> Returned to Previous Overlay\n"; }
void SaveMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void SaveMenuLayer::update(double dt, EngineContext* ctx) {}
void SaveMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(850, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0, 0, 0, 0.8f}, .no_texture = true });
    writeBuffer.push_command(860, 0, RectPayload{ .dest_rect = m_panelBounds, .color = {0.15f, 0.15f, 0.18f, 1.0f}, .no_texture = true });

    if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Slot1, { m_panelBounds.x + 70.0f, m_panelBounds.y + 80.0f, 300.0f, 45.0f }, "SAVE TO SLOT 1")) {
        std::cout << "[Save] Game Progress Saved!\n";
        ctx->layerStack->deferDetach(this); // DETACH SAVE -> RESUMES PAUSE / POPUP
    }

    if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Back, { m_panelBounds.x + 120.0f, m_panelBounds.y + 230.0f, 200.0f, 40.0f }, "CANCEL")) {
        ctx->layerStack->deferDetach(this); // DETACH SAVE
    }
}

void LoadMenuLayer::onAttach(EngineContext* ctx) {}
void LoadMenuLayer::onDetach(EngineContext* ctx) {}
void LoadMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void LoadMenuLayer::update(double dt, EngineContext* ctx) {}
void LoadMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(100, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0.08f, 0.1f, 0.12f, 1.0f}, .no_texture = true });

    // SWAP: LoadMenu -> Gameplay + HUD
    if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Slot1, { 450.0f, 300.0f, 300.0f, 50.0f }, "LOAD SAVE SLOT 1")) {
        ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
        ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
        ctx->layerStack->deferDetach(this); // SWAP OUT
    }

    // SWAP: LoadMenu -> MainMenu
    if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Back, { 450.0f, 480.0f, 300.0f, 45.0f }, "BACK")) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this); // SWAP OUT
    }
}