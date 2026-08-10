#include "SaveLoadLayer.hpp"
#include "Gameplay.hpp"
#include "HUD.hpp"
#include "MainMenu.hpp"
#include <iostream>
#include <cstdio>
#include <filesystem>

// ============================================================================
// SAVE / LOAD SYSTEM MANAGER HELPERS
// ============================================================================
std::string SaveLoadManager::GetSlotPath(int slotIndex) {
    return "saves/slot_" + std::to_string(slotIndex + 1) + ".dat";
}

bool SaveLoadManager::ReadSlotHeader(int slotIndex, GameStateData& outData) {
    BinaryDeserializer deserializer;
    if (FileSystem::LoadFromFile(GetSlotPath(slotIndex), deserializer)) {
        // Implicit deserialization via operator>>
        deserializer >> outData;
        return true;
    }
    return false;
}

// ============================================================================
// SAVE MENU IMPLEMENTATION
// ============================================================================
void SaveMenuLayer::refreshSlotPreviews() {
    m_slotPreviews.assign(SaveLoadManager::NUM_SLOTS, GameStateData{});
    m_slotOccupied.assign(SaveLoadManager::NUM_SLOTS, false);

    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        if (SaveLoadManager::ReadSlotHeader(i, m_slotPreviews[i])) {
            m_slotOccupied[i] = true;
        }
    }
}

void SaveMenuLayer::onAttach(EngineContext* ctx) {
    std::cout << "[SaveMenuLayer] Opening Save Menu overlay.\n";
    refreshSlotPreviews();
}

void SaveMenuLayer::onDetach(EngineContext* ctx) {}

void SaveMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_ESCAPE) {
            ctx->layerStack->deferDetach(this);
        }
    }
}

void SaveMenuLayer::update(double dt, EngineContext* ctx) {}

void SaveMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Fullscreen Dim Backdrop
    writeBuffer.push_command(850, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.78f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Dialog Panel
    writeBuffer.push_command(860, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.14f, 0.18f, 0.96f },
        .no_texture = true,
        .is_world_space = false
        });

    // 3. Header Title & Status
    TextPayload titleText;
    titleText.color = { 1.0f, 0.85f, 0.2f, 1.0f };
    titleText.scale = 26.0f;
    titleText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f };
    titleText.showInCenter = true;
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "SAVE GAME");
    writeBuffer.push_command(870, 0, titleText);

    TextPayload statusText;
    statusText.color = { 0.8f, 0.85f, 0.9f, 1.0f };
    statusText.scale = 16.0f;
    statusText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 65.0f };
    statusText.showInCenter = true;
    std::snprintf(statusText.text_content, sizeof(statusText.text_content), "%s", m_statusMessage.c_str());
    writeBuffer.push_command(870, 0, statusText);

    float btnX = m_panelBounds.x + 30.0f;
    float btnW = m_panelBounds.z - 60.0f;
    float btnH = 75.0f;
    float startY = m_panelBounds.y + 95.0f;
    float slotGap = 90.0f;

    // 4. Procedural Save Slots Loop
    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        float currentY = startY + (i * slotGap);
        uint32_t widgetId = ID_SAVE_Slot_Base + i;

        char slotLabel[128];
        if (m_slotOccupied[i]) {
            const auto& data = m_slotPreviews[i];
            std::snprintf(slotLabel, sizeof(slotLabel),
                "SLOT %d: %s\nLVL %d | SCORE: %d\n%s",
                i + 1, data.playerName.c_str(), data.currentLevel, data.currentScore,
                data.getFormattedTimestamp().c_str());
        }
        else {
            std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %d: [ EMPTY ]", i + 1);
        }

        if (ctx->ui.Button(writeBuffer, ctx, widgetId, { btnX, currentY, btnW, btnH }, slotLabel, 18.0f)) {
            std::filesystem::create_directories("saves");

            // Capture active telemetry from Blackboard
            GameStateData currentPacket = GameStateData::CaptureFromBlackboard(ctx);

            // Serialize data packet using operator<< implicit contract
            BinarySerializer serializer;
            serializer << currentPacket;

            // Save to disk with CRC32 payload checksum
            if (FileSystem::SaveToFile(SaveLoadManager::GetSlotPath(i), serializer)) {
                std::cout << "[SaveLoadLayer] Game state written to " << SaveLoadManager::GetSlotPath(i) << "\n";
                m_statusMessage = "Saved successfully to Slot " + std::to_string(i + 1) + "!";
                refreshSlotPreviews();
            }
            else {
                m_statusMessage = "Failed to write save file!";
            }
        }
    }

    // 5. Back Button
    if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Back, { btnX, startY + (SaveLoadManager::NUM_SLOTS * slotGap) + 10.0f, btnW, 40.0f }, "BACK")) {
        ctx->layerStack->deferDetach(this);
    }
}

