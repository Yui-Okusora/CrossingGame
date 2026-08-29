#include "Gameplay.hpp"
#include "../Entities/GoalEntity.hpp"
#include "../Entities/BenchEntity.hpp"
#include "../Entities/MovingHazardEntity.hpp"
#include "../Entities/ElevatorCrowdEntity.hpp"
#include "../Entities/StudentPlayerEntity.hpp"
#include "../Entities/TeacherNPCEntity.hpp"
#include "WinLoseLayer.hpp"
#include "PauseMenu.hpp"
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

void GameplayLayer::onAttach(EngineContext* ctx) {
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) m_playerName = *nameOpt;
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) m_currentLevel = *levelOpt;
    if (auto modeOpt = ctx->blackboard.get<int>("gameMode")) m_mode = static_cast<GameMode>(*modeOpt);

    m_currentScore = 0;
    m_elapsedTime = 0.0f;
    m_timerStarted = false;
    ctx->blackboard.set("currentScore", m_currentScore);
    ctx->blackboard.set("elapsedTime", m_elapsedTime);

    // Preload all textures into cache
    m_texSafeZone = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassSafePath.png");
    m_texRoad = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Road/Road.png");
    m_texElevatorRoad = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/ElevatorRoad.png");
    m_texCodeLine = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine.png");
    m_texWater = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/River/River.png");
    m_texBusSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Bus/shortBus-sheet.png");
    m_texBusLong = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Bus/LongBus.png");
    m_texExamPaper = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/ExamPaper/ExamPaper.png");

    m_codeObstacleTextures = {
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-NullPtr.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-OutOfMemory.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-StackOverflow.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-Condition.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-While.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-Missing_.png")
    };

    initLevel(m_currentLevel, ctx);
}

void GameplayLayer::onDetach(EngineContext* ctx) {
    m_scene.clear();
    m_lanes.clear();
    m_elevatorCrowds.clear();
    m_player = nullptr;
}

void GameplayLayer::initLevel(int level, EngineContext* ctx) {
    m_scene.clear();
    m_lanes.clear();
    m_elevatorCrowds.clear();
    m_isGameOver = false;
    m_isLevelComplete = false;
    m_timerStarted = false;
    m_totalLanesSpawned = 0;

    // 1. Offset starting coordinate to bottom edge (805 - 64 = 741)
    m_playerStartY = 741.0f;
    m_deathBorderY = 805.0f;
    m_topGeneratedY = m_playerStartY;

    m_player = m_scene.spawn<StudentPlayerEntity>(ctx, glm::vec2(576.0f, m_playerStartY), this);

    if (m_mode == GameMode::Endless) {
        // Initial infinite seed: Generate 25 procedural lanes upward
        generateEndlessChunk(25, ctx);
    }
    else {
        generateLanesForLevel(level);
        m_goalY = m_lanes.back().yPosition;
        for (size_t i = 0; i < m_lanes.size(); ++i) {
            spawnSingleLaneEntities(m_lanes[i], ctx, (i == m_lanes.size() - 1));
        }
    }

    if (m_player) {
        // 2. Lock camera to Y = 0 at start so no space below Y = 805 is visible
        ctx->cameraPos = glm::vec2(0.0f, 0.0f);
        m_cameraTargetY = 0.0f;
    }
}

void GameplayLayer::generateEndlessChunk(int count, EngineContext* ctx) {
    std::mt19937 rng(1337 + m_totalLanesSpawned * 37);
    std::uniform_real_distribution<float> speedDist(120.0f, 210.0f);
    std::uniform_real_distribution<float> offsetDist(0.0f, 320.0f);
    std::uniform_int_distribution<int> hazardTypeDist(0, 3);
    std::bernoulli_distribution dirDist(0.5);

    const LaneType hazardPool[] = { LaneType::Asphalt, LaneType::ElevatorTile, LaneType::IDELane, LaneType::Water };

    for (int i = 0; i < count; ++i) {
        float currentY = m_topGeneratedY - (m_totalLanesSpawned == 0 ? 0.0f : 64.0f);
        m_topGeneratedY = currentY;

        LaneType type = (m_totalLanesSpawned == 0 || m_totalLanesSpawned % 5 == 0)
            ? LaneType::SafeZone
            : hazardPool[hazardTypeDist(rng)];

        LaneData lane{
            .yPosition = currentY,
            .height = 64.0f,
            .type = type,
            .moveSpeed = (type == LaneType::SafeZone) ? 0.0f : speedDist(rng),
            .direction = dirDist(rng) ? 1 : -1,
            .spawnXOffset = offsetDist(rng),
            .seed = static_cast<uint32_t>(rng())
        };

        m_lanes.push_back(lane);
        spawnSingleLaneEntities(lane, ctx, false);
        m_totalLanesSpawned++;
    }
}

