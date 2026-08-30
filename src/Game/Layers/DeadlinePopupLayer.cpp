#include "DeadlinePopupLayer.hpp"

void DeadlinePopupLayer::onAttach(EngineContext* ctx) {
    if (!ctx) return;
    m_alertSFX = ctx->audioEngine.loadSound(SFX_PATH "elevator_chime.wav");
    ctx->audioEngine.play(m_alertSFX, AudioCategory::GameplaySFX);
}

void DeadlinePopupLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS && (ev.key == GLFW_KEY_ENTER || ev.key == GLFW_KEY_SPACE || ev.key == GLFW_KEY_ESCAPE)) {
            ctx->layerStack->deferDetach(this);
        }
    }
}

void DeadlinePopupLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    TextureHandle deadlineTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/DeadlinePopup/DeadlinePopup.png");

    // 1. Fullscreen Dimming Backdrop (Depth: 750)
    writeBuffer.push_command(750, 0, RectPayload{
        .dest_rect = { 0.0f, 0.0f, 1200.0f, 805.0f },
        .color = { 0.0f, 0.0f, 0.0f, 0.80f },
        .no_texture = true,
        .is_world_space = false
        });

    // 2. Modal Slate Panel (Depth: 760)
    writeBuffer.push_command(760, 0, RectPayload{
        .dest_rect = m_panelBounds,
        .color = { 0.12f, 0.14f, 0.18f, 0.98f },
        .no_texture = true,
        .is_world_space = false
        });

    // Warning Red Borders (Depth: 765)
    glm::vec4 borderCol{ 0.95f, 0.25f, 0.25f, 1.0f };
    writeBuffer.push_command(765, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, borderCol, 2.0f });
    writeBuffer.push_command(765, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y}, {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(765, 0, LinePayload{ {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });
    writeBuffer.push_command(765, 0, LinePayload{ {m_panelBounds.x, m_panelBounds.y + m_panelBounds.w}, {m_panelBounds.x + m_panelBounds.z, m_panelBounds.y + m_panelBounds.w}, borderCol, 2.0f });

    // 3. Header Text (Depth: 770)
    TextPayload titleText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 35.0f },
        .color = { 1.0f, 0.25f, 0.25f, 1.0f },
        .scale = 26.0f,
        .showInCenter = true
    };
    std::snprintf(titleText.text_content, sizeof(titleText.text_content), "URGENT DEADLINE!");
    writeBuffer.push_command(770, 0, titleText);

    // 4. Center Deadline Graphic (Depth: 775)
    writeBuffer.push_command(775, 0, RectPayload{
        .dest_rect = { m_panelBounds.x + (m_panelBounds.z * 0.5f) - 100.0f, m_panelBounds.y + 60.0f, 200.0f, 180.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = deadlineTex,
        .no_texture = false,
        .is_world_space = false
        });

    // 5. Notification Message (Depth: 780)
    TextPayload msgText{
        .position = { m_panelBounds.x + (m_panelBounds.z * 0.5f), m_panelBounds.y + 265.0f },
        .color = { 0.92f, 0.94f, 0.98f, 1.0f },
        .scale = 16.0f,
        .showInCenter = true
    };
    std::snprintf(msgText.text_content, sizeof(msgText.text_content),
        "A course assignment is due right now!\nTurn in the task to resume playing.");
    writeBuffer.push_command(780, 0, msgText);

    // 6. Dismiss Button (Depth: 900)
    float btnX = m_panelBounds.x + 40.0f;
    float btnW = m_panelBounds.z - 80.0f;
    float btnY = m_panelBounds.y + 355.0f;

    if (ctx->ui.Button(writeBuffer, ctx, ID_DEADLINE_Dismiss, { btnX, btnY, btnW, 50.0f }, "SUBMIT & RESUME", 18.0f)) {
        ctx->layerStack->deferDetach(this);
    }
}