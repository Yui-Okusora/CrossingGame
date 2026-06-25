#include "Engine/Engine.hpp"
#include <iostream>

class TestSandboxLayer : public IEngineLayer {
private:
    // Core Simulation Metrics (infinite world canvas coordinates)
    glm::vec2 m_playerPos{ 100.0f, 200.0f }, m_playerSize{ 64.0f, 64.0f };
        glm::vec2 m_playerVelocity{ 0.0f, 0.0f }; // Persistent velocity vector tracking for impulse kinematics
    glm::vec4 m_obstacle{ 500.0f, 350.0f, 120.0f, 120.0f };

        // Smooth trailing hardware camera coordinate tracker
        glm::vec2 m_cameraPos{ 0.0f, 0.0f };

    std::string m_uiMessage = "WASD: Move | UI elements are resolution-independent and anchored!";

        // Local Data Stores (Zero widget lifecycle tracking objects)
        float m_masterVolume{ 0.6f };
        std::string m_username{ "Type Here..." };
        uint32_t m_textCursor{ static_cast<uint32_t>(m_username.size()) };

        // Functional Element Activation State Flags
        bool m_widgetsEnabled{ true };

    AudioHandle m_bgm, m_click, m_hit;
        enum CollisionIDs : uint32_t { ID_Player = 1, ID_Obstacle, ID_Btn1, ID_Sld1, ID_Txt1 };

public:
    void onAttach(EngineContext* ctx) override {
        m_bgm = ctx->audioEngine.loadSound(SFX_PATH "bgm.mp3", true);
            m_click = ctx->audioEngine.loadSound(SFX_PATH "click.mp3", false);
            m_hit = ctx->audioEngine.loadSound(SFX_PATH "hit.mp3", false);
            ctx->audioEngine.play(m_bgm, AudioCategory::Music, true);

            // Bootstrap initial camera position directly onto the player to avoid a first-frame snap
            glm::vec2 fbSize = ctx->window->getFramebufferSize();
            float dpiX = ctx->scaleFactorX.load(std::memory_order_relaxed);
            float dpiY = ctx->scaleFactorY.load(std::memory_order_relaxed);
            glm::vec2 initialCanvas{ fbSize.x / dpiX, fbSize.y / dpiY };
        m_cameraPos = m_playerPos - (initialCanvas * 0.5f) + (m_playerSize * 0.5f);
    }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override {
        if (std::holds_alternative<KeyEvent>(event)) {
            auto ev = std::get<KeyEvent>(event);
            if (ev.key == GLFW_KEY_ESCAPE) {
                ctx->isRunning.store(false, std::memory_order_relaxed);
            }
            // Dynamic Element Activation Trigger Toggle
            if (ev.key == GLFW_KEY_T && ev.action == GLFW_PRESS) {
                m_widgetsEnabled = !m_widgetsEnabled;
                m_uiMessage = m_widgetsEnabled ? "UI elements activated." : "UI elements globally deactivated.";
                ctx->audioEngine.play(m_click, AudioCategory::InteractSFX, false);
            }
        }
    }

