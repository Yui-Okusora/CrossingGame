#pragma once
#include <cstdint>

enum class LaneType : uint8_t {
    SafeZone,      // Green brick pavement with static benches
    Asphalt,       // Roadway with moving HCMUS buses
    ElevatorTile,  // Elevator hall with signal lights & rushing crowds
    IDELane,       // Visual Studio Code screen with C++ error streams
    Water          // River channel with floating log platforms
};

enum class SignalState : uint8_t {
    Red,
    Yellow,
    Green
};

struct LaneData {
    float yPosition = 0.0f;
    float height = 64.0f;
    LaneType type = LaneType::SafeZone;
    float moveSpeed = 0.0f;
    int direction = 1;
    float spawnXOffset = 0.0f; // NEW: Procedural phase offset for staggered entity spawning

    // Elevator signal light state tracking
    float signalTimer = 0.0f;
    uint8_t signalPhase = 0; // 0 = Red, 1 = Yellow, 2 = Green
};