#include <Engine/Engine.hpp>
#include <Engine/Core/Scene.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

enum TestIDs : uint32_t {
    ID_PauseBtn = 101,
    ID_VolumeSld = 102,
    ID_DevSwapBtn = 201,
    ID_DevKillBtn = 202,
    ID_DevTxtBox = 203
};

// Lane Types for Crossy Road World Generation
enum class LaneType { Grass, Road, Water };

struct LaneInfo {
    int laneIndex = 0;
    float yPos = 0.0f;
    LaneType type = LaneType::Grass;
};

// ============================================================================
// CROSSY ROAD GAMEPLAY ENTITIES
// ============================================================================

// 1. Coin Pickup Entity
class CoinEntity : public Entity2D {
private:
    AudioHandle m_coinSFX;

public:
    CoinEntity(glm::vec2 pos) {
        position = pos + glm::vec2(16.0f, 16.0f); // Center inside 48x48 tile
        size = { 16.0f, 16.0f };
        isTrigger = true;
        layer = CollisionLayer::Layer_TriggerVolume;
        mask = CollisionLayer::Layer_Player;
    }

    void onAttach(EngineContext* ctx) {
        m_coinSFX = ctx->audioEngine.loadSound(SFX_PATH "click.mp3", false);
    }

    void onTrigger(const CollisionInfo& trigger, EngineContext* ctx) override {
        ctx->audioEngine.play(m_coinSFX, AudioCategory::InteractSFX, false);

        int coins = ctx->blackboard.get<int>("coins").value_or(0);
        ctx->blackboard.set("coins", coins + 1);

        active = false; // Collect item
    }

    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override {
        writeBuffer.push_command(15, 0, RectPayload{
            .dest_rect = { renderPos.x, renderPos.y, size.x, size.y },
            .color = { 1.0f, 0.85f, 0.1f, 1.0f }, // Gold
            .no_texture = true,
            .is_world_space = true
            });
    }
};

// 2. Moving Vehicle / Car Entity (Hazard)
class VehicleEntity : public Entity2D {
private:
    float m_minX = -100.0f;
    float m_maxX = 1300.0f;

public:
    VehicleEntity(glm::vec2 pos, float speed, glm::vec2 vehicleSize = { 80.0f, 36.0f }) {
        position = pos;
        size = vehicleSize;
        velocity = { speed, 0.0f };
        layer = CollisionLayer::Layer_Obstacle;
        mask = CollisionLayer::Layer_Player;
    }

    void onUpdate(float dt, EngineContext* ctx) override {
        // Wrap vehicle around screen boundaries endlessly
        if (velocity.x > 0.0f && position.x > m_maxX) {
            position.x = m_minX;
        }
        else if (velocity.x < 0.0f && position.x < m_minX) {
            position.x = m_maxX;
        }
    }

    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override {
        writeBuffer.push_command(12, 0, RectPayload{
            .dest_rect = { renderPos.x, renderPos.y, size.x, size.y },
            .color = { 0.9f, 0.2f, 0.2f, 1.0f }, // Car Red
            .no_texture = true,
            .is_world_space = true
            });
    }
};

// 3. Player Chicken Entity (Grid Hopper)
class ChickenPlayerEntity : public Entity2D {
private:
    AudioHandle m_hitSFX;
    constexpr static float TILE_SIZE = 48.0f;

    int m_maxLaneReached = 0;
    bool m_isDead = false;

    // Hop Animation State
    float m_hopAnimTimer = 0.0f;
    constexpr static float HOP_DURATION = 0.10f; // 100ms hop animation
    glm::vec2 m_targetPos{ 0.0f, 0.0f };
    glm::vec2 m_startPos{ 0.0f, 0.0f };

public:
    ChickenPlayerEntity(glm::vec2 pos) {
        position = pos;
        m_targetPos = pos;
        m_startPos = pos;
        size = { 36.0f, 36.0f };
        layer = CollisionLayer::Layer_Player;
        mask = CollisionLayer::Layer_Obstacle | CollisionLayer::Layer_TriggerVolume;
    }

    void onAttach(EngineContext* ctx) {
        m_hitSFX = ctx->audioEngine.loadSound(SFX_PATH "hit.mp3", false);
    }

