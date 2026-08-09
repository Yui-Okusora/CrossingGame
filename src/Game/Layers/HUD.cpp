#include "HUD.hpp"

void HUDLayer::onAttach(EngineContext* ctx) {}
void HUDLayer::onDetach(EngineContext* ctx) {}
void HUDLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {}
void HUDLayer::update(double dt, EngineContext* ctx) {}
void HUDLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextPayload hud;
    hud.color = { 1, 1, 1, 1 }; hud.scale = 24.0f;
    hud.position = { 30.0f, 35.0f };
    snprintf(hud.text_content, sizeof(hud.text_content), "HUD | PRESS 'P' TO PAUSE | PRESS 'K' TO TRIGGER WIN/LOSE");
    writeBuffer.push_command(500, 0, hud);
}