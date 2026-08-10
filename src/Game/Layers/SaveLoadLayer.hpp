#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"
#include "../Data/GameStateData.hpp"
#include <vector>
#include <string>

class SaveLoadManager {
public:
    static constexpr int NUM_SLOTS = 3;
    static std::string GetSlotPath(int slotIndex);
    static bool ReadSlotHeader(int slotIndex, GameStateData& outData);
};

// ============================================================================
// SAVE MENU OVERLAY
// ============================================================================
class SaveMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 350.0f, 130.0f, 500.0f, 540.0f };
    std::string m_statusMessage = "Select a slot to write save state";
    std::vector<GameStateData> m_slotPreviews;
    std::vector<bool> m_slotOccupied;

    void refreshSlotPreviews();

public:
    SaveMenuLayer() = default;
    ~SaveMenuLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};

// ============================================================================
// LOAD MENU SCREEN
// ============================================================================
class LoadMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 350.0f, 130.0f, 500.0f, 540.0f };
    std::string m_statusMessage = "Select a slot to restore save state";
    std::vector<GameStateData> m_slotPreviews;
    std::vector<bool> m_slotOccupied;

    void refreshSlotPreviews();

public:
    LoadMenuLayer() = default;
    ~LoadMenuLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;

    [[nodiscard]] bool blocksEvents() const noexcept override { return false; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return false; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;
};