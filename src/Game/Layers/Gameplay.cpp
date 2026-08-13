#include "Gameplay.hpp"
#include "../Entities/GoalEntity.hpp"
#include "../Entities/BenchEntity.hpp"
#include "../Entities/BusEntity.hpp"
#include "../Entities/ElevatorCrowdEntity.hpp"
#include "../Entities/CodeStreamEntity.hpp"
#include "../Entities/LogPlatformEntity.hpp"
#include "../Entities/StudentPlayerEntity.hpp"
#include "../Entities/TeacherNPCEntity.hpp"
#include "WinLoseLayer.hpp"
#include "PauseMenu.hpp"
#include <iostream>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

void GameplayLayer::onAttach(EngineContext* ctx) {
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) {
        m_playerName = *nameOpt;
    }
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) {
        m_currentLevel = *levelOpt;
    }
    else {
        ctx->blackboard.set("currentLevel", m_currentLevel);
    }

    m_currentScore = 0;
    ctx->blackboard.set("currentScore", m_currentScore);

    m_texWater = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/River/River.png");


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

    generateLanesForLevel(level);

    m_playerStartY = m_lanes.front().yPosition;
    m_goalY = m_lanes.back().yPosition;

    spawnLaneEntities(ctx);

    if (m_player) {
        ctx->cameraPos = glm::vec2(0.0f, m_player->position.y - 450.0f);
        m_cameraTargetY = ctx->cameraPos.y;
    }
}

void GameplayLayer::generateLanesForLevel(int level) {
    // Seed main PRNG deterministically per level
    std::mt19937 rng(1337 + level * 100);
    std::uniform_real_distribution<float> speedDist(110.0f + level * 18.0f, 150.0f + level * 28.0f);
    std::uniform_real_distribution<float> offsetDist(0.0f, 320.0f);
    std::uniform_int_distribution<int> clusterDist(1, (level >= 3) ? 3 : 2);
    std::uniform_int_distribution<int> hazardTypeDist(0, 3);
    std::bernoulli_distribution dirDist(0.5);

    int totalLanes = 20 + (level * 3);
    float startY = 700.0f;
    float laneHeight = 64.0f;

    m_lanes.clear();

    // Bottom Start Zone (Safe)
    m_lanes.push_back({ startY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng(), 0.0f, 0 });

    const LaneType hazardPool[] = {
        LaneType::Asphalt,
        LaneType::ElevatorTile,
        LaneType::IDELane,
        LaneType::Water
    };

    int currentLaneIdx = 1;
    int consecutiveHazards = 0;
    int lastDirection = 1;
    int sameDirStreak = 0;

    // Procedurally generate map with grouped hazard clusters
    while (currentLaneIdx < totalLanes - 1) {
        if (consecutiveHazards >= 4) {
            float currentY = startY - (currentLaneIdx * laneHeight);
            m_lanes.push_back({ currentY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng(), 0.0f, 0 });
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
                sameDirStreak++;
                if (sameDirStreak >= 2) {
                    dir = -dir;
                    sameDirStreak = 1;
                }
            }
            else {
                sameDirStreak = 1;
            }
            lastDirection = dir;

            float xOffset = offsetDist(rng);

            m_lanes.push_back({
                currentY,
                laneHeight,
                selectedType,
                speed,
                dir,
                xOffset,
                rng(), // Assign unique lane seed
                0.0f,
                static_cast<uint8_t>(c % 3)
                });

            consecutiveHazards++;
            currentLaneIdx++;
        }
    }

    // Top Goal Zone (Safe)
    float goalY = startY - ((totalLanes - 1) * laneHeight);
    m_lanes.push_back({ goalY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng(), 0.0f, 0 });
}

