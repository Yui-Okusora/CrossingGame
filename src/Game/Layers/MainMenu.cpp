#include "MainMenu.hpp"
#include "SettingsLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "ModeSelectLayer.hpp"
#include "HUD.hpp"
#include "Gameplay.hpp"

// ============================================================================
// NAME INPUT SCREEN
// ============================================================================
void NameInputLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextureHandle bg = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/MainBackground.png");

    // 1. Fullscreen Main Menu Backdrop
    writeBuffer.push_command(50, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = bg,
        .no_texture = false,
        .is_world_space = false
        });

    // Dark Dimming Overlay
    writeBuffer.push_command(55, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.65f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Dark Slate Modal Box
    glm::vec4 panelBounds{ 380.0f, 220.0f, 440.0f, 350.0f };
    writeBuffer.push_command(60, 0, RectPayload{
        .dest_rect = panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    // Modal Border Lines
    glm::vec4 borderCol{ 0.28f, 0.35f, 0.46f, 1.0f };
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x, panelBounds.y}, {panelBounds.x + panelBounds.z, panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x, panelBounds.y}, {panelBounds.x, panelBounds.y + panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x + panelBounds.z, panelBounds.y}, {panelBounds.x + panelBounds.z, panelBounds.y + panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x, panelBounds.y + panelBounds.w}, {panelBounds.x + panelBounds.z, panelBounds.y + panelBounds.w}, borderCol, 2.0f });

    // 3. Header Title
    TextPayload title{
        .position = { panelBounds.x + (panelBounds.z * 0.5f), panelBounds.y + 35.0f },
        .color = { 1.0f, 0.85f, 0.2f, 1.0f },
        .scale = 24.0f,
        .showInCenter = true
    };
    std::snprintf(title.text_content, sizeof(title.text_content), "STUDENT PROFILE");
    writeBuffer.push_command(70, 0, title);

    // 4. Text Input Box & Action Buttons
    float btnX = panelBounds.x + 40.0f;
    float btnW = panelBounds.z - 80.0f;

    ctx->ui.TextBox(writeBuffer, ctx, ID_NAME_TextBox, { btnX, panelBounds.y + 85.0f, btnW, 50.0f }, m_playerName, m_cursorPos, 20.0f);

    if (ctx->ui.Button(writeBuffer, ctx, ID_NAME_Confirm, { btnX, panelBounds.y + 175.0f, btnW, 45.0f }, "CONFIRM", 20.0f)) {
        ctx->blackboard.set("playerName", m_playerName);
        ctx->layerStack->deferAttach(std::make_unique<ModeSelectLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    if (ctx->ui.Button(writeBuffer, ctx, ID_NAME_Back, { btnX, panelBounds.y + 245.0f, btnW, 45.0f }, "BACK", 20.0f)) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this);
    }
}

// ============================================================================
// MAIN MENU SCREEN
// ============================================================================
void MainMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextureHandle bg = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/MainBackground.png");
    TextureHandle playBtn = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Play button.png");
    TextureHandle continueBtn = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Continue button.png");
    TextureHandle optionBtn = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Option button.png");
    TextureHandle exitBtn = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/Exit button.png");

    writeBuffer.push_command(50, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = bg,
        .no_texture = false,
        .is_world_space = false
        });

    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_MM_Start, { 470.0f, 380.0f, 192.0f, 72.0f }, playBtn, { 2, 1 })) {
        ctx->layerStack->deferAttach(std::make_unique<NameInputLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_MM_Continue, { 470.0f, 455.0f, 192.0f, 72.0f }, continueBtn, { 2, 1 })) {
        ctx->layerStack->deferAttach(std::make_unique<LoadMenuLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_MM_Settings, { 470.0f, 530.0f, 192.0f, 72.0f }, optionBtn, { 2, 1 })) {
        ctx->layerStack->deferAttach(std::make_unique<SettingsMenuLayer>());
        return;
    }

    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_MM_Exit, { 470.0f, 605.0f, 192.0f, 72.0f }, exitBtn, { 2, 1 })) {
        ctx->isRunning.store(false, std::memory_order_relaxed);
    }
}