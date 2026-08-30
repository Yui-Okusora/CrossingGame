#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class DeadlinePopupLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 380.0f, 180.0f, 440.0f, 440.0f };
    AudioHandle m_alertSFX{ 0 };

public:
    DeadlinePopupLayer() = default;
    ~DeadlinePopupLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    // Freezes physics simulation and input for gameplay layers underneath
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return false; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override {}
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};