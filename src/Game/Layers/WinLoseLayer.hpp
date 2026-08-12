#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"
#include <string>

class WinLosePopupLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 400.0f, 210.0f, 400.0f, 380.0f };

    std::string m_playerName = "HCMUS Student";
    int m_finalScore = 0;
    int m_highestScore = 0;
    int m_currentLevel = 1;
    bool m_isVictory = false;

public:
    explicit WinLosePopupLayer(bool isVictory = false) : m_isVictory(isVictory) {}
    ~WinLosePopupLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    // Modal overlay: suspends physics updates and blocks input to layers beneath
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};