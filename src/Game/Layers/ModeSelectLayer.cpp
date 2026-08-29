#include "ModeSelectLayer.hpp"
#include "Gameplay.hpp"
#include "HUD.hpp"
#include "MainMenu.hpp"

void ModeSelectLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextureHandle bg = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Main Menu/MainBackground.png");

    // 1. Background Pass
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

    // 2. Dark Slate Modal Box
    glm::vec4 panelBounds{ 380.0f, 200.0f, 440.0f, 380.0f };
    writeBuffer.push_command(60, 0, RectPayload{
        .dest_rect = panelBounds,
        .color = { 0.12f, 0.15f, 0.20f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    // Border Outlines
    glm::vec4 borderCol{ 0.28f, 0.35f, 0.46f, 1.0f };
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x, panelBounds.y}, {panelBounds.x + panelBounds.z, panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x, panelBounds.y}, {panelBounds.x, panelBounds.y + panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x + panelBounds.z, panelBounds.y}, {panelBounds.x + panelBounds.z, panelBounds.y + panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(65, 0, LinePayload{ {panelBounds.x, panelBounds.y + panelBounds.w}, {panelBounds.x + panelBounds.z, panelBounds.y + panelBounds.w}, borderCol, 2.0f });

    // 3. Header Title
    TextPayload title{
        .position = { panelBounds.x + (panelBounds.z * 0.5f), panelBounds.y + 35.0f },
        .color = { 1.0f, 0.85f, 0.2f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(title.text_content, sizeof(title.text_content), "SELECT GAME MODE");
    writeBuffer.push_command(70, 0, title);

    float btnX = panelBounds.x + 40.0f;
    float btnW = panelBounds.z - 80.0f;
    float startY = panelBounds.y + 85.0f;
    float gap = 70.0f;

    // Campaign Button
    if (ctx->ui.Button(writeBuffer, ctx, ID_MODE_Campaign, { btnX, startY, btnW, 52.0f }, "CAMPAIGN (5 LEVELS)", 18.0f)) {
        ctx->blackboard.set("gameMode", static_cast<int>(GameMode::Campaign));
        ctx->blackboard.set("currentLevel", 1);
        ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
        ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    // Endless Survival Button
    if (ctx->ui.Button(writeBuffer, ctx, ID_MODE_Endless, { btnX, startY + gap, btnW, 52.0f }, "ENDLESS SURVIVAL", 18.0f)) {
        ctx->blackboard.set("gameMode", static_cast<int>(GameMode::Endless));
        ctx->blackboard.set("currentLevel", 1);
        ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
        ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
        ctx->layerStack->deferDetach(this);
        return;
    }

    // Back Button
    if (ctx->ui.Button(writeBuffer, ctx, ID_MODE_Back, { btnX, startY + (gap * 2.0f) + 10.0f, btnW, 45.0f }, "BACK", 18.0f)) {
        ctx->layerStack->deferAttach(std::make_unique<MainMenuLayer>());
        ctx->layerStack->deferDetach(this);
    }
}