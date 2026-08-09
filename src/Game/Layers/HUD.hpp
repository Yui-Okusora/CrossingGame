#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class HUDLayer : public IEngineLayer {
public:
    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    [[nodiscard]] bool blocksEvents() const noexcept override { return false; } // PASS-THROUGH TO GAMEPLAY
    [[nodiscard]] bool blocksUpdates() const noexcept override { return false; }
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};