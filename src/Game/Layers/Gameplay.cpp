#include "Gameplay.hpp"
#include "WinLoseLayer.hpp"
#include "PauseMenu.hpp"

void GameplayLayer::onAttach(EngineContext* ctx) {}
void GameplayLayer::onDetach(EngineContext* ctx) {}
void GameplayLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_P) {
            // ADD FRONT / DIS INPUT -> Push Pause Overlay on top
            ctx->layerStack->deferAttach(std::make_unique<PauseMenuLayer>());
        }
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_K) {
            // WIN/LOSE ADD / DIS INPUT -> Push Win/Lose Overlay on top
            ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>());
        }
    }
}
void GameplayLayer::update(double dt, EngineContext* ctx) {}
void GameplayLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    writeBuffer.push_command(0, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0.2f, 0.4f, 0.25f, 1.0f}, .no_texture = true });
}