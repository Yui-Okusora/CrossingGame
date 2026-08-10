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
    static std::mt19937 rng(1337 + level); // Seeded PRNG replacing rand()
    std::uniform_real_distribution<float> speedDist(0.0f, 40.0f);

    int laneCount = 10 + (level * 3);
    float startY = 700.0f;
    float laneHeight = 64.0f;

    m_lanes.push_back({ startY, laneHeight, LaneType::SafeZone, 0.0f, 0 });

    for (int i = 1; i < laneCount - 1; ++i) {
        float currentY = startY - (i * laneHeight);
        LaneType type = static_cast<LaneType>((i % 4) + 1);
        float speed = 120.0f + (level * 25.0f) + speedDist(rng);
        int dir = (i % 2 == 0) ? 1 : -1;

        m_lanes.push_back({ currentY, laneHeight, type, speed, dir, 0.0f, static_cast<uint8_t>(i % 3) });
    }

    float goalY = startY - ((laneCount - 1) * laneHeight);
    m_lanes.push_back({ goalY, laneHeight, LaneType::SafeZone, 0.0f, 0 });
}

void GameplayLayer::spawnLaneEntities(EngineContext* ctx) {
    for (size_t i = 0; i < m_lanes.size(); ++i) {
        const auto& lane = m_lanes[i];

        if (i == m_lanes.size() - 1) {
            m_scene.spawn<GoalEntity>(glm::vec2(0.0f, lane.yPosition));
        }
        else if (lane.type == LaneType::SafeZone && i > 0) {
            m_scene.spawn<BenchEntity>(glm::vec2(250.0f, lane.yPosition));
            m_scene.spawn<BenchEntity>(glm::vec2(850.0f, lane.yPosition));
        }
        else if (lane.type == LaneType::Asphalt) {
            m_scene.spawn<BusEntity>(glm::vec2(100.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<BusEntity>(glm::vec2(650.0f, lane.yPosition), lane.moveSpeed, lane.direction);
        }
        else if (lane.type == LaneType::ElevatorTile) {
            // Spawn 1 wide crowd entity per elevator lane
            auto* crowd = m_scene.spawn<ElevatorCrowdEntity>(glm::vec2(0.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_elevatorCrowds.push_back(crowd);
        }
        else if (lane.type == LaneType::IDELane) {
            m_scene.spawn<CodeStreamEntity>(glm::vec2(150.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<CodeStreamEntity>(glm::vec2(750.0f, lane.yPosition), lane.moveSpeed, lane.direction);
        }
        else if (lane.type == LaneType::Water) {
            m_scene.spawn<LogPlatformEntity>(glm::vec2(100.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<LogPlatformEntity>(glm::vec2(550.0f, lane.yPosition), lane.moveSpeed, lane.direction);
            m_scene.spawn<LogPlatformEntity>(glm::vec2(1000.0f, lane.yPosition), lane.moveSpeed, lane.direction);
        }
    }

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