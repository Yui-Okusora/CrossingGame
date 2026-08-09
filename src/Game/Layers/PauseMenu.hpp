#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class PauseMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 450.0f, 200.0f, 300.0f, 400.0f };
public:
    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }  // DIS INPUT
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; } // PAUSE GAMEPLAY
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};