    void onUpdate(float dt, EngineContext* ctx) override {
        if (m_isDead) return;

        // 1. Process Discrete Grid Input
        glm::vec2 hopDir{ 0.0f, 0.0f };
        if (ctx->input.isKeyJustPressed(GLFW_KEY_W) || ctx->input.isKeyJustPressed(GLFW_KEY_UP))    hopDir.y -= 1.0f; // Hop Up (Forward)
        if (ctx->input.isKeyJustPressed(GLFW_KEY_S) || ctx->input.isKeyJustPressed(GLFW_KEY_DOWN))  hopDir.y += 1.0f; // Hop Down
        if (ctx->input.isKeyJustPressed(GLFW_KEY_A) || ctx->input.isKeyJustPressed(GLFW_KEY_LEFT))  hopDir.x -= 1.0f; // Hop Left
        if (ctx->input.isKeyJustPressed(GLFW_KEY_D) || ctx->input.isKeyJustPressed(GLFW_KEY_RIGHT)) hopDir.x += 1.0f; // Hop Right

        if (glm::length(hopDir) > 0.0f && m_hopAnimTimer <= 0.0f) {
            m_startPos = position;
            m_targetPos = position + (hopDir * TILE_SIZE);
            m_targetPos.x = glm::clamp(m_targetPos.x, 60.0f, 1140.0f - size.x);
            m_hopAnimTimer = HOP_DURATION;

            // Track high score lane
            int currentLane = static_cast<int>(-m_targetPos.y / TILE_SIZE);
            if (currentLane > m_maxLaneReached) {
                m_maxLaneReached = currentLane;
                ctx->blackboard.set("score", m_maxLaneReached);
            }
        }

        // 2. Interpolate Discrete Grid Hop Position
        if (m_hopAnimTimer > 0.0f) {
            m_hopAnimTimer -= dt;
            float t = 1.0f - (m_hopAnimTimer / HOP_DURATION);
            t = glm::clamp(t, 0.0f, 1.0f);
            position = glm::mix(m_startPos, m_targetPos, t);
        }
        else {
            position = m_targetPos;
        }
    }

    void onCollision(const CollisionInfo& hit, EngineContext* ctx) override {
        if (!m_isDead && (hit.targetLayer & CollisionLayer::Layer_Obstacle)) {
            ctx->audioEngine.play(m_hitSFX, AudioCategory::GameplaySFX, false);
            m_isDead = true;
            ctx->blackboard.set("gameOver", true);
        }
    }

    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override {
        // Visual Hop Stretch Scaling Effect
        glm::vec2 renderSize = size;
        if (m_hopAnimTimer > 0.0f) {
            float progress = (m_hopAnimTimer / HOP_DURATION);
            renderSize.y += std::sin(progress * 3.14159f) * 12.0f; // Stretch upwards mid-hop
        }

        glm::vec4 color = m_isDead ? glm::vec4{ 0.3f, 0.3f, 0.3f, 1.0f } : glm::vec4{ 0.95f, 0.95f, 0.95f, 1.0f };

        // Render Chicken Body
        writeBuffer.push_command(20, 0, RectPayload{
            .dest_rect = { renderPos.x, renderPos.y, renderSize.x, renderSize.y },
            .color = color,
            .no_texture = true,
            .is_world_space = true
            });

        // Render Yellow Beak Accent
        if (!m_isDead) {
            writeBuffer.push_command(21, 0, RectPayload{
                .dest_rect = { renderPos.x + 12.0f, renderPos.y - 4.0f, 12.0f, 8.0f },
                .color = { 1.0f, 0.6f, 0.0f, 1.0f },
                .no_texture = true,
                .is_world_space = true
                });
        }
    }

    bool isDead() const noexcept { return m_isDead; }
};

// ============================================================================
// LAYER 1: PAUSE MODAL OVERLAY
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

    [[nodiscard]] bool blocksEvents() const noexcept override { return true; }
    [[nodiscard]] bool blocksUpdates() const noexcept override { return true; }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override {
        if (std::holds_alternative<KeyEvent>(event)) {
            auto ev = std::get<KeyEvent>(event);
            if (ev.key == GLFW_KEY_P && ev.action == GLFW_PRESS) {
                ctx->layerStack->deferDetach(this);
            }
        }
    }

    void update(double dt, EngineContext* ctx) override {}

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override {
        writeBuffer.push_command(800, 0, RectPayload{ .dest_rect = {0, 0, 1200, 805}, .color = {0.0f, 0.0f, 0.0f, 0.65f}, .no_texture = true });
        writeBuffer.push_command(810, 0, RectPayload{ .dest_rect = m_panelBounds, .color = {0.11f, 0.11f, 0.14f, 1.0f}, .no_texture = true });

        if (ctx->ui.Button(writeBuffer, ctx, ID_PauseBtn, m_resumeBtn, "RESUME GAME", 24.0f)) {
            ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX, false);
            ctx->layerStack->deferDetach(this);
            return;
        }

        ctx->ui.Slider(writeBuffer, ctx, ID_VolumeSld, m_sliderTrack, m_localVolCache);
        ctx->audioEngine.setCategoryVolume(AudioCategory::Master, m_localVolCache);

        TextPayload txt;
        txt.color = { 0.8f, 0.8f, 0.85f, 1.0f };
        txt.scale = 22.0f;
        txt.position = { m_panelBounds.x + 20.0f, m_panelBounds.y + 200.0f };
        snprintf(txt.text_content, sizeof(txt.text_content), "Master Volume: %d%%", static_cast<int>(m_localVolCache * 100.0f));
        writeBuffer.push_command(830, 0, txt);
    }
};

