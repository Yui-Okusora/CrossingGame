#include <Engine/Engine.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Unique Identification Key Enums for Subsystem Queries
enum TestIDs : uint32_t {
    ID_Player = 1,
    ID_HazardBlock = 2,
    ID_PauseBtn = 101,
    ID_VolumeSld = 102,
    ID_DevSwapBtn = 201,
    ID_DevKillBtn = 202,
    ID_DevTxtBox = 203
};

// ============================================================================
// LAYER 1: THE INTERCEPTING MODAL OVERLAY (PAUSE MENU)
// Evaluates: Flow propagation markers, custom styling loops, live volume updates
// ============================================================================
class PauseMenuLayer : public IEngineLayer {
private:
    glm::vec4 m_panelBounds{ 450.0f, 250.0f, 300.0f, 300.0f };
    glm::vec4 m_resumeBtn{ 490.0f, 320.0f, 220.0f, 45.0f };
    glm::vec4 m_sliderTrack{ 490.0f, 420.0f, 220.0f, 16.0f };

    float m_localVolCache{ 0.5f };
    AudioHandle m_clickSFX;

public:
    void onAttach(EngineContext* ctx) override {
        m_clickSFX = ctx->audioEngine.loadSound(SFX_PATH "click.mp3", false);
        m_localVolCache = ctx->audioEngine.getCategoryVolume(AudioCategory::Master);
    }
    void onDetach(EngineContext* ctx) override {}

    // --- CRITICAL FLOW PROPAGATION FILTERS ---
    [[nodiscard]] bool blocksEvents() const noexcept override { return true; } // Stops inputs from leaking downward
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; } // Freezes the physics updates below it

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override {
        // Intercept alternate key execution paths to discard the layer
        if (std::holds_alternative<KeyEvent>(event)) {
            auto ev = std::get<KeyEvent>(event);
            if (ev.key == GLFW_KEY_P && ev.action == GLFW_PRESS) {
                ctx->layerStack->deferDetach(this); // Safe extraction outside loop traversal
            }
        }
    }

    void update(double dt, EngineContext* ctx) override {}

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override {
        // 1. Alpha screen overlay to darken background game mechanics
        writeBuffer.push_command(800, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0.0f, 0.0f, 0.0f, 0.65f}, .no_texture = true });

        // 2. Render Modal backplate background frame [cite: 231]
        writeBuffer.push_command(810, 0, RectPayload{ .dest_rect = m_panelBounds, .color = {0.11f, 0.11f, 0.14f, 1.0f}, .no_texture = true });

        // 3. Interactive Resume Button Execution Block
        UIState btnState = UI::Button(ctx, ID_PauseBtn, m_resumeBtn);
        if (btnState.clicked) {
            ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX, false);
            ctx->layerStack->deferDetach(this); // Safe unmount trigger
            return;
        }

        // Custom styling managed completely by you based on returned structural state flags
        glm::vec4 activeBtnColor = btnState.pressed ? glm::vec4{ 0.1f, 0.45f, 0.75f, 1.0f } :
            btnState.hovered ? glm::vec4{ 0.25f, 0.28f, 0.35f, 1.0f } : glm::vec4{ 0.18f, 0.19f, 0.22f, 1.0f };
        writeBuffer.push_command(820, 0, ButtonPayload{
            .dest_rect = m_resumeBtn, .idle_color = activeBtnColor, .hover_color = activeBtnColor,
            .click_color = activeBtnColor, .text_color = {1,1,1,1}, .text_scale = 0.35f, // Safe scalar fractional scale [cite: 34]
            .is_hovered = btnState.hovered, .is_pressed = btnState.pressed, .no_texture = true, .label = "RESUME WORLD"
            });

        // 4. Interactive Volume Slider Processing Block
        UIState sldState = UI::Slider(ctx, ID_VolumeSld, m_sliderTrack, m_localVolCache);
        ctx->audioEngine.setCategoryVolume(AudioCategory::Master, m_localVolCache); // Dynamic live runtime stream modification! [cite: 41]

        writeBuffer.push_command(820, 0, SliderPayload{
            .track_rect = m_sliderTrack, .track_color = {0.06f, 0.06f, 0.08f, 1.0f},
            .knob_color = sldState.pressed ? glm::vec4{0.1f, 0.45f, 0.75f, 1.0f} : glm::vec4{0.45f, 0.45f, 0.5f, 1.0f},
            .knob_size = {16, 30}, .normalized_value = m_localVolCache, .use_textures = false
            });

        TextPayload txt; txt.color = { 0.8f, 0.8f, 0.85f, 1.0f }; txt.scale = 0.3f; txt.position = { m_panelBounds.x + 35.0f, m_panelBounds.y + 140.0f };
        snprintf(txt.text_content, sizeof(txt.text_content), "Master Mixer Volume: %d%%", static_cast<int>(m_localVolCache * 100.0f));
        writeBuffer.push_command(830, 0, txt);
    }
};

