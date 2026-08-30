#include "Gameplay.hpp"
#include "DeadlinePopupLayer.hpp"
#include "../Entities/GoalEntity.hpp"
#include "../Entities/BenchEntity.hpp"
#include "../Entities/MovingHazardEntity.hpp"
#include "../Entities/ElevatorCrowdEntity.hpp"
#include "../Entities/StudentPlayerEntity.hpp"
#include "../Entities/TeacherNPCEntity.hpp"
#include "WinLoseLayer.hpp"
#include "PauseMenu.hpp"
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>

void GameplayLayer::updateScore(int newScore, EngineContext* ctx) {
    m_currentScore = newScore;
    ctx->blackboard.set("currentScore", m_currentScore);

    if (m_currentScore > m_highestScore) {
        m_highestScore = m_currentScore;
        ctx->blackboard.set("highestScore", m_highestScore);
    }
}

void GameplayLayer::updateDeadlinePopupTrigger(float dt, EngineContext* ctx) {
    if (!m_timerStarted || m_isGameOver || m_isLevelComplete) return;

    m_deadlineTimer += dt;
    if (m_deadlineTimer >= m_nextDeadlineInterval) {
        m_deadlineTimer = 0.0f;

        static std::mt19937 popupRng(static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_real_distribution<float> intervalDist(15.0f, 30.0f);
        m_nextDeadlineInterval = intervalDist(popupRng);

        ctx->layerStack->deferAttach(std::make_unique<DeadlinePopupLayer>());
    }
}

void GameplayLayer::onAttach(EngineContext* ctx) {
    if (auto nameOpt = ctx->blackboard.get<std::string>("playerName")) m_playerName = *nameOpt;
    if (auto levelOpt = ctx->blackboard.get<int>("currentLevel")) m_currentLevel = *levelOpt;
    if (auto modeOpt = ctx->blackboard.get<int>("gameMode")) m_mode = static_cast<GameMode>(*modeOpt);

    if (auto scoreOpt = ctx->blackboard.get<int>("currentScore")) m_currentScore = *scoreOpt;
    else m_currentScore = 0;

    if (auto timeOpt = ctx->blackboard.get<float>("elapsedTime")) m_elapsedTime = *timeOpt;
    else m_elapsedTime = 0.0f;

    if (auto highScoreOpt = ctx->blackboard.get<int>("highestScore")) {
        m_highestScore = *highScoreOpt;
    }
    else {
        m_highestScore = m_currentScore;
        ctx->blackboard.set("highestScore", m_highestScore);
    }

    m_maxLevel = MAX_CAMPAIGN_LEVELS;
    m_timerStarted = (m_elapsedTime > 0.0f);
    m_deadlineTimer = 0.0f;
    m_nextDeadlineInterval = 22.0f;
    m_endSequenceTimer = 0.0f;
    m_popupTriggered = false;

    ctx->blackboard.set("currentScore", m_currentScore);
    ctx->blackboard.set("elapsedTime", m_elapsedTime);
    ctx->blackboard.set("currentLevel", m_currentLevel);
    ctx->blackboard.set("maxLevel", m_maxLevel);
    ctx->blackboard.set("gameMode", static_cast<int>(m_mode));

    m_texGrassStart = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassStart.png");
    m_texGrassSafePath = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassSafePath.png");
    m_texGrassEnd = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/grassEnd.png");
    m_texLevelUpLine = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Grass/LevelUpLine.png");
    m_texRoad = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Road/Road.png");
    m_texElevatorRoad = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/ElevatorRoad.png");
    m_texCodeLine = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine.png");
    m_texWater = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/River/River.png");
    m_texBusSheet = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Bus/shortBus-sheet.png");
    m_texExamPaper = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/ExamPaper/ExamPaper.png");

    m_texElevatorClosed = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/elevator-closed.png");
    m_texElevatorOpened = ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/Elevator/elevator-opened.png");

    m_codeObstacleTextures = {
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-NullPtr.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-OutOfMemory.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-StackOverflow.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-Condition.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-While.png"),
        ctx->assetManager.loadTexture(RESOURCES_PATH "Sprite/CodeObstacles/CodeLine-Missing_.png")
    };

    m_bgmMusic = ctx->audioEngine.loadSound(SFX_PATH "gameplay_bgm.wav", true);
    m_elevatorChimeSFX = ctx->audioEngine.loadSound(SFX_PATH "elevator_chime.wav");
    m_levelCompleteSFX = ctx->audioEngine.loadSound(SFX_PATH "level_complete.wav");

    ctx->audioEngine.play(m_bgmMusic, AudioCategory::Music, true);

    initLevel(m_currentLevel, ctx);
}

void GameplayLayer::onDetach(EngineContext* ctx) {
    if (ctx && m_bgmMusic.id != 0) {
        ctx->audioEngine.stop(m_bgmMusic);
    }
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
    m_popupTriggered = false;
    m_endSequenceTimer = 0.0f;
    m_timerStarted = (m_elapsedTime > 0.0f);
    m_totalLanesSpawned = 0;

    m_playerStartY = 741.0f;
    m_deathBorderY = 805.0f;
    m_topGeneratedY = m_playerStartY;

    uint64_t freshSeed = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    m_endlessRng.seed(static_cast<uint32_t>(freshSeed ^ (freshSeed >> 32)));

    m_player = m_scene.spawn<StudentPlayerEntity>(ctx, glm::vec2(576.0f, m_playerStartY), this);

    if (m_mode == GameMode::Endless) {
        generateEndlessChunk(25, ctx);
    }
    else {
        generateLanesForLevel(level);
        m_goalY = m_lanes.back().yPosition;
        for (size_t i = 0; i < m_lanes.size(); ++i) {
            spawnSingleLaneEntities(m_lanes[i], ctx, (i == m_lanes.size() - 1));
        }
    }

    ctx->cameraPos = glm::vec2(0.0f, 0.0f);
    m_cameraTargetY = 0.0f;
}

void GameplayLayer::generateEndlessChunk(int count, EngineContext* ctx) {
    std::uniform_real_distribution<float> speedDist(130.0f, 220.0f);
    std::uniform_real_distribution<float> offsetDist(0.0f, 320.0f);
    std::uniform_int_distribution<int> hazardTypeDist(0, 3);
    std::bernoulli_distribution dirDist(0.5);

    const LaneType hazardPool[] = { LaneType::Asphalt, LaneType::ElevatorTile, LaneType::IDELane, LaneType::Water };

    for (int i = 0; i < count; ++i) {
        float currentY = m_topGeneratedY - (m_totalLanesSpawned == 0 ? 0.0f : 64.0f);
        m_topGeneratedY = currentY;

        LaneType type = (m_totalLanesSpawned < 2 || m_totalLanesSpawned % 6 == 0)
            ? LaneType::SafeZone
            : hazardPool[hazardTypeDist(m_endlessRng)];

        LaneData lane{
            .yPosition = currentY,
            .height = 64.0f,
            .type = type,
            .moveSpeed = (type == LaneType::SafeZone) ? 0.0f : speedDist(m_endlessRng),
            .direction = dirDist(m_endlessRng) ? 1 : -1,
            .spawnXOffset = offsetDist(m_endlessRng),
            .seed = static_cast<uint32_t>(m_endlessRng())
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

    int totalLanes = 10 + (level * 2);
    float startY = 741.0f;
    float laneHeight = 64.0f;

    m_lanes.clear();

    m_lanes.push_back({ startY, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng() });
    m_lanes.push_back({ startY - laneHeight, laneHeight, LaneType::SafeZone, 0.0f, 0, 0.0f, rng() });

    const LaneType hazardPool[] = { LaneType::Asphalt, LaneType::ElevatorTile, LaneType::IDELane, LaneType::Water };

    int currentLaneIdx = 2;
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

            if (dir == lastDirection && ++sameDirStreak >= 2) {
                dir = -dir; sameDirStreak = 1;
            }
            else if (dir != lastDirection) {
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

    if (lane.type == LaneType::SafeZone && lane.yPosition < 677.0f) {
        int benchCount = std::uniform_int_distribution<int>(1, 3)(laneRng);
        std::vector<int> validCols(16);
        std::iota(validCols.begin(), validCols.end(), 1);
        std::shuffle(validCols.begin(), validCols.end(), laneRng);

        for (int b = 0; b < benchCount; ++b) {
            m_scene.spawn<BenchEntity>(ctx, glm::vec2(validCols[b] * 64.0f, lane.yPosition));
        }

        auto buffType = static_cast<TeacherBuffType>(std::uniform_int_distribution<int>(0, 2)(laneRng));
        auto* teacher = m_scene.spawn<TeacherNPCEntity>(
            ctx,
            glm::vec2(validCols[benchCount] * 64.0f, lane.yPosition),
            glm::vec2(validCols[benchCount + 1] * 64.0f, lane.yPosition),
            buffType
        );
        teacher->setPlayer(m_player);
    }
    else if (lane.type == LaneType::Asphalt) {
        int count = std::uniform_int_distribution<int>(1, 2)(laneRng);
        float sector = 1100.0f / count;

        for (int b = 0; b < count; ++b) {
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
            auto* stream = m_scene.spawn<MovingHazardEntity>(
                ctx, glm::vec2(x, lane.yPosition), lane.moveSpeed, lane.direction,
                glm::vec2(100.0f, 35.0f), CollisionLayer::Layer_Enemy, CollisionLayer::Layer_Player,
                false, 12.0f, glm::vec4(1.0f), 40
            );
            stream->animator.addAnimation("stream", AnimationClip{ codeTex, { 1, 1 }, 0, 0, 1.0f, false });
            stream->animator.play("stream");
        }
    }
    else if (lane.type == LaneType::Water) {
        int count = std::uniform_int_distribution<int>(2, 4)(laneRng);
        float sector = 1050.0f / count;

        for (int l = 0; l < count; ++l) {
            float x = std::fmod((l * sector) + lane.spawnXOffset, 1050.0f);
            auto* platform = m_scene.spawn<MovingHazardEntity>(
                ctx, glm::vec2(x, lane.yPosition), lane.moveSpeed, lane.direction,
                glm::vec2(96.0f, 48.0f), CollisionLayer::Layer_TriggerVolume, CollisionLayer::Layer_Player,
                true, 8.0f, glm::vec4(1.0f), 25
            );
            platform->animator.addAnimation("float", AnimationClip{ m_texExamPaper, { 1, 1 }, 0, 0, 1.0f, false });
            platform->animator.play("float");
        }
    }
}

void GameplayLayer::updateWaterAnimation(float dt) {
    int total = static_cast<int>(m_waterAtlasDims.x * m_waterAtlasDims.y);
    if (total <= 0) return;

    for (auto& lane : m_lanes) {
        if (lane.type != LaneType::Water) continue;
        lane.waterAnimTimer += dt;
        if (lane.waterAnimTimer >= m_waterFrameDuration) {
            lane.waterAnimTimer -= m_waterFrameDuration;
            lane.waterAnimFrame = static_cast<uint8_t>((lane.waterAnimFrame + 1) % total);
        }
    }
}

void GameplayLayer::updateElevatorSignals(float dt, EngineContext* ctx) {
    size_t crowdIdx = 0;
    for (auto& lane : m_lanes) {
        if (lane.type != LaneType::ElevatorTile) continue;

        lane.signalTimer += dt;
        if (lane.signalPhase == 0 && lane.signalTimer >= 3.0f) {
            lane.signalPhase = 1;
            lane.signalTimer = 0.0f;
            if (ctx) ctx->audioEngine.play(m_elevatorChimeSFX, AudioCategory::GameplaySFX);
        }
        else if (lane.signalPhase == 1 && lane.signalTimer >= 1.2f) {
            lane.signalPhase = 2;
            lane.signalTimer = 0.0f;
        }
        else if (lane.signalPhase == 2) {
            bool finished = (crowdIdx < m_elevatorCrowds.size() && m_elevatorCrowds[crowdIdx])
                ? m_elevatorCrowds[crowdIdx]->hasFinishedCrossing() : true;
            if (finished) {
                lane.signalPhase = 0;
                lane.signalTimer = 0.0f;
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
            //if (ev.key == GLFW_KEY_K) triggerGameOver(ctx);
        }
    }
}

void GameplayLayer::update(double dt, EngineContext* ctx) {
    float fDt = static_cast<float>(dt);

    // Allow lose / win animation sequence to play out smoothly before attaching the result overlay
    if (m_isGameOver) {
        m_scene.fixedUpdate(dt, ctx);
        m_endSequenceTimer += fDt;
        bool animFinished = !m_player || m_player->animator.isCurrentAnimationFinished();
        if ((animFinished || m_endSequenceTimer >= 0.85f) && !m_popupTriggered) {
            m_popupTriggered = true;
            ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>(false));
        }
        return;
    }

    if (m_isLevelComplete && m_currentLevel == m_maxLevel) {
        m_scene.fixedUpdate(dt, ctx);
        m_endSequenceTimer += fDt;
        bool animFinished = !m_player || m_player->animator.isCurrentAnimationFinished();
        if ((animFinished || m_endSequenceTimer >= 0.85f) && !m_popupTriggered) {
            m_popupTriggered = true;
            ctx->layerStack->deferAttach(std::make_unique<WinLosePopupLayer>(true));
        }
        return;
    }

    if (!m_timerStarted && m_player && m_player->hasStartedFirstMove()) {
        m_timerStarted = true;
    }

    if (m_timerStarted) {
        m_elapsedTime += fDt;
        ctx->blackboard.set("elapsedTime", m_elapsedTime);

        updateDeadlinePopupTrigger(fDt, ctx);

        if (m_mode == GameMode::Endless) {
            m_deathBorderY -= m_deathBorderSpeed * fDt;

            if (m_player && m_player->position.y >= m_deathBorderY) {
                m_player->handleLethalDamage(ctx);
                return;
            }

            if (m_player && m_player->position.y < m_topGeneratedY + 800.0f) {
                generateEndlessChunk(15, ctx);
            }

            int distanceScore = static_cast<int>((m_playerStartY - m_player->position.y) / 64.0f) * 10;
            int timeScore = static_cast<int>(m_elapsedTime * 15.0f);
            int multiplier = m_player ? m_player->getScoreMultiplier() : 1;
            int candidateScore = (timeScore + distanceScore) * multiplier;
            if (candidateScore > m_currentScore) {
                updateScore(candidateScore, ctx);
            }
        }
    }

    updateElevatorSignals(fDt, ctx);
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
            if (distanceScore * multiplier > m_currentScore) {
                updateScore(distanceScore * multiplier, ctx);
            }
        }
    }
}

void GameplayLayer::populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) {
    for (size_t i = 0; i < m_lanes.size(); ++i) {
        const auto& lane = m_lanes[i];

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

        TextureHandle tex = m_texGrassSafePath;
        bool skipDefaultDraw = false;

        if (lane.type == LaneType::SafeZone) {
            if (i == 0) { continue; }
            if (i == 1) {
                writeBuffer.push_command(10, 0, RectPayload{
                    .dest_rect = { 0.0f, lane.yPosition, 1200.0f, lane.height * 5.0f },
                    .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    .texture = m_texGrassStart,
                    .no_texture = false,
                    .is_world_space = true
                    });
                continue;
            }
            else if (i == m_lanes.size() - 1 && m_mode == GameMode::Campaign
                && m_currentLevel != m_maxLevel) {
                float renderHeight = lane.height * 4.0f;
                float extra = renderHeight - lane.height;

                writeBuffer.push_command(10, 0, RectPayload{
                    .dest_rect = { 0.0f, lane.yPosition - extra, 1200.0f, renderHeight },
                    .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    .texture = m_texLevelUpLine,
                    .no_texture = false,
                    .is_world_space = true
                    });
                continue;
            }
            else if (i == m_lanes.size() - 1 && m_mode == GameMode::Campaign) {
                bool isMaxLevel = (m_currentLevel == m_maxLevel);
                TextureHandle endTex = isMaxLevel ? m_texGrassEnd : m_texLevelUpLine;
                float renderHeight = lane.height * (isMaxLevel ? 5.0f : 5.0f);
                float extra = renderHeight - lane.height;

                writeBuffer.push_command(10, 0, RectPayload{
                    .dest_rect = { 0.0f, lane.yPosition - extra, 1200.0f, renderHeight },
                    .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    .texture = endTex,
                    .no_texture = false,
                    .is_world_space = true
                    });
                continue;
            }
            else {
                tex = m_texGrassSafePath;
            }
        }
        else if (lane.type == LaneType::Asphalt) {
            tex = m_texRoad;
        }
        else if (lane.type == LaneType::ElevatorTile) {
            tex = m_texElevatorRoad;
        }
        else if (lane.type == LaneType::IDELane) {
            tex = m_texCodeLine;
        }

        writeBuffer.push_command(10, 0, RectPayload{
            .dest_rect = { 0.0f, lane.yPosition, 1200.0f, lane.height },
            .color = { 1.0f, 1.0f, 1.0f, 1.0f },
            .texture = tex,
            .no_texture = false,
            .is_world_space = true
            });

        if (lane.type == LaneType::ElevatorTile) {
            TextureHandle doorTex = (lane.signalPhase == 0) ? m_texElevatorClosed : m_texElevatorOpened;

            writeBuffer.push_command(15, 0, RectPayload{
                .dest_rect = { 0.0f, lane.yPosition, 64.0f, 64.0f },
                .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                .texture = doorTex,
                .no_texture = false,
                .is_world_space = true
                });

            writeBuffer.push_command(15, 0, RectPayload{
                .dest_rect = { 1136.0f, lane.yPosition, 64.0f, 64.0f },
                .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                .texture = doorTex,
                .flip_x = true,
                .no_texture = false,
                .is_world_space = true
                });
        }
    }

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
    m_endSequenceTimer = 0.0f;
    m_popupTriggered = false;
    updateScore(m_currentScore, ctx);
}

void GameplayLayer::triggerLevelComplete(EngineContext* ctx) {
    if (m_isLevelComplete) return;

    if (m_currentLevel < m_maxLevel) {
        if (ctx) ctx->audioEngine.play(m_levelCompleteSFX, AudioCategory::GameplaySFX);
        int timeBonus = (std::max)(0, static_cast<int>(1000.0f - (m_elapsedTime * 20.0f)));
        updateScore(m_currentScore + timeBonus, ctx);

        m_currentLevel++;
        ctx->blackboard.set("currentLevel", m_currentLevel);
        initLevel(m_currentLevel, ctx);
    }
    else {
        m_isLevelComplete = true;
        m_endSequenceTimer = 0.0f;
        m_popupTriggered = false;
        if (m_player) m_player->playWinAnimation();
        if (ctx) ctx->audioEngine.play(m_levelCompleteSFX, AudioCategory::GameplaySFX);
        updateScore(m_currentScore, ctx);
    }
}