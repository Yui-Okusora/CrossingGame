#include "PauseMenu.hpp"
#include "SettingsLayer.hpp"
#include "SaveLoadLayer.hpp"
#include "MainMenu.hpp"
#include "Gameplay.hpp"
#include "HUD.hpp"

void PauseMenuLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && (ev.key == GLFW_KEY_ESCAPE || ev.key == GLFW_KEY_P)) {
            ctx->layerStack->deferDetach(this);
        }
    }
}

void PauseMenuLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Fullscreen Dimming Backdrop
    writeBuffer.push_command(750, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.75f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Keyboard Panel Background
    TextureHandle panelTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/MenuPanel.png");
    TextureHandle settingBtnTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/SettingButton.png");
    TextureHandle saveBtnTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/SaveButton.png");
    TextureHandle restartBtnTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/RestartButton.png");
    TextureHandle playBtnTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/PlayButton.png");
    TextureHandle mainMenuBtnTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Menu/MainMenuButton.png");

    writeBuffer.push_command(760, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = panelTex,
        .no_texture = false,
        .is_world_space = false
        });

    // 3. Header Title
    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y - 18.0f },
        .color = { 1.0f, 0.85f, 0.2f, 1.0f },
        .scale = 28.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "PAUSED");
    writeBuffer.push_command(770, 0, titleText);

    // ========================================================================
    // 4. KEYBOARD KEYCAP BUTTONS ({ 2, 1 } 2-State Spritesheets)
    // ========================================================================
    float px = m_panelBounds.x;
    float py = m_panelBounds.y;

    float keySize = 56.0f;

    // Row 1: Settings (Gear) & Save (Floppy Disk)
    glm::vec4 settingBounds{ px + 124.0f, py + 52.0f, keySize, keySize };
    glm::vec4 saveBounds{ px + 242.0f, py + 52.0f, keySize, keySize };

    // Row 2: Restart (Undo Arrow) & Resume (Play Arrow)
    glm::vec4 restartBounds{ px + 182.0f, py + 148.0f, keySize, keySize };
    glm::vec4 playBounds{ px + 300.0f, py + 148.0f, keySize, keySize };

    // Row 3: Main Menu (ESC Key - Wide 1.5u Keycap)
    glm::vec4 escBounds{ px + 440.0f, py + 208.0f, 88.0f, keySize };

    // SETTINGS BUTTON
    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_PAUSE_Settings, settingBounds, settingBtnTex, { 2, 1 })) {
        ctx->layerStack->deferAttach(std::make_unique<SettingsMenuLayer>());
        return;
    }

    // SAVE BUTTON
    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_PAUSE_Save, saveBounds, saveBtnTex, { 2, 1 })) {
        ctx->layerStack->deferAttach(std::make_unique<SaveMenuLayer>());
        return;
    }

    // RESTART BUTTON
    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_PAUSE_Restart, restartBounds, restartBtnTex, { 2, 1 })) {
        ctx->blackboard.set("currentLevel", 1);
        ctx->blackboard.set("currentScore", 0);
        ctx->blackboard.set("elapsedTime", 0.0f);

        ctx->layerStack->deferClear();
        ctx->layerStack->deferAttach(std::make_unique<GameplayLayer>());
        ctx->layerStack->deferAttach(std::make_unique<HUDLayer>());
        return;
    }

    // RESUME BUTTON
    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_PAUSE_Resume, playBounds, playBtnTex, { 2, 1 })) {
        ctx->layerStack->deferDetach(this);
        return;
    }

    // MAIN MENU BUTTON
    if (ctx->ui.TexturedButton(writeBuffer, ctx, ID_PAUSE_MainMenu, escBounds, mainMenuBtnTex, { 2, 1 })) {
        ctx->layerStack->clear(ctx);
        ctx->layerStack->pushLayer(std::make_unique<MainMenuLayer>(), ctx);
        return;
    }

    // Helper Legend
    TextPayload legendText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + m_panelBounds.w + 24.0f },
        .color = { 0.75f, 0.8f, 0.85f, 0.9f },
        .scale = 16.0f,
        .showInCenter = true
    };
    std::snprintf(legendText.text_content, sizeof(legendText.text_content),
        "[SETTINGS]  [SAVE]  |  [RESTART]  [RESUME]  |  [ESC: MAIN MENU]");
    writeBuffer.push_command(770, 0, legendText);
}