// ============================================================================
// LAYER 2: THE GAMEPLAY SIMULATION WORLD
// Evaluates: Fixed-Timestep loops, input polling, cache-dense box physics
// ============================================================================
class GameplaySimulationLayer : public IEngineLayer {
private:
    glm::vec2 m_playerPos{ 200.0f, 300.0f }, m_playerSize{ 64.0f, 64.0f };
    glm::vec4 m_playerColor{ 0.0f, 1.0f, 0.0f, 1.0f };

    glm::vec4 m_hazardBounds{ 600.0f, 200.0f, 96.0f, 96.0f };
    glm::vec2 m_hazardVelocity{ 320.0f, 240.0f };

    glm::vec2 m_playerPosPrev{ 200.0f, 300.0f };
    glm::vec4 m_hazardBoundsPrev{ 600.0f, 200.0f, 96.0f, 96.0f };

    bool m_hasTriggeredHit{ false };
    AudioHandle m_hitSFX;
    std::vector<CollisionResult> m_physicsScratchpad;

public:
    void onAttach(EngineContext* ctx) override {
        m_hitSFX = ctx->audioEngine.loadSound(SFX_PATH "hit.mp3", false);
            m_physicsScratchpad.reserve(16);
    }
    void onDetach(EngineContext* ctx) override {}

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override {
        // Intercept edge-triggered key press to instantiate modal overlay scene
        if (std::holds_alternative<KeyEvent>(event)) {
            auto ev = std::get<KeyEvent>(event);
                if (ev.key == GLFW_KEY_P && ev.action == GLFW_PRESS) {
                    // Instantiates a Pause Menu directly above this layer safely at the frame boundary
                    ctx->layerStack->deferAttach(std::make_unique<PauseMenuLayer>());
                }
        }
    }

    void update(double dt, EngineContext* ctx) override {
        m_playerPosPrev = m_playerPos;
        m_hazardBoundsPrev = m_hazardBounds;

        const float fDt = static_cast<float>(dt);

        // Dynamic scale query to establish current logical window boundaries
        glm::vec2 fbSize = ctx->window->getFramebufferSize();
        float dpiX = ctx->scaleFactorX.load(std::memory_order_relaxed);
        float dpiY = ctx->scaleFactorY.load(std::memory_order_relaxed);
        glm::vec2 logicalCanvas{ fbSize.x / dpiX, fbSize.y / dpiY };

        // Standard input handling [cite: 589-591]
        glm::vec2 inputVector{ 0.0f, 0.0f };
        if (ctx->input.isKeyHeld(GLFW_KEY_W)) inputVector.y -= 1.0f;
        if (ctx->input.isKeyHeld(GLFW_KEY_S)) inputVector.y += 1.0f;
        if (ctx->input.isKeyHeld(GLFW_KEY_A)) inputVector.x -= 1.0f;
        if (ctx->input.isKeyHeld(GLFW_KEY_D)) inputVector.x += 1.0f;

        if (glm::length(inputVector) > 0.0f) {
            m_playerPos += glm::normalize(inputVector) * 450.0f * fDt;
        }

        // FIXED: Dynamic containment clamping using scaled window parameters
        m_playerPos.x = glm::clamp(m_playerPos.x, 0.0f, logicalCanvas.x - m_playerSize.x);
        m_playerPos.y = glm::clamp(m_playerPos.y, 0.0f, logicalCanvas.y - m_playerSize.y);

        // Hazard movement updates
        m_hazardBounds.x += m_hazardVelocity.x * fDt;
        m_hazardBounds.y += m_hazardVelocity.y * fDt;

        // FIXED: Bounce elements cleanly off scaled layout limits
        if (m_hazardBounds.x <= 0.0f || m_hazardBounds.x >= logicalCanvas.x - m_hazardBounds.z) m_hazardVelocity.x *= -1.0f;
        if (m_hazardBounds.y <= 0.0f || m_hazardBounds.y >= logicalCanvas.y - m_hazardBounds.w)  m_hazardVelocity.y *= -1.0f;

        // 3. Bidirectional AABB Collision Sweep [cite: 24-25]
        m_physicsScratchpad.clear();
        glm::vec4 currentBox{ m_playerPos.x, m_playerPos.y, m_playerSize.x, m_playerSize.y };
            ctx->collisionWorld.query_aabb(currentBox, CollisionLayer::Layer_Player, CollisionLayer::Layer_Obstacle, m_physicsScratchpad);

            if (!m_physicsScratchpad.empty()) {
                if (!m_hasTriggeredHit) {
                    ctx->audioEngine.play(m_hitSFX, AudioCategory::GameplaySFX, false);
                        m_hasTriggeredHit = true;
                }
                m_playerColor = glm::vec4{ 1.0f, 0.15f, 0.15f, 1.0f }; // Flash Red [cite: 27]
            }
            else {
                m_hasTriggeredHit = false;
                m_playerColor = glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f };  // Default Green [cite: 25]
            }
    }

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override {
        // Fetch the current thread-safe frame timeline remainder fraction
        float alpha = ctx->physicsAlpha.load(std::memory_order_relaxed);

        // FIXED: Linearly interpolate positions to match your screen's precise refresh rate time!
        glm::vec2 lerpedPlayerPos = glm::mix(m_playerPosPrev, m_playerPos, alpha);

        glm::vec2 lerpedHazardPos = glm::mix(
            glm::vec2(m_hazardBoundsPrev.x, m_hazardBoundsPrev.y),
            glm::vec2(m_hazardBounds.x, m_hazardBounds.y),
            alpha
        );

        glm::vec4 drawHazardRect{ lerpedHazardPos.x, lerpedHazardPos.y, m_hazardBounds.z, m_hazardBounds.w };
        glm::vec4 drawPlayerRect{ lerpedPlayerPos.x, lerpedPlayerPos.y, m_playerSize.x, m_playerSize.y };

        // Register the true current structural collider box into the hash system
        ctx->collisionWorld.register_collider(ID_HazardBlock, m_hazardBounds, CollisionLayer::Layer_Obstacle);

        // FIXED: Push the smoothly blended positions into the drawing pipeline
        writeBuffer.push_command(10, 0, RectPayload{ .dest_rect = drawHazardRect, .color = {0.55f, 0.12f, 0.12f, 1.0f}, .no_texture = true });
        writeBuffer.push_command(20, 0, RectPayload{ .dest_rect = drawPlayerRect, .color = m_playerColor, .no_texture = true });
    }
};