void GameplayLayer::spawnLaneEntities(EngineContext* ctx) {
    // --- FIX: Spawn player first to pass reference to teachers ---
    m_player = m_scene.spawn<StudentPlayerEntity>(ctx, glm::vec2(576.0f, m_playerStartY), this);

    for (size_t i = 0; i < m_lanes.size(); ++i) {
        const auto& lane = m_lanes[i];
        std::mt19937 laneRng(lane.seed);

        if (i == m_lanes.size() - 1) {
            m_scene.spawn<GoalEntity>(ctx, glm::vec2(0.0f, lane.yPosition));
        }
        else if (lane.type == LaneType::SafeZone && i > 0) {
            std::uniform_int_distribution<int> benchCountDist(1, 3);
            int benchCount = benchCountDist(laneRng);

            std::vector<int> validTileCols(16);
            std::iota(validTileCols.begin(), validTileCols.end(), 1);
            std::shuffle(validTileCols.begin(), validTileCols.end(), laneRng);

            for (int b = 0; b < benchCount; ++b) {
                float benchX = validTileCols[b] * 64.0f;
                m_scene.spawn<BenchEntity>(ctx, glm::vec2(benchX, lane.yPosition));
            }

            std::uniform_int_distribution<int> buffTypeDist(0, 2);
            TeacherBuffType randomBuff = static_cast<TeacherBuffType>(buffTypeDist(laneRng));

            float patrolStartX = (validTileCols[benchCount] * 64.0f);
            float patrolEndX = (validTileCols[benchCount + 1] * 64.0f);

            auto* teacher = m_scene.spawn<TeacherNPCEntity>(
                ctx,
                glm::vec2(patrolStartX, lane.yPosition),
                glm::vec2(patrolEndX, lane.yPosition),
                randomBuff
            );

            // --- FIX: Bind player pointer ---
            teacher->setPlayer(m_player);
            teacher->onAttach(ctx);
        }
        else if (lane.type == LaneType::Asphalt) {
            std::uniform_int_distribution<int> countDist(1, (m_currentLevel >= 3) ? 3 : 2);
            int busCount = countDist(laneRng);

            float sectorWidth = 1100.0f / busCount;
            std::uniform_real_distribution<float> jitterDist(0.0f, sectorWidth - 180.0f);

            for (int b = 0; b < busCount; ++b) {
                float x = (b * sectorWidth) + jitterDist(laneRng) + lane.spawnXOffset;
                m_scene.spawn<BusEntity>(ctx, glm::vec2(std::fmod(x, 1100.0f), lane.yPosition), lane.moveSpeed, lane.direction);
            }
        }
        else if (lane.type == LaneType::ElevatorTile) {
            auto* crowd = m_scene.spawn<ElevatorCrowdEntity>(ctx, glm::vec2(0.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_elevatorCrowds.push_back(crowd);
        }
        else if (lane.type == LaneType::IDELane) {
            std::uniform_int_distribution<int> countDist(2, 3);
            int streamCount = countDist(laneRng);

            float sectorWidth = 1100.0f / streamCount;
            std::uniform_real_distribution<float> jitterDist(0.0f, sectorWidth - 150.0f);

            for (int c = 0; c < streamCount; ++c) {
                float x = (c * sectorWidth) + jitterDist(laneRng) + lane.spawnXOffset;
                m_scene.spawn<CodeStreamEntity>(ctx, glm::vec2(std::fmod(x, 1100.0f), lane.yPosition), lane.moveSpeed, lane.direction);
            }
        }
        else if (lane.type == LaneType::Water) {
            std::uniform_int_distribution<int> countDist(2, 4);
            int logCount = countDist(laneRng);

            float sectorWidth = 1050.0f / logCount;
            std::uniform_real_distribution<float> jitterDist(0.0f, (std::max)(10.0f, sectorWidth - 200.0f));

            for (int l = 0; l < logCount; ++l) {
                float x = (l * sectorWidth) + jitterDist(laneRng) + lane.spawnXOffset;
                m_scene.spawn<LogPlatformEntity>(ctx, glm::vec2(std::fmod(x, 1050.0f), lane.yPosition), lane.moveSpeed, lane.direction);
            }
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
            lane.signalPhase = 1;
            lane.signalTimer = 0.0f;
        }
        else if (lane.signalPhase == 1 && lane.signalTimer >= 1.2f) {
            lane.signalPhase = 2;
            lane.signalTimer = 0.0f;
        }
        else if (lane.signalPhase == 2) {
            bool crowdFinished = true;
            if (crowdIdx < m_elevatorCrowds.size() && m_elevatorCrowds[crowdIdx]) {
                crowdFinished = m_elevatorCrowds[crowdIdx]->hasFinishedCrossing();
            }

            if (crowdFinished) {
                lane.signalPhase = 0;
                lane.signalTimer = 0.0f;
            }
        }

        SignalState state = static_cast<SignalState>(lane.signalPhase);

        if (crowdIdx < m_elevatorCrowds.size() && m_elevatorCrowds[crowdIdx]) {
            m_elevatorCrowds[crowdIdx]->setSignal(state);
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
            if (ev.key == GLFW_KEY_K) {
                triggerGameOver(ctx);
            }
        }
    }
}

void GameplayLayer::update(double dt, EngineContext* ctx) {
    if (m_isGameOver || m_isLevelComplete) return;

    float fDt = static_cast<float>(dt);

    updateElevatorSignals(fDt);
    updateWaterAnimation(fDt);

    if (m_player) {
        m_player->isOnWaterLane = false;
        m_player->currentWaterVel = glm::vec2(0.0f, 0.0f);

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
    }

    if (m_player && m_player->position.y <= m_goalY + 20.0f) {
        triggerLevelComplete(ctx);
        return;
    }

    if (m_player) {
        m_cameraTargetY = m_player->position.y - 450.0f;
        float factor = 1.0f - std::exp(-6.0f * fDt);
        ctx->cameraPos.y = glm::mix(ctx->cameraPos.y, m_cameraTargetY, factor);

        int distanceScore = static_cast<int>((m_playerStartY - m_player->position.y) / 64.0f) * 10;
        updateScore(distanceScore, ctx);
    }
}

void GameplayLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    for (const auto& lane : m_lanes) {
        TextureHandle laneTex;

        switch (lane.type) {
        case LaneType::SafeZone:     laneTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassSafePath.png"); break;
        case LaneType::Asphalt:      laneTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Road/Road.png"); break;
        case LaneType::ElevatorTile: laneTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/ElevatorRoad.png"); break;
        case LaneType::IDELane:      laneTex = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine.png"); break;
        case LaneType::Water:        
        {
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
        }

        writeBuffer.push_command(10, 0, RectPayload{
            .dest_rect = { 0.0f, lane.yPosition, 1200.0f, lane.height },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = laneTex,
            .atlas_dimensions = { 1, 1 },
            .atlas_pos = { 0, 0 },
            .no_texture = false,
            .is_world_space = true
            });

        if (lane.type == LaneType::ElevatorTile) {
            glm::vec4 signalColor = (lane.signalPhase == 0) ? glm::vec4{ 0.9f, 0.1f, 0.1f, 1.0f }
                : (lane.signalPhase == 1) ? glm::vec4{ 0.9f, 0.8f, 0.1f, 1.0f }
            : glm::vec4{ 0.1f, 0.9f, 0.2f, 1.0f };

            writeBuffer.push_command(15, 0, RectPayload{
                .dest_rect = { 15.0f, lane.yPosition + 12.0f, 24.0f, 40.0f },
                .color = signalColor,
                .no_texture = true,
                .is_world_space = true
                });

            writeBuffer.push_command(15, 0, RectPayload{
                .dest_rect = { 1161.0f, lane.yPosition + 12.0f, 24.0f, 40.0f },
                .color = signalColor,
                .no_texture = true,
                .is_world_space = true
                });
        }
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
        m_currentLevel++;
        ctx->blackboard.set("currentLevel", m_currentLevel);
        initLevel(m_currentLevel, ctx);
    }
    else {
        m_isLevelComplete = true;
        ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>(true));
    }
}

void GameplayLayer::updateScore(int newScore, EngineContext* ctx) {
    if (m_player) {
        newScore *= m_player->getScoreMultiplier();
    }
    if (newScore > m_currentScore) {
        m_currentScore = newScore;
        ctx->blackboard.set("currentScore", m_currentScore);
    }
}