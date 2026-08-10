#include "Gameplay.hpp"
#include "../Entities/GoalEntity.hpp"
#include "../Entities/BenchEntity.hpp"
#include "../Entities/BusEntity.hpp"
#include "../Entities/ElevatorCrowdEntity.hpp"
#include "../Entities/CodeStreamEntity.hpp"
#include "../Entities/LogPlatformEntity.hpp"
#include "../Entities/StudentPlayerEntity.hpp"
#include "WinLoseLayer.hpp"
#include "PauseMenu.hpp"
#include <iostream>
#include <random>
#include <cmath>

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
    // 1. Seed PRNG deterministically per level
    std::mt19937 rng(1337 + level * 100);
    std::uniform_real_distribution<float> speedDist(110.0f + level * 18.0f, 150.0f + level * 28.0f);
    std::uniform_real_distribution<float> offsetDist(0.0f, 320.0f);
    std::uniform_int_distribution<int> clusterDist(1, (level >= 3) ? 3 : 2); // Clusters of 1-3 consecutive same-type lanes
    std::uniform_int_distribution<int> hazardTypeDist(0, 3);
    std::bernoulli_distribution dirDist(0.5);

    int totalLanes = 10 + (level * 3);
    float startY = 700.0f;
    float laneHeight = 64.0f;

    m_lanes.clear();

    // Bottom Start Zone (Safe)
    m_lanes.push_back({ startY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, 0.0f, 0 });

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

    // 2. Procedurally generate map with grouped hazard clusters
    while (currentLaneIdx < totalLanes - 1) {
        // Pacing Rule: Force a SafeZone rest stop if player faced 4 consecutive hazard lanes
        if (consecutiveHazards >= 4) {
            float currentY = startY - (currentLaneIdx * laneHeight);
            m_lanes.push_back({ currentY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, 0.0f, 0 });
            consecutiveHazards = 0;
            currentLaneIdx++;
            continue;
        }

        // Choose a hazard type and group cluster size
        LaneType selectedType = hazardPool[hazardTypeDist(rng)];
        int clusterSize = clusterDist(rng);

        for (int c = 0; c < clusterSize && currentLaneIdx < totalLanes - 1; ++c) {
            float currentY = startY - (currentLaneIdx * laneHeight);
            float speed = speedDist(rng);

            // Direction Logic: Prevent more than 2 consecutive lanes moving in the exact same direction
            int dir = dirDist(rng) ? 1 : -1;
            if (dir == lastDirection) {
                sameDirStreak++;
                if (sameDirStreak >= 2) {
                    dir = -dir; // Flip direction
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
                xOffset, // Staggered spawn phase shift
                0.0f,
                static_cast<uint8_t>(c % 3)
                });

            consecutiveHazards++;
            currentLaneIdx++;
        }
    }

    // Top Goal Zone (Safe)
    float goalY = startY - ((totalLanes - 1) * laneHeight);
    m_lanes.push_back({ goalY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, 0.0f, 0 });
}