void GameplayLayer::generateLanesForLevel(int level) {
    std::mt19937 rng(1337 + level * 100);
    std::uniform_real_distribution<float> speedDist(110.0f + level * 18.0f, 150.0f + level * 28.0f);
    std::uniform_real_distribution<float> offsetDist(0.0f, 320.0f);
    std::uniform_int_distribution<int> clusterDist(1, (level >= 3) ? 3 : 2);
    std::uniform_int_distribution<int> hazardTypeDist(0, 3);
    std::bernoulli_distribution dirDist(0.5);

    int totalLanes = 10 + (level * 3);
    float startY = 741.0f;
    float laneHeight = 64.0f;

    m_lanes.clear();
    m_lanes.push_back({ startY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng() });

    const LaneType hazardPool[] = { LaneType::Asphalt, LaneType::ElevatorTile, LaneType::IDELane, LaneType::Water };

    int currentLaneIdx = 1;
    int consecutiveHazards = 0;
    int lastDirection = 1;
    int sameDirStreak = 0;

    while (currentLaneIdx < totalLanes - 1) {
        if (consecutiveHazards >= 4) {
            float currentY = startY - (currentLaneIdx * laneHeight);
            m_lanes.push_back({ currentY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng() });
            consecutiveHazards = 0;
            currentLaneIdx++;
            continue;
        }

        LaneType selectedType = hazardPool[hazardTypeDist(rng)];
        int clusterSize = clusterDist(rng);

        for (int c = 0; c < clusterSize && currentLaneIdx < totalLanes - 1; ++c) {
            float currentY = startY - (currentLaneIdx * laneHeight);
            float speed = speedDist(rng);

            int dir = dirDist(rng) ? 1 : -1;
            if (dir == lastDirection) {
                if (++sameDirStreak >= 2) { dir = -dir; sameDirStreak = 1; }
            }
            else {
                sameDirStreak = 1;
            }
            lastDirection = dir;

            m_lanes.push_back({ currentY, laneHeight, selectedType, speed, dir, offsetDist(rng), rng(), 0.0f, static_cast<uint8_t>(c % 3) });
            consecutiveHazards++;
            currentLaneIdx++;
        }
    }

    float goalY = startY - ((totalLanes - 1) * laneHeight);
    m_lanes.push_back({ goalY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng() });
}

