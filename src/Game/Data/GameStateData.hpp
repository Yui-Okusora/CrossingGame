#pragma once
#include <Engine/Engine.hpp>
#include <string>
#include <cstdint>

struct GameStateData {
    std::string playerName = "HCMUS Student";
    int32_t gameMode = 0; // 0 = Campaign, 1 = Endless
    int32_t currentLevel = 1;
    int32_t currentScore = 0;
    int32_t highestScore = 0;
    float elapsedTime = 0.0f;
    uint64_t timestampMs = 0;

    void serialize(BinarySerializer& serializer) const;
    bool deserialize(BinaryDeserializer& deserializer);

    static GameStateData CaptureFromBlackboard(EngineContext* ctx);
    void ApplyToBlackboard(EngineContext* ctx) const;
    [[nodiscard]] std::string getFormattedTimestamp() const;
};