void GameplayLayer::spawnLaneEntities(EngineContext* ctx) {
    for (size_t i = 0; i < m_lanes.size(); ++i) {
        const auto& lane = m_lanes[i];
        float offset = lane.spawnXOffset;

        if (i == m_lanes.size() - 1) {
            m_scene.spawn<GoalEntity>(glm::vec2(0.0f, lane.yPosition));
        }
        else if (lane.type == LaneType::SafeZone && i > 0) {
            m_scene.spawn<BenchEntity>(glm::vec2(192.0f, lane.yPosition));
            m_scene.spawn<BenchEntity>(glm::vec2(832.0f, lane.yPosition));
        }
        else if (lane.type == LaneType::Asphalt) {
            float x1 = std::fmod(100.0f + offset, 1100.0f);
            float x2 = std::fmod(650.0f + offset, 1100.0f);
            m_scene.spawn<BusEntity>(glm::vec2(x1, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<BusEntity>(glm::vec2(x2, lane.yPosition), lane.moveSpeed, lane.direction);
        }
        else if (lane.type == LaneType::ElevatorTile) {
            auto* crowd = m_scene.spawn<ElevatorCrowdEntity>(glm::vec2(0.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_elevatorCrowds.push_back(crowd);
        }
        else if (lane.type == LaneType::IDELane) {
            float x1 = std::fmod(150.0f + offset, 1100.0f);
            float x2 = std::fmod(750.0f + offset, 1100.0f);
            m_scene.spawn<CodeStreamEntity>(glm::vec2(x1, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<CodeStreamEntity>(glm::vec2(x2, lane.yPosition), lane.moveSpeed, lane.direction);
        }
        else if (lane.type == LaneType::Water) {
            // 3 logs staggered with phase offset across the water channel
            float x1 = std::fmod(64.0f + offset, 1050.0f);
            float x2 = std::fmod(440.0f + offset, 1050.0f);
            float x3 = std::fmod(816.0f + offset, 1050.0f);
            m_scene.spawn<LogPlatformEntity>(glm::vec2(x1, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<LogPlatformEntity>(glm::vec2(x2, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<LogPlatformEntity>(glm::vec2(x3, lane.yPosition), lane.moveSpeed, lane.direction);
        }
    }

    // Grid-aligned player start spawn
    m_player = m_scene.spawn<StudentPlayerEntity>(glm::vec2(576.0f, m_playerStartY), this);
}

void GameplayLayer::updateElevatorSignals(float dt) {
    size_t crowdIdx = 0;

    for (auto& lane : m_lanes) {
        if (lane.type != LaneType::ElevatorTile) continue;

        lane.signalTimer += dt;

        // Phase 0: RED (3.0s window - Safe for student)
        if (lane.signalPhase == 0 && lane.signalTimer >= 3.0f) {
            lane.signalPhase = 1; // Turn Yellow
            lane.signalTimer = 0.0f;
        }
        // Phase 1: YELLOW (1.2s warning light before crowd rushes)
        else if (lane.signalPhase == 1 && lane.signalTimer >= 1.2f) {
            lane.signalPhase = 2; // Turn Green
            lane.signalTimer = 0.0f;
        }
        // Phase 2: GREEN (Crowd rushes - ONLY resets to Red AFTER crowd finishes crossing)
        else if (lane.signalPhase == 2) {
            bool crowdFinished = true;
            if (crowdIdx < m_elevatorCrowds.size() && m_elevatorCrowds[crowdIdx]) {
                crowdFinished = m_elevatorCrowds[crowdIdx]->hasFinishedCrossing();
            }

            if (crowdFinished) {
                lane.signalPhase = 0; // Safe to reset to Red
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
        // Frame-rate independent exponential decay for camera tracking
        float factor = 1.0f - std::exp(-6.0f * fDt);
        ctx->cameraPos.y = glm::mix(ctx->cameraPos.y, m_cameraTargetY, factor);

        int distanceScore = static_cast<int>((m_playerStartY - m_player->position.y) / 64.0f) * 10;
        updateScore(distanceScore, ctx);
    }
}

void GameplayLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    // 1. Render Lane Background Strips
    for (const auto& lane : m_lanes) {
        glm::vec4 laneColor{ 0.2f, 0.2f, 0.2f, 1.0f };

        switch (lane.type) {
        case LaneType::SafeZone:     laneColor = { 0.22f, 0.45f, 0.22f, 1.0f }; break;
        case LaneType::Asphalt:      laneColor = { 0.15f, 0.15f, 0.18f, 1.0f }; break;
        case LaneType::ElevatorTile: laneColor = { 0.50f, 0.45f, 0.38f, 1.0f }; break;
        case LaneType::IDELane:      laneColor = { 0.10f, 0.12f, 0.18f, 1.0f }; break;
        case LaneType::Water:        laneColor = { 0.12f, 0.30f, 0.55f, 1.0f }; break;
        }

        writeBuffer.push_command(10, 0, RectPayload{
            .dest_rect = { 0.0f, lane.yPosition, 1200.0f, lane.height },
            .color = laneColor,
            .no_texture = true,
            .is_world_space = true
            });

        // 2. Render Elevator Traffic Signal Lamps at Both Ends
        if (lane.type == LaneType::ElevatorTile) {
            glm::vec4 signalColor = (lane.signalPhase == 0) ? glm::vec4{ 0.9f, 0.1f, 0.1f, 1.0f }
                : (lane.signalPhase == 1) ? glm::vec4{ 0.9f, 0.8f, 0.1f, 1.0f }
            : glm::vec4{ 0.1f, 0.9f, 0.2f, 1.0f };

            // Left Signal Lamp
            writeBuffer.push_command(15, 0, RectPayload{
                .dest_rect = { 15.0f, lane.yPosition + 12.0f, 24.0f, 40.0f },
                .color = signalColor,
                .no_texture = true,
                .is_world_space = true
                });

            // Right Signal Lamp
            writeBuffer.push_command(15, 0, RectPayload{
                .dest_rect = { 1161.0f, lane.yPosition + 12.0f, 24.0f, 40.0f },
                .color = signalColor,
                .no_texture = true,
                .is_world_space = true
                });
        }
    }

    // 3. Render Scene Entities
    m_scene.populateRenderStream(writeBuffer, ctx);
}

void GameplayLayer::triggerGameOver(EngineContext* ctx) {
    if (m_isGameOver) return;
    m_isGameOver = true;
    ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>());
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
        ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>());
    }
}

void GameplayLayer::updateScore(int newScore, EngineContext* ctx) {
    if (newScore > m_currentScore) {
        m_currentScore = newScore;
        ctx->blackboard.set("currentScore", m_currentScore);
    }
}