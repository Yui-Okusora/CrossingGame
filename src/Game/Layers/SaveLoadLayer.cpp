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
    writeBuffer.push_command(850, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.75f },
        .no_texture = true,
        .is_world_space = false
        });

    writeBuffer.push_command(860, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    glm::vec4 borderCol{ 0.28f, 0.35f, 0.46f, 1.0f };
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(865, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });

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
    float slotGap = 98.0f;
    float slotHeight = 84.0f;

    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        float currentY = startY + (i * slotGap);

        if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Slot_Base + i, { btnX, currentY, btnW, slotHeight }, "")) {
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

        if (m_slotOccupied[i]) {
            const auto& data = m_slotPreviews[i];
            GameMode slotMode = static_cast<GameMode>(data.gameMode);
            AcademicBadgeInfo badgeInfo = GetAcademicBadgeInfo(slotMode, data.currentLevel, data.currentScore);
            TextureHandle badgeTex = ctx->assetManager.loadTexture(badgeInfo.badgePath);

            TextPayload namePayload{
                .position = { btnX + 18.0f, currentY + 18.0f },
                .color = { 1.0f, 0.90f, 0.35f, 1.0f },
                .scale = 16.0f,
                .showInCenter = false
            };
            std::snprintf(namePayload.text_content, sizeof(namePayload.text_content),
                "SLOT %d: %s", i + 1, data.playerName.c_str());
            writeBuffer.push_command(920, 0, namePayload);

            TextPayload statsPayload{
                .position = { btnX + 18.0f, currentY + 40.0f },
                .color = { 0.90f, 0.94f, 1.0f, 1.0f },
                .scale = 15.0f,
                .showInCenter = false
            };
            if (slotMode == GameMode::Endless) {
                std::snprintf(statsPayload.text_content, sizeof(statsPayload.text_content),
                    "ENDLESS MODE  |  SCORE: %d", data.currentScore);
            }
            else {
                std::snprintf(statsPayload.text_content, sizeof(statsPayload.text_content),
                    "LEVEL %d  |  SCORE: %d", data.currentLevel, data.currentScore);
            }
            writeBuffer.push_command(920, 0, statsPayload);

            TextPayload timePayload{
                .position = { btnX + 18.0f, currentY + 62.0f },
                .color = { 0.68f, 0.74f, 0.84f, 1.0f },
                .scale = 15.0f,
                .showInCenter = false
            };
            std::snprintf(timePayload.text_content, sizeof(timePayload.text_content),
                "SAVED: %s", data.getFormattedTimestamp().c_str());
            writeBuffer.push_command(920, 0, timePayload);

            if (badgeTex.id != 0) {
                float badgeSize = 52.0f;
                float badgeX = btnX + btnW - badgeSize - 16.0f;
                float badgeY = currentY + (slotHeight - badgeSize) * 0.5f;

                writeBuffer.push_command(925, 0, RectPayload{
                    .dest_rect = { badgeX, badgeY, badgeSize, badgeSize },
                    .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    .texture = badgeTex,
                    .no_texture = false,
                    .is_world_space = false
                    });
            }
        }
        else {
            TextPayload emptyPayload{
                .position = { btnX + (btnW * 0.5f), currentY + (slotHeight * 0.5f) },
                .color = { 0.55f, 0.60f, 0.68f, 1.0f },
                .scale = 17.0f,
                .showInCenter = true
            };
            std::snprintf(emptyPayload.text_content, sizeof(emptyPayload.text_content), "SLOT %d: [ EMPTY ]", i + 1);
            writeBuffer.push_command(920, 0, emptyPayload);
        }
    }

    if (ctx->ui.Button(writeBuffer, ctx, ID_SAVE_Back, { btnX, startY + (SaveLoadManager::NUM_SLOTS * slotGap) + 8.0f, btnW, 45.0f }, "BACK", 18.0f)) {
        ctx->layerStack->deferDetach(this);
    }
}

// ============================================================================
// LOAD MENU SCREEN
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
    float slotGap = 98.0f;
    float slotHeight = 84.0f;

    for (int i = 0; i < SaveLoadManager::NUM_SLOTS; ++i) {
        float currentY = startY + (i * slotGap);

        if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Slot_Base + i, { btnX, currentY, btnW, slotHeight }, "")) {
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

        if (m_slotOccupied[i]) {
            const auto& data = m_slotPreviews[i];
            GameMode slotMode = static_cast<GameMode>(data.gameMode);
            AcademicBadgeInfo badgeInfo = GetAcademicBadgeInfo(slotMode, data.currentLevel, data.currentScore);
            TextureHandle badgeTex = ctx->assetManager.loadTexture(badgeInfo.badgePath);

            TextPayload namePayload{
                .position = { btnX + 18.0f, currentY + 18.0f },
                .color = { 1.0f, 0.90f, 0.35f, 1.0f },
                .scale = 16.0f,
                .showInCenter = false
            };
            std::snprintf(namePayload.text_content, sizeof(namePayload.text_content),
                "SLOT %d: %s", i + 1, data.playerName.c_str());
            writeBuffer.push_command(920, 0, namePayload);

            TextPayload statsPayload{
                .position = { btnX + 18.0f, currentY + 40.0f },
                .color = { 0.90f, 0.94f, 1.0f, 1.0f },
                .scale = 15.0f,
                .showInCenter = false
            };
            if (slotMode == GameMode::Endless) {
                std::snprintf(statsPayload.text_content, sizeof(statsPayload.text_content),
                    "ENDLESS MODE  |  SCORE: %d", data.currentScore);
            }
            else {
                std::snprintf(statsPayload.text_content, sizeof(statsPayload.text_content),
                    "LEVEL %d  |  SCORE: %d", data.currentLevel, data.currentScore);
            }
            writeBuffer.push_command(920, 0, statsPayload);

            TextPayload timePayload{
                .position = { btnX + 18.0f, currentY + 62.0f },
                .color = { 0.68f, 0.74f, 0.84f, 1.0f },
                .scale = 15.0f,
                .showInCenter = false
            };
            std::snprintf(timePayload.text_content, sizeof(timePayload.text_content),
                "SAVED: %s", data.getFormattedTimestamp().c_str());
            writeBuffer.push_command(920, 0, timePayload);

            if (badgeTex.id != 0) {
                float badgeSize = 52.0f;
                float badgeX = btnX + btnW - badgeSize - 16.0f;
                float badgeY = currentY + (slotHeight - badgeSize) * 0.5f;

                writeBuffer.push_command(925, 0, RectPayload{
                    .dest_rect = { badgeX, badgeY, badgeSize, badgeSize },
                    .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    .texture = badgeTex,
                    .no_texture = false,
                    .is_world_space = false
                    });
            }
        }
        else {
            TextPayload emptyPayload{
                .position = { btnX + (btnW * 0.5f), currentY + (slotHeight * 0.5f) },
                .color = { 0.55f, 0.60f, 0.68f, 1.0f },
                .scale = 17.0f,
                .showInCenter = true
            };
            std::snprintf(emptyPayload.text_content, sizeof(emptyPayload.text_content), "SLOT %d: [ EMPTY ]", i + 1);
            writeBuffer.push_command(920, 0, emptyPayload);
        }
    }

    if (ctx->ui.Button(writeBuffer, ctx, ID_LOAD_Back, { btnX, startY + (SaveLoadManager::NUM_SLOTS * slotGap) + 8.0f, btnW, 45.0f }, "BACK", 18.0f)) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this);
    }
}