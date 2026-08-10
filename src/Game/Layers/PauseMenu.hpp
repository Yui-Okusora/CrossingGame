#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class PauseMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 450.0f, 180.0f, 300.0f, 410.0f };

public:
    PauseMenuLayer() = default;
    ~PauseMenuLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    // Modal overlay behavior: freezes physics updates and blocks input to layers beneath
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};