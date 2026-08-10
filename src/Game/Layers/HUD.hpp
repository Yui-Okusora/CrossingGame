#pragma once
#include <Engine/Engine.hpp>
#include <string>

class HUDLayer : public IEngineLayer {
private:
    std::string m_playerName = "HCMUS Student";
    int m_currentLevel = 1;
    int m_currentScore = 0;
    int m_highestScore = 0;

public:
    HUDLayer() = default;
    ~HUDLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    // Pass-through behavior: allow input events and fixed updates to reach GameplayLayer underneath
    [[nodiscard]] bool blocksEvents() const noexcept override { return false; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return false; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};