void GameplayLayer::spawnSingleLaneEntities(const LaneData& lane, EngineContext* ctx, bool isGoal) {
    std::mt19937 laneRng(lane.seed);

    if (isGoal) {
        m_scene.spawn<GoalEntity>(ctx, glm::vec2(0.0f, lane.yPosition));
        return;
    }

    if (lane.type == LaneType::SafeZone && lane.yPosition < 700.0f) {
        std::uniform_int_distribution<int> benchCountDist(1, 3);
        int benchCount = benchCountDist(laneRng);

        std::vector<int> validTileCols(16);
        std::iota(validTileCols.begin(), validTileCols.end(), 1);
        std::shuffle(validTileCols.begin(), validTileCols.end(), laneRng);

        for (int b = 0; b < benchCount; ++b) {
            m_scene.spawn<BenchEntity>(ctx, glm::vec2(validTileCols[b] * 64.0f, lane.yPosition));
        }

        std::uniform_int_distribution<int> buffTypeDist(0, 2);
        auto* teacher = m_scene.spawn<TeacherNPCEntity>(
            ctx,
            glm::vec2(validTileCols[benchCount] * 64.0f, lane.yPosition),
            glm::vec2(validTileCols[benchCount + 1] * 64.0f, lane.yPosition),
            static_cast<TeacherBuffType>(buffTypeDist(laneRng))
        );
        teacher->setPlayer(m_player);
    }
    else if (lane.type == LaneType::Asphalt) {
        int busCount = std::uniform_int_distribution<int>(1, 2)(laneRng);
        float sector = 1100.0f / busCount;

        for (int b = 0; b < busCount; ++b) {
            float x = std::fmod((b * sector) + lane.spawnXOffset, 1100.0f);
            auto* bus = m_scene.spawn<MovingHazardEntity>(
                ctx, glm::vec2(x, lane.yPosition), lane.moveSpeed, lane.direction,
                glm::vec2(130.0f, 48.0f), CollisionLayer::Layer_Enemy, CollisionLayer::Layer_Player,
                false, 8.0f, glm::vec4(1.0f), 40
            );
            bus->animator.addAnimation("drive", AnimationClip{ m_texBusSheet, { 4, 1 }, 0, 3, 0.1f, true });
            bus->animator.play("drive");
        }
    }
    else if (lane.type == LaneType::ElevatorTile) {
        auto* crowd = m_scene.spawn<ElevatorCrowdEntity>(ctx, glm::vec2(0.0f, lane.yPosition), lane.moveSpeed, lane.direction);
        m_elevatorCrowds.push_back(crowd);
    }
    else if (lane.type == LaneType::IDELane) {
        int count = std::uniform_int_distribution<int>(2, 3)(laneRng);
        float sector = 1100.0f / count;

        for (int c = 0; c < count; ++c) {
            float x = std::fmod((c * sector) + lane.spawnXOffset, 1100.0f);
            TextureHandle codeTex = m_codeObstacleTextures[c % m_codeObstacleTextures.size()];
            auto* codeStream = m_scene.spawn<MovingHazardEntity>(
                ctx, glm::vec2(x, lane.yPosition), lane.moveSpeed, lane.direction,
                glm::vec2(110.0f, 40.0f), CollisionLayer::Layer_Enemy, CollisionLayer::Layer_Player,
                false, 12.0f, glm::vec4(1.0f), 40
            );
            codeStream->animator.addAnimation("stream", AnimationClip{ codeTex, { 1, 1 }, 0, 0, 1.0f, false });
            codeStream->animator.play("stream");
        }
    }
    else if (lane.type == LaneType::Water) {
        int count = std::uniform_int_distribution<int>(2, 4)(laneRng);
        float sector = 1050.0f / count;

        for (int l = 0; l < count; ++l) {
            float x = std::fmod((l * sector) + lane.spawnXOffset, 1050.0f);
            auto* platform = m_scene.spawn<MovingHazardEntity>(
                ctx, glm::vec2(x, lane.yPosition), lane.moveSpeed, lane.direction,
                glm::vec2(170.0f, 50.0f), CollisionLayer::Layer_TriggerVolume, CollisionLayer::Layer_Player,
                true, 7.0f, glm::vec4(1.0f), 25
            );
            platform->animator.addAnimation("float", AnimationClip{ m_texExamPaper, { 1, 1 }, 0, 0, 1.0f, false });
            platform->animator.play("float");
        }
    }
}

void GameplayLayer::updateWaterAnimation(float dt) {
    int totalFrames = static_cast<int>(m_waterAtlasDims.x * m_waterAtlasDims.y);
    if (totalFrames <= 0) return;

    for (auto& lane : m_lanes) {
        if (lane.type != LaneType::Water) continue;
        lane.waterAnimTimer += dt;
        if (lane.waterAnimTimer >= m_waterFrameDuration) {
            lane.waterAnimTimer -= m_waterFrameDuration;
            lane.waterAnimFrame = static_cast<uint8_t>((lane.waterAnimFrame + 1) % totalFrames);
        }
    }
}

