#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class GameplayLayer : public IEngineLayer {
public:
    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};

