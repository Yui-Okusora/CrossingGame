#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"

class SettingsMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 400.0f, 180.0f, 400.0f, 420.0f };

    float m_masterVol = 1.0f;
    float m_musicVol = 0.5f;
    float m_sfxVol = 1.0f;

public:
    SettingsMenuLayer() = default;
    ~SettingsMenuLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    // Modal overlay: freezes physics updates and blocks input to layers beneath
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};