// ============================================================================
// LOAD MENU IMPLEMENTATION
// ============================================================================
void LoadMenuLayer::refreshSlotPreviews() {
    m_slotPreviews.assign(SaveLoadManager::NUM_SLOTS, GameStateData{});
    m_slotOccupied.assign(SaveLoadManager::NUM_SLOTS, false);

    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        if (SaveLoadManager::ReadSlotHeader(i, m_slotPreviews[i])) {
            m_slotOccupied[i] = true;
        }
    }
}

void LoadMenuLayer::onAttach(EngineContext* ctx) {
    std::cout << "[LoadMenuLayer] Opening Load Menu screen.\n";
    refreshSlotPreviews();
}

void LoadMenuLayer::onDetach(EngineContext* ctx) {}

void LoadMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_ESCAPE) {
            ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
            ctx->layerStack->deferDetach(this);
        }
    }
}

void LoadMenuLayer::update(double dt, EngineContext* ctx) {}

void LoadMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Base Background
    writeBuffer.push_command(100, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.08f, 0.1f, 0.12f, 1.0f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Dialog Panel
    writeBuffer.push_command(110, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.14f, 0.18f, 0.96f },
        .no_texture = true,
        .is_world_space = false
        });

    // 3. Header Title & Status
    TextPayload titleText;
    titleText.color = { 0.3f, 0.85f, 1.0f, 1.0f };
    titleText.scale = 26.0f;
    titleText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f };
    titleText.showInCenter = true;
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "LOAD GAME");
    writeBuffer.push_command(120, 0, titleText);

    TextPayload statusText;
    statusText.color = { 0.8f, 0.85f, 0.9f, 1.0f };
    statusText.scale = 16.0f;
    statusText.position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 65.0f };
    statusText.showInCenter = true;
    std::snprintf(statusText.text_content, sizeof(statusText.text_content), "%s", m_statusMessage.c_str());
    writeBuffer.push_command(120, 0, statusText);

    float btnX = m_panelBounds.x + 30.0f;
    float btnW = m_panelBounds.z - 60.0f;
    float btnH = 75.0f;
    float startY = m_panelBounds.y + 95.0f;
    float slotGap = 90.0f;

    // 4. Procedural Load Slots Loop
    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        float currentY = startY + (i * slotGap);
        uint32_t widgetId = ID_LOAD_Slot_Base + i;

        char slotLabel[128];
        if (m_slotOccupied[i]) {
            const auto& data = m_slotPreviews[i];
            std::snprintf(slotLabel, sizeof(slotLabel),
                "SLOT %d: %s\nLVL %d | SCORE: %d\n%s",
                i + 1, data.playerName.c_str(), data.currentLevel, data.currentScore,
                data.getFormattedTimestamp().c_str());
        }
        else {
            std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %d: [ EMPTY ]", i + 1);
        }

        if (ctx->ui.Button(writeBuffer, ctx, widgetId, { btnX, currentY, btnW, btnH }, slotLabel, 18.0f)) {
            if (m_slotOccupied[i]) {
                BinaryDeserializer deserializer;

                if (FileSystem::LoadFromFile(SaveLoadManager::GetSlotPath(i), deserializer)) {
                    GameStateData loadedPacket;

                    // Implicit deserialization via operator>>
                    deserializer >> loadedPacket;

                    // Hydrate Blackboard
                    loadedPacket.ApplyToBlackboard(ctx);

                    std::cout << "[SaveLoadLayer] Restored save state from Slot " << (i + 1) << "!\n";

                    // Transition to Gameplay
                    ctx->layerStack->deferClear();
                    ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
                    ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
                    return;
                }
                else {
                    m_statusMessage = "Corrupted save file in Slot " + std::to_string(i + 1) + "!";
                }
            }
            else {
                m_statusMessage = "Slot " + std::to_string(i + 1) + " is empty!";
            }
        }
    }

    // 5. Back Button
    if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Back, { btnX, startY + (SaveLoadManager::NUM_SLOTS * slotGap) + 10.0f, btnW, 40.0f }, "BACK")) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this);
    }
}