    void update(double dt, EngineContext* ctx) override {
        const float fDt = static_cast<float>(dt);

        // 1. Process standard player steering direction vectors [cite: 21-23]
        glm::vec2 inputDir{ 0.0f, 0.0f };
        if (ctx->input.isKeyHeld(GLFW_KEY_W)) inputDir.y -= 1.0f;
            if (ctx->input.isKeyHeld(GLFW_KEY_S)) inputDir.y += 1.0f;
                if (ctx->input.isKeyHeld(GLFW_KEY_A)) inputDir.x -= 1.0f;
                    if (ctx->input.isKeyHeld(GLFW_KEY_D)) inputDir.x += 1.0f;

                        if (glm::length(inputDir) > 0.0f) {
                            m_playerVelocity = glm::normalize(inputDir) * 450.0f; // Maintain stable velocity [cite: 23]
                        }
                        else {
                            // Apply fluid deceleration damping when keys are released to make bounces noticeable
                            m_playerVelocity = Utils::lerp2(m_playerVelocity, glm::vec2(0.0f, 0.0f), 0.15f, 0.01f);
                        }

        m_playerPos += m_playerVelocity * fDt; // Update position transform strip [cite: 23]

        // Infinite canvas world limits containment
        m_playerPos.x = glm::clamp(m_playerPos.x, -5000.0f, 5000.0f);
        m_playerPos.y = glm::clamp(m_playerPos.y, -5000.0f, 5000.0f);

        // ============================================================================
        // ENHANCEMENT: ENHANCED KINEMATIC BOUNCE RESOLUTION
        // Replaces simple boolean overlaps with physical restitution calculations
        // ============================================================================
        glm::vec4 playerRect{ m_playerPos.x, m_playerPos.y, m_playerSize.x, m_playerSize.y };
            PhysicsManifold manifold = CollisionEngine::CalculateManifold(playerRect, m_obstacle);

        if (manifold.intersected) {
            ctx->audioEngine.stop(m_hit);
                ctx->audioEngine.play(m_hit, AudioCategory::GameplaySFX, false);

                // Instantly deflect velocity vectors across contact normals with a 70% bounce retention factor
                CollisionEngine::ResolveKinematicBounce(m_playerPos, m_playerVelocity, m_playerSize, m_obstacle, 0.70f);
        }

        // 2. Compute Player-Centered Hardware Camera Targets
        glm::vec2 fbSize = ctx->window->getFramebufferSize();
            float dpiX = ctx->scaleFactorX.load(std::memory_order_relaxed);
            float dpiY = ctx->scaleFactorY.load(std::memory_order_relaxed);
            glm::vec2 logicalCanvas{ fbSize.x / dpiX, fbSize.y / dpiY };

        glm::vec2 targetCameraPos = m_playerPos - (logicalCanvas * 0.5f) + (m_playerSize * 0.5f);

        // Smoothly pan camera frames toward the target position using step lerping [cite: 396-398]
        m_cameraPos = Utils::lerp2(m_cameraPos, targetCameraPos, 0.12f, 0.01f);
    }

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override {
        // Fetch dynamic platform configurations to set up the viewport bounds
        glm::vec2 fbSize = ctx->window->getFramebufferSize();
            float dpiX = ctx->scaleFactorX.load(std::memory_order_relaxed);
            float dpiY = ctx->scaleFactorY.load(std::memory_order_relaxed);
            glm::vec2 canvas{ fbSize.x / dpiX, fbSize.y / dpiY };

        // ============================================================================
        // PASS 1: WORLD SPACE GRAPHICS COMMAND STREAM (SCROLLS WITH CAMERA)
        // All elements pushed here render relative to world coordinates
        // ============================================================================
        writeBuffer.push_command(0, 0, ClearScreenPayload{ .color = { 0.08f, 0.08f, 0.1f, 1.0f } });
            writeBuffer.push_command(1, 0, CameraPayload{ .position = m_cameraPos, .zoom = 1.0f });

        // Draw entities into world coordinates [cite: 30-31]
        glm::vec4 dynamicPlayerColor = !m_playerVelocity.x && !m_playerVelocity.y ? glm::vec4(0, 1, 0, 1) : glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);
        if (CollisionEngine::CalculateManifold({ m_playerPos.x, m_playerPos.y, m_playerSize.x, m_playerSize.y }, m_obstacle).intersected) {
            dynamicPlayerColor = glm::vec4{ 1.0f, 0.15f, 0.15f, 1.0f }; // Flash Red on rigid impact [cite: 27]
        }

        writeBuffer.push_command(10, 0, RectPayload{ .dest_rect = m_obstacle, .color = { 0.4f, 0.15f, 0.15f, 1.0f }, .no_texture = true });
            writeBuffer.push_command(20, 0, RectPayload{ .dest_rect = { m_playerPos.x, m_playerPos.y, m_playerSize.x, m_playerSize.y }, .color = dynamicPlayerColor, .no_texture = true });

            // ============================================================================
            // PASS 2: HUD SPACE RUN (PINNED RESOLUTION-INDEPENDENT COORD MARGINS)
            // Forces camera views back to static layout rules
            // ============================================================================
            writeBuffer.push_command(799, 0, CameraPayload{ .position = {0,0}, .zoom = 1.0f });

