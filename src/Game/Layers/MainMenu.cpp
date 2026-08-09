#include "MainMenu.hpp"
#include "SettingsLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "HUD.hpp"
#include "Gameplay.hpp"

void NameInputLayer::onAttach(EngineContext* ctx) {}
void NameInputLayer::onDetach(EngineContext* ctx) {}
void NameInputLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void NameInputLayer::update(double dt, EngineContext* ctx) {}
void NameInputLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(100, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0.08f, 0.1f, 0.12f, 1.0f}, .no_texture = true });

    ctx->ui.TextBox(writeBuffer, ctx, ID_NAME_TextBox, { 450.0f, 320.0f, 300.0f, 50.0f }, m_playerName, m_cursorPos);

    // SWAP: NameInput -> Gameplay + HUD
    if (ctx->ui.Button(writeBuffer, ctx, ID_NAME_Confirm, { 450.0f, 400.0f, 300.0f, 50.0f }, "START GAMEPLAY")) {
        ctx->blackboard.set("playerName", m_playerName);
        ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
        ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
        ctx->layerStack->deferDetach(this); // SWAP OUT
    }

    // SWAP: NameInput -> MainMenu
    if (ctx->ui.Button(writeBuffer, ctx, ID_NAME_Back, { 450.0f, 470.0f, 300.0f, 45.0f }, "BACK")) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this); // SWAP OUT
    }
}

void MainMenuLayer::onAttach(EngineContext* ctx) {}
void MainMenuLayer::onDetach(EngineContext* ctx) {}
void MainMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void MainMenuLayer::update(double dt, EngineContext* ctx) {}
void MainMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(100, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0.08f, 0.12f, 0.1f, 1.0f}, .no_texture = true });

    // SWAP: MainMenu -> NameInput
    if (ctx->ui.Button(writeBuffer, ctx, ID_MM_Start, { 470.0f, 280.0f, 260.0f, 55.0f }, "START")) {
        ctx->layerStack->deferAttach(std::make_unique<NameInputLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    // SWAP: MainMenu -> LoadMenu
    if (ctx->ui.Button(writeBuffer, ctx, ID_MM_Continue, { 470.0f, 355.0f, 260.0f, 55.0f }, "CONTINUE")) {
        ctx->layerStack->deferAttach(std::make_unique<LoadMenuLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    // ADD FRONT / DIS INPUT -> Push Settings Overlay on top of MainMenu
    if (ctx->ui.Button(writeBuffer, ctx, ID_MM_Settings, { 470.0f, 430.0f, 260.0f, 55.0f }, "SETTINGS")) {
        ctx->layerStack->deferAttach(std::make_unique<SettingsMenuLayer>());
        return;
    }

    // EXIT
    if (ctx->ui.Button(writeBuffer, ctx, ID_MM_Exit, { 470.0f, 505.0f, 260.0f, 55.0f }, "EXIT")) {
        ctx->isRunning.store(false, std::memory_order_relaxed);
        return;
    }
}