// ============================================================================
// LAYER 2: CROSSY ROAD GAMEPLAY LAYER
// ============================================================================
class CrossyRoadGameplayLayer : public IEngineLayer {
private:
    Scene2D m_scene;
    ChickenPlayerEntity* m_player = nullptr;
    std::vector<LaneInfo> m_lanes;
    int m_highestLaneGenerated = 0;
    constexpr static float TILE_SIZE = 48.0f;

    void generateLane(EngineContext* ctx, int laneIdx) {
        float yPos = -static_cast<float>(laneIdx) * TILE_SIZE;
        LaneType type = (laneIdx <= 2) ? LaneType::Grass : (rand() % 100 < 65 ? LaneType::Road : LaneType::Grass);

        m_lanes.push_back({ laneIdx, yPos, type });

        if (type == LaneType::Road) {
            float speed = 160.0f + static_cast<float>(rand() % 220);
            if (rand() % 2 == 0) speed *= -1.0f; // Randomize left or right traffic direction

            // Spawn 2 vehicles per road lane
            m_scene.spawn<VehicleEntity>(glm::vec2{ 100.0f + (rand() % 300), yPos + 6.0f }, speed);
            m_scene.spawn<VehicleEntity>(glm::vec2{ 600.0f + (rand() % 300), yPos + 6.0f }, speed);
        }
        else if (type == LaneType::Grass && rand() % 100 < 35) {
            // Spawn collectible coin on grass lanes
            float randomX = static_cast<float>(100 + (rand() % 900));
            m_scene.spawn<CoinEntity>(glm::vec2{ randomX, yPos })->onAttach(ctx);
        }
    }

public:
    void onAttach(EngineContext* ctx) override {
        m_scene.clear();
        m_lanes.clear();
        m_highestLaneGenerated = 0;

        ctx->blackboard.set("score", 0);
        ctx->blackboard.set("coins", 0);
        ctx->blackboard.set("gameOver", false);

        // Spawn initial 30 lanes
        for (int i = -4; i < 26; ++i) {
            generateLane(ctx, i);
            m_highestLaneGenerated = i;
        }

        // Spawn Player Chicken at starting grass lane
        m_player = m_scene.spawn<ChickenPlayerEntity>(glm::vec2{ 576.0f, 0.0f });
        m_player->onAttach(ctx);
    }

    void onDetach(EngineContext* ctx) override {
        m_scene.clear();
    }

    void handleEvent(const EngineEvent& event, EngineContext* ctx) override {
        if (std::holds_alternative<KeyEvent>(event)) {
            auto ev = std::get<KeyEvent>(event);
            if (ev.key == GLFW_KEY_P && ev.action == GLFW_PRESS) {
                ctx->layerStack->deferAttach(std::make_unique<PauseMenuLayer>());
            }
            if (ev.key == GLFW_KEY_R && ev.action == GLFW_PRESS && m_player && m_player->isDead()) {
                onAttach(ctx); // Reset game on death
            }
        }
    }

