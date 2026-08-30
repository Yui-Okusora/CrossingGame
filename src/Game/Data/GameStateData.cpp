#include "GameStateData.hpp"
#include <chrono>

void GameStateData::serialize(BinarySerializer& serializer) const {
    serializer << playerName
        << gameMode
        << currentLevel
        << currentScore
        << highestScore
        << elapsedTime
        << timestampMs;
}

bool GameStateData::deserialize(BinaryDeserializer& deserializer) {
    deserializer >> playerName
        >> gameMode
        >> currentLevel
        >> currentScore
        >> highestScore
        >> elapsedTime
        >> timestampMs;

    return !deserializer;
}

GameStateData GameStateData::CaptureFromBlackboard(EngineContext* ctx) {
    GameStateData data;
    if (ctx) {
        data.playerName = ctx->blackboard.get<std::string>("playerName").value_or("HCMUS Student");
        data.gameMode = ctx->blackboard.get<int>("gameMode").value_or(0);
        data.currentLevel = static_cast<int32_t>(ctx->blackboard.get<int>("currentLevel").value_or(1));
        data.currentScore = static_cast<int32_t>(ctx->blackboard.get<int>("currentScore").value_or(0));
        data.highestScore = static_cast<int32_t>(ctx->blackboard.get<int>("highestScore").value_or(0));
        data.elapsedTime = ctx->blackboard.get<float>("elapsedTime").value_or(0.0f);
    }

    auto now = std::chrono::system_clock::now();
    data.timestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
        );

    return data;
}

void GameStateData::ApplyToBlackboard(EngineContext* ctx) const {
    if (!ctx) return;
    ctx->blackboard.set("playerName", playerName);
    ctx->blackboard.set("gameMode", static_cast<int>(gameMode));
    ctx->blackboard.set("currentLevel", static_cast<int>(currentLevel));
    ctx->blackboard.set("currentScore", static_cast<int>(currentScore));
    ctx->blackboard.set("highestScore", static_cast<int>(highestScore));
    ctx->blackboard.set("elapsedTime", elapsedTime);
}

std::string GameStateData::getFormattedTimestamp() const {
    if (timestampMs == 0) return "N/A";
    return Utils::formatTS(timestampMs);
}