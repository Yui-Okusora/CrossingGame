#pragma once
#include <Engine/Engine.hpp>
#include <string>
#include <cstdint>

struct GameStateData {
    std::string playerName = "HCMUS Student";
    int32_t currentLevel = 1;
    int32_t currentScore = 0;
    int32_t highestScore = 0;
    uint64_t timestampMs = 0;

    // --- IMPLICIT SERIALIZATION CONTRACT ---
    // Called implicitly by BinarySerializer (e.g. `serializer << data;`)
    void serialize(BinarySerializer& serializer) const;

    // Called implicitly by BinaryDeserializer (e.g. `deserializer >> data;`)
    bool deserialize(BinaryDeserializer& deserializer);

    // --- ENGINE BLACKBOARD INTERFACE ---
    static GameStateData CaptureFromBlackboard(EngineContext* ctx);
    void ApplyToBlackboard(EngineContext* ctx) const;
    [[nodiscard]] std::string getFormattedTimestamp() const;
};