            // Dynamic coordinate resolution mapping via edge anchoring functions
            glm::vec4 myBtnRect = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(240, 50), glm::vec2(30, 30), UIAnchor::BottomLeft);
        glm::vec4 mySliderRect = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(300, 20), glm::vec2(30, 45), UIAnchor::BottomCenter);
        glm::vec4 myTxtRect = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(320, 45), glm::vec2(30, 32), UIAnchor::BottomRight);

        // Render stationary HUD notification texts top-left [cite: 33-35]
        TextPayload txt; txt.color = { 0.9f, 0.92f, 0.95f, 1.0f }; txt.scale = 0.35f; // Realigned standard scale point sizes [cite: 34]
        snprintf(txt.text_content, sizeof(txt.text_content), "%s", m_uiMessage.c_str());
            txt.position = { 30.0f, 40.0f }; writeBuffer.push_command(800, 0, txt);

            snprintf(txt.text_content, sizeof(txt.text_content), "World Coordinates: X:%.1f Y:%.1f | Hotkey [T]: Toggle UI Activity", m_playerPos.x, m_playerPos.y);
        txt.position = { 30.0f, 75.0f }; txt.color = glm::vec4(0.5f, 0.55f, 0.6f, 1.0f);
        writeBuffer.push_command(800, 0, txt);

        // ----------------------------------------------------------------------------
        // DECLARATIVE INTERACTION UI LAYER WITH ELEMENT ACTIVATION GATES
        // ----------------------------------------------------------------------------

        // 1. Interactive Button Widget
        UIState btn = UI::Button(ctx, ID_Btn1, myBtnRect, m_widgetsEnabled);
            if (btn.clicked) {
                ctx->audioEngine.play(m_click, AudioCategory::InteractSFX, false);
                    m_uiMessage = "Telemetry modification payload successfully generated!";
            }
        writeBuffer.push_command(900, 0, ButtonPayload{
            .dest_rect = myBtnRect,
            .idle_color = {0.18f, 0.19f, 0.22f, 1.0f},
            .hover_color = {0.25f, 0.26f, 0.32f, 1.0f},
            .click_color = {0.12f, 0.45f, 0.75f, 1.0f},
            .text_color = {1.0f, 1.0f, 1.0f, 1.0f},
            .text_scale = 0.35f, // Fixed typography sizing scale metrics [cite: 39]
            .is_hovered = btn.hovered,
            .is_pressed = btn.pressed,
            .no_texture = true,
            .is_disabled = !m_widgetsEnabled, // Element-Level Draw Control
            .label = "MUTATE TELEMETRY"
            });

        // 2. Interactive Slider Widget
        UIState sld = UI::Slider(ctx, ID_Sld1, mySliderRect, m_masterVolume, m_widgetsEnabled);
            if (m_widgetsEnabled) {
                ctx->audioEngine.setCategoryVolume(AudioCategory::Master, m_masterVolume);
            }
        writeBuffer.push_command(900, 0, SliderPayload{
            .track_rect = mySliderRect,
            .track_color = m_widgetsEnabled ? glm::vec4(0.12f, 0.12f, 0.14f, 1.0f) : glm::vec4(0.08f, 0.08f, 0.09f, 1.0f),
            .knob_color = !m_widgetsEnabled ? glm::vec4(0.2f, 0.2f, 0.22f, 1.0f) : sld.pressed ? glm::vec4{0.1f, 0.45f, 0.75f, 1.0f} : glm::vec4{0.4f, 0.4f, 0.45f, 1.0f},
            .knob_size = {16, 30},
            .normalized_value = m_masterVolume,
            .use_textures = false
            });

        // 3. Interactive TextBox Widget
        UIState box = UI::TextBox(ctx, ID_Txt1, myTxtRect, m_username, m_textCursor, m_widgetsEnabled);
            TextBoxPayload tb{
                .bounds = myTxtRect,
                .background_color = m_widgetsEnabled ? glm::vec4(0.05f, 0.05f, 0.06f, 1.0f) : glm::vec4(0.08f, 0.08f, 0.09f, 1.0f),
                .border_color = !m_widgetsEnabled ? glm::vec4(0.15f, 0.15f, 0.17f, 1.0f) : box.focused ? glm::vec4{0.1f, 0.45f, 0.75f, 1.0f} : glm::vec4{0.2f, 0.2f, 0.25f, 1.0f},
                .text_color = m_widgetsEnabled ? glm::vec4(0.9f, 0.9f, 0.95f, 1.0f) : glm::vec4(0.35f, 0.35f, 0.38f, 1.0f),
                .cursor_index = m_textCursor,
                .text_scale = 0.35f, // Fixed typography sizing scale metrics [cite: 45]
                .is_focused = box.focused
        };
        snprintf(tb.content, sizeof(tb.content), "%s", m_username.c_str());
            writeBuffer.push_command(900, 0, tb);
    }

    void onDetach(EngineContext* ctx) override {}
};

//int main() {
//    try {
//        std::cout << "[Engine] Initializing Application Facade System...\n";
//
//        // 1. Spin up window context layers and graphic batch renderer blocks
//        Application app("Horizon Engine - Multi-threaded Test Driver", 1200, 805);
//
//        // 2. Fetch the newly created engine context hub
//        EngineContext& ctx = app.getContext();
//
//        // 3. Mount our testing layer into the composite execution stack
//        ctx.layerStack->pushLayer(std::make_unique<TestSandboxLayer>(), &ctx);
//
//        // 4. Fire up loops (automatically spins up background simulation worker thread)
//        std::cout << "[Engine] Bootstrapping Complete. Handing over loops to hardware cores.\n";
//        app.run();
//    }
//    catch (const std::exception& e) {
//        std::cerr << "[Fatal Exception Encountered]: " << e.what() << "\n";
//        return -1;
//    }
//
//    std::cout << "[Engine] Shutdown complete. Resources cleanly detached.\n";
//    return 0;
//}