    void update(double dt, EngineContext* ctx) override {
        m_scene.fixedUpdate(dt, ctx);

        if (m_player) {
            // Dynamically generate new lanes ahead as player advances forward
            int playerCurrentLane = static_cast<int>(-m_player->position.y / TILE_SIZE);
            while (m_highestLaneGenerated < playerCurrentLane + 25) {
                m_highestLaneGenerated++;
                generateLane(ctx, m_highestLaneGenerated);
            }

            // Smooth Camera Follow tracking forward player progress
            glm::vec2 targetCam = glm::vec2(0.0f, m_player->position.y - 500.0f);
            ctx->cameraPos.y = Utils::lerp2(ctx->cameraPos, targetCam, 0.10f).y;
        }
    }

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override {
        // 1. Render World Lane Strips
        for (const auto& lane : m_lanes) {
            glm::vec4 laneColor = (lane.type == LaneType::Grass) ?
                glm::vec4{ 0.3f, 0.7f, 0.25f, 1.0f } : glm::vec4{ 0.18f, 0.18f, 0.2f, 1.0f };

            writeBuffer.push_command(0, 0, RectPayload{
                .dest_rect = { 0.0f, lane.yPos, 1200.0f, TILE_SIZE },
                .color = laneColor,
                .no_texture = true,
                .is_world_space = true
                });
        }

        // 2. Render Scene Entities (Vehicles, Coins, Player)
        m_scene.populateRenderStream(writeBuffer, ctx);

        // 3. HUD Display (Fixed Screen-Space UI)
        int score = ctx->blackboard.get<int>("score").value_or(0);
        int coins = ctx->blackboard.get<int>("coins").value_or(0);
        bool isGameOver = ctx->blackboard.get<bool>("gameOver").value_or(false);

        TextPayload hudTxt;
        hudTxt.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        hudTxt.scale = 28.0f;
        hudTxt.position = { 30.0f, 35.0f };
        snprintf(hudTxt.text_content, sizeof(hudTxt.text_content), "TOP LANE: %d  |  COINS: %d", score, coins);
        writeBuffer.push_command(500, 0, hudTxt);

        if (isGameOver) {
            TextPayload overTxt;
            overTxt.color = { 1.0f, 0.2f, 0.2f, 1.0f };
            overTxt.scale = 36.0f;
            overTxt.position = { 600.0f, 350.0f };
            overTxt.showInCenter = true;
            snprintf(overTxt.text_content, sizeof(overTxt.text_content), "SQUASHED! PRESS 'R' TO RESTART");
            writeBuffer.push_command(600, 0, overTxt);
        }
    }
};

// ============================================================================
// DEVELOPER DASHBOARD LAYER
// ============================================================================
class DevDashboardLayer : public IEngineLayer {
private:
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

    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override {
        glm::vec2 canvas{ 1200.0f, 805.0f };

        float barHeight = 105.0f;
        glm::vec4 dynamicRibbon{ 0.0f, canvas.y - barHeight, canvas.x, barHeight };
        writeBuffer.push_command(880, 0, RectPayload{ .dest_rect = dynamicRibbon, .color = {0.05f, 0.05f, 0.07f, 1.0f}, .no_texture = true });

        glm::vec4 dynamicSwapBounds = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(200, 45), glm::vec2(30, 30), UIAnchor::BottomLeft);
        glm::vec4 dynamicKillBounds = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(200, 45), glm::vec2(250, 30), UIAnchor::BottomLeft);
        glm::vec4 dynamicTxtBounds = UIAnchorEngine::CalculateBounds(canvas, glm::vec2(250, 45), glm::vec2(470, 30), UIAnchor::BottomLeft);

        TextPayload info;
        info.color = { 0.5f, 0.7f, 0.9f, 1.0f };
        info.scale = 22.0f;
        info.position = { 30.f, 75.f };
        snprintf(info.text_content, sizeof(info.text_content), "Active Stack Size: %zu | Mouse Screen Coords: {%.1f, %.1f}",
            ctx->layerStack->getLayers().size(), ctx->input.getMousePosition().x, ctx->input.getMousePosition().y);
        writeBuffer.push_command(890, 0, info);

        if (ctx->ui.Button(writeBuffer, ctx, ID_DevSwapBtn, dynamicSwapBounds, "SWAP BASE LAYERS", 24.0f)) {
            ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX, false);
            if (ctx->layerStack->getLayers().size() >= 2) { ctx->layerStack->deferSwap(0, 1); }
        }

        if (ctx->ui.Button(writeBuffer, ctx, ID_DevKillBtn, dynamicKillBounds, "DETACH SELF", 24.0f)) {
            ctx->audioEngine.play(m_clickSFX, AudioCategory::InteractSFX, false);
            ctx->layerStack->deferDetach(this);
            return;
        }

        ctx->ui.TextBox(writeBuffer, ctx, ID_DevTxtBox, dynamicTxtBounds, m_profileName, m_txtCursor, 24.0f);
    }
};

// ============================================================================
// SYSTEM ENTRY DRIVER
// ============================================================================
int main() {
    try {
        std::cout << "[Horizon Engine] Initializing Crossy Road Simulation Suite...\n";
        Application app("Horizon Engine v2.0 - Crossy Road Test", 1200, 805);
        EngineContext& ctx = app.getContext();

        AudioHandle bgmTrack = ctx.audioEngine.loadSound(SFX_PATH "bgm.mp3", true);
        ctx.audioEngine.play(bgmTrack, AudioCategory::Music, true);

        ctx.layerStack->pushLayer(std::make_unique<CrossyRoadGameplayLayer>(), &ctx);
        ctx.layerStack->pushLayer(std::make_unique<DevDashboardLayer>(), &ctx);

        std::cout << "[Horizon Engine] Bootstrapping Complete. Activating worker cores.\n";
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Core Halt]: " << e.what() << "\n";
        return -1;
    }
    return 0;
}