#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class SettingsMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 400.0f, 200.0f, 400.0f, 380.0f };
    float m_volCache = 0.8f;
public:
    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }  // DIS INPUT
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; } // PAUSE LOWER UPDATES
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};