// ============================================================================
// LAYER 3: THE DEPLOYED DEVELOPER DASHBOARD INTERFACE
// Evaluates: Advanced Deferred Stack Commands Queue, character streams, textboxes
// ============================================================================
class DevDashboardLayer : public IEngineLayer {
private:
    glm::vec4 m_swapBtnBounds{ 30.0f, 720.0f, 200.0f, 45.0f };
    glm::vec4 m_killBtnBounds{ 250.0f, 720.0f, 200.0f, 45.0f };
    glm::vec4 m_txtBoxBounds{ 470.0f, 720.0f, 250.0f, 45.0f };

    std::string m_profileName{ "Yui Okusora" };
    uint32_t m_txtCursor{ static_cast<uint32_t>(m_profileName.size()) };
    AudioHandle m_clickSFX;

public:
    void onAttach(EngineContext* ctx) override {
        m_clickSFX = ctx->audioEngine.loadSound(SFX_PATH "click.mp3", false);
    }
    void onDetach(EngineContext* ctx) override {}

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override {}
    void update(double dt, EngineContext* ctx) override {}

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
        // 1. Fetch current runtime resolution aspects 
        glm::vec2 canvas = ctx->window->getFramebufferSize();

        writeBuffer.push_command(799, 0, CameraPayload{ .position = {0,0}, .zoom = 1.0f });

        // FIXED: Render the bottom control bar relative to the true bottom edge of your screen space
        float barHeight = 105.0f;
        glm::vec4 dynamicRibbon{ 0.0f, canvas.y - barHeight, canvas.x, barHeight };
        writeBuffer.push_command(880, 0, RectPayload{ .dest_rect = dynamicRibbon, .color = {0.05f, 0.05f, 0.07f, 1.0f}, .no_texture = true });

