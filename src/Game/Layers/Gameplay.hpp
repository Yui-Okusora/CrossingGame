#pragma once
#include <Engine/Engine.hpp>
#include "GameGeneral.hpp"
#include "LaneData.hpp"
#include <vector>
#include <string>

class StudentPlayerEntity;
class ElevatorCrowdEntity;

class GameplayLayer : public IEngineLayer {
private:
    Scene2D m_scene;
    std::vector<LaneData> m_lanes;
    StudentPlayerEntity* m_player = nullptr;
    std::vector<ElevatorCrowdEntity*> m_elevatorCrowds;

    std::string m_playerName = "HCMUS Student";
    int m_currentLevel = 1;
    int m_maxLevel = 5;
    int m_currentScore = 0;
    int m_highestScore = 0;

    float m_playerStartY = 700.0f;
    float m_goalY = 0.0f;
    float m_cameraTargetY = 0.0f;

    bool m_isGameOver = false;
    bool m_isLevelComplete = false;

    void initLevel(int level, EngineContext* ctx);
    void generateLanesForLevel(int level);
    void spawnLaneEntities(EngineContext* ctx);
    void updateElevatorSignals(float dt);

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
    void updateScore(int newScore, EngineContext* ctx);
};