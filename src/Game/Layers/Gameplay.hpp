#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"
#include "LaneData.hpp"
#include <vector>
#include <string>
#include <random>

class StudentPlayerEntity;
class ElevatorCrowdEntity;

class GameplayLayer : public IEngineLayer {
private:
    Scene2D m_scene;
    std::vector<LaneData> m_lanes;
    StudentPlayerEntity* m_player = nullptr;
    std::vector<ElevatorCrowdEntity*> m_elevatorCrowds;

    std::string m_playerName = "HCMUS Student";
    GameMode m_mode = GameMode::Campaign;

    int m_currentLevel = 1;
    int m_maxLevel = MAX_CAMPAIGN_LEVELS;
    int m_currentScore = 0;
    int m_highestScore = 0;
    float m_elapsedTime = 0.0f;
    bool m_timerStarted = false;

    // Transition & Animation Delay Trackers
    float m_endSequenceTimer = 0.0f;
    bool m_popupTriggered = false;

    // Deadline Random Popup Tracker
    float m_deadlineTimer = 0.0f;
    float m_nextDeadlineInterval = 20.0f;

    // Endless Infinite Generation Trackers
    std::mt19937 m_endlessRng;
    float m_topGeneratedY = 700.0f;
    int m_totalLanesSpawned = 0;
    float m_deathBorderY = 764.0f;
    float m_deathBorderSpeed = 44.0f;

    float m_playerStartY = 741.0f;
    float m_goalY = 0.0f;
    float m_cameraTargetY = 0.0f;

    bool m_isGameOver = false;
    bool m_isLevelComplete = false;

    // Terrain Textures
    TextureHandle m_texGrassStart{};
    TextureHandle m_texGrassSafePath{};
    TextureHandle m_texGrassEnd{};
    TextureHandle m_texLevelUpLine{};
    TextureHandle m_texRoad{};
    TextureHandle m_texElevatorRoad{};
    TextureHandle m_texCodeLine{};
    TextureHandle m_texWater{};
    TextureHandle m_texBusSheet{};
    TextureHandle m_texExamPaper{};

    // Elevator Doors
    TextureHandle m_texElevatorClosed{};
    TextureHandle m_texElevatorOpened{};

    // Gameplay Audio Handles
    AudioHandle m_bgmMusic{ 0 };
    AudioHandle m_elevatorChimeSFX{ 0 };
    AudioHandle m_levelCompleteSFX{ 0 };

    std::vector<TextureHandle> m_codeObstacleTextures;

    glm::uvec2 m_waterAtlasDims{ 4, 1 };
    float m_waterFrameDuration = 0.12f;

    void initLevel(int level, EngineContext* ctx);
    void generateLanesForLevel(int level);
    void generateEndlessChunk(int count, EngineContext* ctx);
    void spawnSingleLaneEntities(const LaneData& lane, EngineContext* ctx, bool isGoal = false);
    void updateElevatorSignals(float dt, EngineContext* ctx);
    void updateWaterAnimation(float dt);
    void updateScore(int newScore, EngineContext* ctx);
    void updateDeadlinePopupTrigger(float dt, EngineContext* ctx);

public:
    GameplayLayer() = default;
    ~GameplayLayer() override = default;

    void onAttach(EngineContext* ctx) override;
    void onDetach(EngineContext* ctx) override;
    void handleEvent(const EngineEvent& event, EngineContext* ctx) override;
    void update(double dt, EngineContext* ctx) override;
    void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) override;

    void triggerGameOver(EngineContext* ctx);
    void triggerLevelComplete(EngineContext* ctx);
};