        // FIXED: Calculate anchor positions based on the full physical canvas size [cite: 52-53, 465]
        glm::vec4 dynamicSwapBounds = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(200, 45), glm::vec2(30, 30), UIAnchor::BottomLeft);
        glm::vec4 dynamicKillBounds = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(200, 45), glm::vec2(250, 30), UIAnchor::BottomLeft);
        glm::vec4 dynamicTxtBounds = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(250, 45), glm::vec2(470, 30), UIAnchor::BottomLeft);

        // Pass elements to UI collision check and render pipeline natively [cite: 614-624]
        UIState swapState = UI::Button(ctx, ID_DevSwapBtn, dynamicSwapBounds);
        UIState killState = UI::Button(ctx, ID_DevKillBtn, dynamicKillBounds);
        UIState boxState = UI::TextBox(ctx, ID_DevTxtBox, dynamicTxtBounds, m_profileName, m_txtCursor);

        // Text Overlay Snapping points
        TextPayload info; info.color = { 0.5f, 0.7f, 0.9f, 1.0f }; info.scale = 0.32f;
        info.position = { 30.f, 30.f };
        snprintf(info.text_content, sizeof(info.text_content), "Active Stack Size: %zu | Mouse Screen Coords: {%.1f, %.1f}",
            ctx->layerStack->getLayers().size(), ctx->input.getMousePosition().x, ctx->input.getMousePosition().y);
        writeBuffer.push_command(890, 0, info);

        info.position = { 30.f, 60.f }; info.color = { 0.8f, 0.8f, 0.4f, 1.0f };
        snprintf(info.text_content, sizeof(info.text_content), "Active Dev Profile: %s", m_profileName.c_str());
        writeBuffer.push_command(890, 0, info);

        // Button A Pass [cite: 614-617]
        if (swapState.clicked) {
            ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX, false);
            if (ctx->layerStack->getLayers().size() >= 2) { ctx->layerStack->deferSwap(0, 1); }
        }
        writeBuffer.push_command(900, 0, ButtonPayload{
            .dest_rect = dynamicSwapBounds, .idle_color = {0.12f, 0.25f, 0.15f, 1.0f}, .hover_color = {0.2f, 0.4f, 0.25f, 1.0f},
            .click_color = {0.08f, 0.18f, 0.1f, 1.0f}, .text_color = {1,1,1,1}, .text_scale = 0.32f,
            .is_hovered = swapState.hovered, .is_pressed = swapState.pressed, .no_texture = true, .label = "SWAP BASE LAYERS"
            });

        // Button B Pass [cite: 618-620]
        if (killState.clicked) {
            ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX, false);
            ctx->layerStack->deferDetach(this);
            return;
        }
        writeBuffer.push_command(900, 0, ButtonPayload{
            .dest_rect = dynamicKillBounds, .idle_color = {0.28f, 0.12f, 0.12f, 1.0f}, .hover_color = {0.45f, 0.18f, 0.18f, 1.0f},
            .click_color = {0.18f, 0.08f, 0.08f, 1.0f}, .text_color = {1,1,1,1}, .text_scale = 0.32f,
            .is_hovered = killState.hovered, .is_pressed = killState.pressed, .no_texture = true, .label = "DETACH SELF"
            });

        // TextBox Pass [cite: 621-624]
        TextBoxPayload tb{
            .bounds = dynamicTxtBounds, .background_color = {0.02f, 0.02f, 0.03f, 1.0f},
            .border_color = boxState.focused ? glm::vec4{0.1f, 0.45f, 0.75f, 1.0f} : glm::vec4{0.2f, 0.22f, 0.26f, 1.0f},
            .text_color = {0.95f, 0.95f, 1.0f, 1.0f}, .cursor_index = m_txtCursor, .text_scale = 0.32f, .is_focused = boxState.focused
        };
        snprintf(tb.content, sizeof(tb.content), "%s", m_profileName.c_str());
        writeBuffer.push_command(900, 0, tb);
    }
};

// ============================================================================
// SYSTEM REBUILD BOOTSTRAPPING ENTRY DRIVER
// ============================================================================
int main() {
    try {
        std::cout << "[Engine Verification Run] Initiating testing configuration blueprints...\n";
            Application app("Horizon Engine v2.0 - Complete Subsystem Suite", 1200, 805);
            EngineContext& ctx = app.getContext();

            // Load baseline continuous ambient audio pipeline
            AudioHandle bgmTrack = ctx.audioEngine.loadSound(SFX_PATH "bgm.mp3", true);
            ctx.audioEngine.play(bgmTrack, AudioCategory::Music, true);

            // Mount the comprehensive multi-layered structure
            // Layer 0: Gameplay Simulation Loop 
            ctx.layerStack->pushLayer(std::make_unique<GameplaySimulationLayer>(), &ctx);

            // Layer 1: Persistent Topmost Developer Panel Layout
            ctx.layerStack->pushLayer(std::make_unique<DevDashboardLayer>(), &ctx);

            std::cout << "[Engine Verification Run] Bootstrapping Complete. Activating worker cores.\n";
            app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Core Halt]: " << e.what() << "\n";
            return -1;
    }
    return 0;
}