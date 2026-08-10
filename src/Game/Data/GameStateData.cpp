#include "GameStateData.hpp"
#include <chrono>

void GameStateData::serialize(BinarySerializer& serializer) const {
    // Uses BinarySerializer operator<< chaining
    serializer << playerName
        << currentLevel
        << currentScore
        << highestScore
        << timestampMs;
}

bool GameStateData::deserialize(BinaryDeserializer& deserializer) {
    // Uses BinaryDeserializer operator>> chaining
    deserializer >> playerName
        >> currentLevel
        >> currentScore
        >> highestScore
        >> timestampMs;

    return !deserializer; // Returns true if stream read succeeded without errors
}

GameStateData GameStateData::CaptureFromBlackboard(EngineContext* ctx) {
    GameStateData data;
    if (ctx) {
        data.playerName = ctx->blackboard.get<std::string>("playerName").value_or("HCMUS Student");
        data.currentLevel = static_cast<int32_t>(ctx->blackboard.get<int>("currentLevel").value_or(1));
        data.currentScore = static_cast<int32_t>(ctx->blackboard.get<int>("currentScore").value_or(0));
        data.highestScore = static_cast<int32_t>(ctx->blackboard.get<int>("highestScore").value_or(0));
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
    ctx->blackboard.set("currentLevel", static_cast<int>(currentLevel));
    ctx->blackboard.set("currentScore", static_cast<int>(currentScore));
    ctx->blackboard.set("highestScore", static_cast<int>(highestScore));
}

std::string GameStateData::getFormattedTimestamp() const {
    if (timestampMs == 0) return "N/A";
    return Utils::formatTS(timestampMs);
}