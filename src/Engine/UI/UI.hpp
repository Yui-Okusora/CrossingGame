#pragma once
#include <glm/glm.hpp>
#include <string>

class EngineContext;
struct UIState;

namespace UI {
    // Registers bounds and resolves mouse interaction triggers
    UIState Button(EngineContext* ctx, uint32_t id, const glm::vec4& bounds, bool enabled = true);

    // Processes mouse drag percentages and mutates your local value float inline
    UIState Slider(EngineContext* ctx, uint32_t id, const glm::vec4& trackBounds, float& outValue, bool enabled = true);

    // Drains the UTF-32 character stream into your target string inline
    UIState TextBox(EngineContext* ctx, uint32_t id, const glm::vec4& bounds, std::string& targetString, uint32_t& outCursor, bool enabled = true);
}