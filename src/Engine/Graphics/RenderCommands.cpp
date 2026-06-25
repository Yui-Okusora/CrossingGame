#include "RenderCommands.hpp"

RenderData::RenderData() {
    commands.reserve(256);
    payload_arena.reserve(4096);
}

RenderData::~RenderData() = default;

void RenderData::reset() {
    commands.clear();
    payload_arena.clear();
}