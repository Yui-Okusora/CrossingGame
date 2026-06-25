#include "Processor.hpp"
#include "../Context/EngineContext.hpp"
#include "../Core/Layer.hpp"
#include <thread>
#include <chrono>

Processor::Processor(EngineContext* ctx) : m_ctx(ctx) {}

void Processor::operator()() {
    double lastTime = glfwGetTime(); // Use high-precision timing hooks [cite: 1304]
    std::vector<EngineEvent> frameEvents;

    double accumulator = 0.0;
    const double timestep = 1.0 / 60.0; // Fixed physics step tick [cite: 1296]

    const double MIN_TIMESTEP = 1.0 / 200.0; // Captures ultra-high refresh monitors (200Hz+)
    const double MAX_TIMESTEP = 1.0 / 30.0;  // Safeguard against extreme frame drops

    while (m_ctx->isRunning.load(std::memory_order_relaxed)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (deltaTime < MIN_TIMESTEP) deltaTime = MIN_TIMESTEP;
        if (deltaTime > MAX_TIMESTEP) deltaTime = MAX_TIMESTEP;

        accumulator += deltaTime;

        // 1. Safe, concurrent event acquisition
        m_ctx->eventSystem.acquireEvents(frameEvents);

        m_ctx->input.advance_frame_history();

        // ============================================================================
        // STEP 0: APPLY DEFERRED LAYER ACTIONS SECURELY OUTSIDE TRAVERSAL LOOPS
        // ============================================================================
        m_ctx->layerStack->processDeferredCommands(m_ctx);

        // ============================================================================
        // 1. FIXED: NATIVE STRUCT VARIANT TYPE HYDRATION PASS
        // ============================================================================
        for (const auto& event : frameEvents) {
            if (std::holds_alternative<KeyEvent>(event)) {
                auto ev = std::get<KeyEvent>(event);
                if (ev.key >= 0 && ev.key < 384) {
                    if (ev.action == GLFW_PRESS)   m_ctx->input.keys_mask[ev.key >> 6] |= (1ULL << (ev.key & 63));
                    if (ev.action == GLFW_RELEASE) m_ctx->input.keys_mask[ev.key >> 6] &= ~(1ULL << (ev.key & 63));
                }
            }
            else if (std::holds_alternative<MouseBtnEvent>(event)) {
                auto ev = std::get<MouseBtnEvent>(event);
                if (ev.button >= 0 && ev.button < 8) {
                    if (ev.action == GLFW_PRESS)   m_ctx->input.mouse_mask |= (1 << ev.button);
                    if (ev.action == GLFW_RELEASE) m_ctx->input.mouse_mask &= ~(1 << ev.button);
                }
            }
            else if (std::holds_alternative<MouseMoveEvent>(event)) {
                auto ev = std::get<MouseMoveEvent>(event);

                // FIX: Pass raw mouse positions directly. 
                // This guarantees your cursor position perfectly aligns with your UI boundaries!
                m_ctx->input.mousePos = glm::vec2(static_cast<float>(ev.x), static_cast<float>(ev.y));
            }
            // FIX: Match explicitly against your defined type structures
            else if (std::holds_alternative<CharInputEvent>(event)) {
                auto ev = std::get<CharInputEvent>(event);
                if (m_ctx->input.unicode_count < 16) {
                    m_ctx->input.unicode_queue[m_ctx->input.unicode_count++] = ev.codepoint;
                }
            }
            // FIX: Match explicitly against your defined type structures
            else if (std::holds_alternative<MouseScrollEvent>(event)) {
                auto ev = std::get<MouseScrollEvent>(event);
                m_ctx->input.scrollDelta += glm::vec2(static_cast<float>(ev.xoffset), static_cast<float>(ev.yoffset));
            }
        }

        // 2. Evaluate point intersection checks using stable hydrated coordinates
        m_ctx->ui.update_system_states(m_ctx);

        // 3. Forward events down to active layers
        auto& layers = m_ctx->layerStack->getLayers();
        for (const auto& event : frameEvents) {
            for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
                if ((*it)->isSuspended()) continue; // SKIP INPUTS IF SUSPENDED

                (*it)->handleEvent(event, m_ctx);
                if ((*it)->blocksEvents()) break;
            }
        }

        // 4. Fixed Timestep Physics Simulation Loop [cite: 206-209]
        for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
            if ((*it)->isSuspended()) continue;
            (*it)->update(deltaTime, m_ctx); // Passes dynamic frame delta cleanly down the pipe [cite: 207]
            if ((*it)->blocksUpdates()) break;
        }

        // 5. Serialize Graphics and Build Render Stream (Zero Allocations)
        RenderData& writeBuffer = m_ctx->renderBuffer.getWriteBuffer();
        writeBuffer.reset();
        m_ctx->collisionWorld.clear_world();

        for (auto& layer : layers) {
            if (layer->isVisible()) { // Skips rendering automatically if hidden/suspended
                layer->populateRenderStream(writeBuffer, m_ctx);
            }
        }

        // 5. Atomic pointer swap across thread boundary [cite: 1112]
        m_ctx->renderBuffer.swap();

    }
}