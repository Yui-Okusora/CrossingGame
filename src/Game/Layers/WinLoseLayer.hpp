#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"
#include <string>

class WinLosePopupLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 360.0f, 160.0f, 480.0f, 480.0f };

    std::string m_playerName = "HCMUS Student";
    GameMode m_mode = GameMode::Campaign;
    int m_finalScore = 0;
    int m_highestScore = 0;
    int m_currentLevel = 1;
    float m_elapsedTime = 0.0f;
    bool m_isVictory = false;

public:
    explicit WinLosePopupLayer(bool isVictory = false) : m_isVictory(isVictory) {}
    ~WinLosePopupLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};