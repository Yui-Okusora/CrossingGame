#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class SaveMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 380.0f, 220.0f, 440.0f, 340.0f };
public:
    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }  // DIS INPUT
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};

class LoadMenuLayer : public IEngineLayer {
public:
    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};

