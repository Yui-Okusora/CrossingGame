#include "SaveLoadLayer.hpp"
#include "Gameplay.hpp"
#include "HUD.hpp"
#include "MainMenu.hpp"
#include <filesystem>

std::string SaveLoadManager::GetSlotPath(int slotIndex) {
    return "saves/slot_" + std::to_string(slotIndex + 1) + ".dat";
}

bool SaveLoadManager::ReadSlotHeader(int slotIndex, GameStateData& outData) {
    BinaryDeserializer deserializer;
    if (FileSystem::LoadFromFile(GetSlotPath(slotIndex), deserializer)) {
        deserializer >> outData;
        return true;
    }
    return false;
}

// ============================================================================
// SAVE MENU OVERLAY
// ============================================================================
void SaveMenuLayer::refreshSlotPreviews() {
    m_slotPreviews.assign(SaveLoadManager::NUM_SLOTS, GameStateData{});
    m_slotOccupied.assign(SaveLoadManager::NUM_SLOTS, false);
    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        if (SaveLoadManager::ReadSlotHeader(i, m_slotPreviews[i])) m_slotOccupied[i] = true;
    }
}

void SaveMenuLayer::onAttach(EngineContext* ctx) { refreshSlotPreviews(); }
void SaveMenuLayer::onDetach(EngineContext* ctx) {}

void SaveMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && ev.key == GLFW_KEY_ESCAPE) ctx->layerStack->deferDetach(this);
    }
}

void SaveMenuLayer::update(double dt, EngineContext* ctx) {}

void SaveMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Semi-transparent backdrop dimming
    writeBuffer.push_command(850, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.75f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Dark Slate Modal Box
    writeBuffer.push_command(860, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    // Border Lines
    glm::vec4 borderCol{ 0.28f, 0.35f, 0.46f, 1.0f };
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });

    // 3. Header Title & Status
    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f },
        .color = { 1.0f, 0.85f, 0.2f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "SAVE GAME");
    writeBuffer.push_command(870, 0, titleText);

    TextPayload statusText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 65.0f },
        .color = { 0.75f, 0.82f, 0.92f, 1.0f },
        .scale = 15.0f,
        .showInCenter = true
    };
    std::snprintf(statusText.text_content, sizeof(statusText.text_content), "%s", m_statusMessage.c_str());
    writeBuffer.push_command(870, 0, statusText);

    float btnX = m_panelBounds.x + 35.0f;
    float btnW = m_panelBounds.z - 70.0f;
    float startY = m_panelBounds.y + 95.0f;
    float slotGap = 92.0f;

    // 4. Save Slots
    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        float currentY = startY + (i * slotGap);
        char slotLabel[128];
        if (m_slotOccupied[i]) {
            const auto& data = m_slotPreviews[i];
            std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %d: %s | LVL %d | SCORE: %d\n%s",
                i + 1, data.playerName.c_str(), data.currentLevel, data.currentScore, data.getFormattedTimestamp().c_str());
        }
        else {
            std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %d: [ EMPTY ]", i + 1);
        }

        if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Slot_Base + i, { btnX, currentY, btnW, 76.0f }, slotLabel, 16.0f)) {
            std::filesystem::create_directories("saves");
            GameStateData packet = GameStateData::CaptureFromBlackboard(ctx);
            BinarySerializer serializer;
            serializer << packet;
            if (FileSystem::SaveToFile(SaveLoadManager::GetSlotPath(i), serializer)) {
                m_statusMessage = "Saved successfully to Slot " + std::to_string(i + 1) + "!";
                refreshSlotPreviews();
            }
            else {
                m_statusMessage = "Failed to write save file!";
            }
        }
    }

    // 5. Back Button
    if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Back, { btnX, startY + (SaveLoadManager::NUM_SLOTS * slotGap) + 10.0f, btnW, 45.0f }, "BACK", 18.0f)) {
        ctx->layerStack->deferDetach(this);
    }
}

// ============================================================================
// LOAD MENU OVERLAY
// ============================================================================
void LoadMenuLayer::refreshSlotPreviews() {
    m_slotPreviews.assign(SaveLoadManager::NUM_SLOTS, GameStateData{});
    m_slotOccupied.assign(SaveLoadManager::NUM_SLOTS, false);
    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        if (SaveLoadManager::ReadSlotHeader(i, m_slotPreviews[i])) m_slotOccupied[i] = true;
    }
}

void LoadMenuLayer::onAttach(EngineContext* ctx) { refreshSlotPreviews(); }
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
    TextureHandle bg = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/MainBackground.png");

    writeBuffer.push_command(50, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = bg,
        .no_texture = false,
        .is_world_space = false
        });

    writeBuffer.push_command(55, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.65f },
        .no_texture = true,
        .is_world_space = false
        });

    // Dark Slate Modal Box
    writeBuffer.push_command(60, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    glm::vec4 borderCol{ 0.28f, 0.35f, 0.46f, 1.0f };
    writeBuffer.push_command(65, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });

    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f },
        .color = { 0.3f, 0.85f, 1.0f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "LOAD GAME");
    writeBuffer.push_command(70, 0, titleText);

    TextPayload statusText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 65.0f },
        .color = { 0.75f, 0.82f, 0.92f, 1.0f },
        .scale = 15.0f,
        .showInCenter = true
    };
    std::snprintf(statusText.text_content, sizeof(statusText.text_content), "%s", m_statusMessage.c_str());
    writeBuffer.push_command(70, 0, statusText);

    float btnX = m_panelBounds.x + 35.0f;
    float btnW = m_panelBounds.z - 70.0f;
    float startY = m_panelBounds.y + 95.0f;
    float slotGap = 92.0f;

    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        float currentY = startY + (i * slotGap);
        char slotLabel[128];
        if (m_slotOccupied[i]) {
            const auto& data = m_slotPreviews[i];
            std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %d: %s | LVL %d | SCORE: %d\n%s",
                i + 1, data.playerName.c_str(), data.currentLevel, data.currentScore, data.getFormattedTimestamp().c_str());
        }
        else {
            std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %d: [ EMPTY ]", i + 1);
        }

        if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Slot_Base + i, { btnX, currentY, btnW, 76.0f }, slotLabel, 16.0f)) {
            if (m_slotOccupied[i]) {
                BinaryDeserializer deserializer;
                if (FileSystem::LoadFromFile(SaveLoadManager::GetSlotPath(i), deserializer)) {
                    GameStateData loadedPacket;
                    deserializer >> loadedPacket;
                    loadedPacket.ApplyToBlackboard(ctx);

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

    if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Back, { btnX, startY + (SaveLoadManager::NUM_SLOTS * slotGap) + 10.0f, btnW, 45.0f }, "BACK", 18.0f)) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this);
    }
}