void GameplayLayer::updateElevatorSignals(float dt) {
    size_t crowdIdx = 0;
    for (auto& lane : m_lanes) {
        if (lane.type != LaneType::ElevatorTile) continue;

        lane.signalTimer += dt;
        if (lane.signalPhase == 0 && lane.signalTimer >= 3.0f) {
            lane.signalPhase = 1; lane.signalTimer = 0.0f;
        }
        else if (lane.signalPhase == 1 && lane.signalTimer >= 1.2f) {
            lane.signalPhase = 2; lane.signalTimer = 0.0f;
        }
        else if (lane.signalPhase == 2) {
            bool finished = (crowdIdx < m_elevatorCrowds.size() && m_elevatorCrowds[crowdIdx])
                ? m_elevatorCrowds[crowdIdx]->hasFinishedCrossing() : true;
            if (finished) {
                lane.signalPhase = 0; lane.signalTimer = 0.0f;
            }
        }

        if (crowdIdx < m_elevatorCrowds.size() && m_elevatorCrowds[crowdIdx]) {
            m_elevatorCrowds[crowdIdx]->setSignal(static_cast<SignalState>(lane.signalPhase));
            crowdIdx++;
        }
    }
}

void GameplayLayer::handleEvent(const EngineEvent& event, EngineContext* ctx) {
    if (std::holds_alternative<KeyEvent>(event)) {
        auto ev = std::get<KeyEvent>(event);
        if (ev.action == GLFW_PRESS) {
            if (ev.key == GLFW_KEY_P || ev.key == GLFW_KEY_ESCAPE) {
                ctx->layerStack->deferAttach(std::make_unique<PauseMenuLayer>());
            }
            if (ev.key == GLFW_KEY_K) triggerGameOver(ctx);
        }
    }
}

void GameplayLayer::update(double dt, EngineContext* ctx) {
    if (m_isGameOver || m_isLevelComplete) return;

    float fDt = static_cast<float>(dt);

    // Timer begins exclusively after player initiates their first move
    if (!m_timerStarted && m_player && m_player->hasStartedFirstMove()) {
        m_timerStarted = true;
    }

    if (m_timerStarted) {
        m_elapsedTime += fDt;
        ctx->blackboard.set("elapsedTime", m_elapsedTime);

        // Endless Mode: Upward creeping death border
        if (m_mode == GameMode::Endless) {
            float speed = m_deathBorderSpeed + (m_currentLevel * 3.0f);
            m_deathBorderY -= speed * fDt;

            if (m_player && m_player->position.y >= m_deathBorderY) {
                triggerGameOver(ctx);
                return;
            }

            // Generate infinite lanes ahead as player progresses[cite: 9, 10]
            if (m_player && m_player->position.y < m_topGeneratedY + 800.0f) {
                generateEndlessChunk(15, ctx);
            }

            int distanceScore = static_cast<int>((m_playerStartY - m_player->position.y) / 64.0f) * 10;
            int timeScore = static_cast<int>(m_elapsedTime * 15.0f);
            int multiplier = m_player ? m_player->getScoreMultiplier() : 1;
            m_currentScore = (timeScore + distanceScore) * multiplier;
            ctx->blackboard.set("currentScore", m_currentScore);
        }
    }

    updateElevatorSignals(fDt);
    updateWaterAnimation(fDt);

    if (m_player) {
        m_player->isOnWaterLane = false;
        m_player->currentWaterVel = glm::vec2(0.0f);

        for (const auto& lane : m_lanes) {
            if (lane.type == LaneType::Water &&
                m_player->position.y >= lane.yPosition - 10.0f &&
                m_player->position.y <= lane.yPosition + lane.height - 10.0f) {
                m_player->isOnWaterLane = true;
                m_player->currentWaterVel = glm::vec2(lane.moveSpeed * static_cast<float>(lane.direction), 0.0f);
                break;
            }
        }
    }

    m_scene.fixedUpdate(dt, ctx);

    if (m_player) {
        m_player->postPhysicsUpdate(fDt, ctx);

        // Campaign Mode win condition at Goal Line[cite: 9, 10]
        if (m_mode == GameMode::Campaign && m_player->position.y <= m_goalY + 20.0f) {
            triggerLevelComplete(ctx);
            return;
        }

        m_cameraTargetY = (std::min)(0.0f, m_player->position.y - 450.0f);
        float factor = 1.0f - std::exp(-6.0f * fDt);
        ctx->cameraPos.y = glm::mix(ctx->cameraPos.y, m_cameraTargetY, factor);

        if (m_mode == GameMode::Campaign && m_timerStarted) {
            int distanceScore = static_cast<int>((m_playerStartY - m_player->position.y) / 64.0f) * 10;
            int multiplier = m_player->getScoreMultiplier();
            if (distanceScore > m_currentScore) {
                m_currentScore = distanceScore * multiplier;
                ctx->blackboard.set("currentScore", m_currentScore);
            }
        }
    }
}

void GameplayLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    for (const auto& lane : m_lanes) {
        if (lane.type == LaneType::Water) {
            uint32_t col = lane.waterAnimFrame % m_waterAtlasDims.x;
            uint32_t row = lane.waterAnimFrame / m_waterAtlasDims.x;
            writeBuffer.push_command(10, 0, RectPayload{
                .dest_rect = { 0.0f, lane.yPosition, 1200.0f, lane.height },
                .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                .texture = m_texWater,
                .atlas_dimensions = m_waterAtlasDims,
                .atlas_pos = { col, row },
                .no_texture = false,
                .is_world_space = true
                });
            continue;
        }

        TextureHandle tex = (lane.type == LaneType::SafeZone) ? m_texSafeZone :
            (lane.type == LaneType::Asphalt) ? m_texRoad :
            (lane.type == LaneType::ElevatorTile) ? m_texElevatorRoad : m_texCodeLine;

        writeBuffer.push_command(10, 0, RectPayload{
            .dest_rect = { 0.0f, lane.yPosition, 1200.0f, lane.height },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = tex,
            .no_texture = false,
            .is_world_space = true
            });

        if (lane.type == LaneType::ElevatorTile) {
            glm::vec4 color = (lane.signalPhase == 0) ? glm::vec4{ 0.9f, 0.1f, 0.1f, 1.0f } :
                (lane.signalPhase == 1) ? glm::vec4{ 0.9f, 0.8f, 0.1f, 1.0f } : glm::vec4{ 0.1f, 0.9f, 0.2f, 1.0f };

            writeBuffer.push_command(15, 0, RectPayload{ .dest_rect = { 15.0f, lane.yPosition + 12.0f, 24.0f, 40.0f }, .color = color, .no_texture = true, .is_world_space = true });
            writeBuffer.push_command(15, 0, RectPayload{ .dest_rect = { 1161.0f, lane.yPosition + 12.0f, 24.0f, 40.0f }, .color = color, .no_texture = true, .is_world_space = true });
        }
    }

    // Render Endless Mode Red Danger Fog & Creeping Death Border[cite: 9, 10]
    if (m_mode == GameMode::Endless) {
        writeBuffer.push_command(80, 0, RectPayload{
            .dest_rect = { 0.0f, m_deathBorderY, 1200.0f, 800.0f },
            .color = { 0.85f, 0.1f, 0.1f, 0.45f },
            .no_texture = true,
            .is_world_space = true
            });

        writeBuffer.push_command(85, 0, RectPayload{
            .dest_rect = { 0.0f, m_deathBorderY - 4.0f, 1200.0f, 8.0f },
            .color = { 1.0f, 0.2f, 0.2f, 0.95f },
            .no_texture = true,
            .is_world_space = true
            });
    }

    m_scene.populateRenderStream(writeBuffer, ctx);
}

void GameplayLayer::triggerGameOver(EngineContext* ctx) {
    if (m_isGameOver) return;
    m_isGameOver = true;
    ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>(false));
}

void GameplayLayer::triggerLevelComplete(EngineContext* ctx) {
    if (m_isLevelComplete) return;

    if (m_currentLevel < m_maxLevel) {
        int timeBonus = (std::max)(0, static_cast<int>(1000.0f - (m_elapsedTime * 20.0f)));
        m_currentScore += timeBonus;
        ctx->blackboard.set("currentScore", m_currentScore);

        m_currentLevel++;
        ctx->blackboard.set("currentLevel", m_currentLevel);
        initLevel(m_currentLevel, ctx);
    }
    else {
        m_isLevelComplete = true;
        ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>(true));
    }
}