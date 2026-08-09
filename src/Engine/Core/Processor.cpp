#include "Processor.hpp"
#include "../Context/EngineContext.hpp"
#include "../Core/Layer.hpp"
#include <thread>

Processor::Processor(EngineContext* ctx) : m_ctx(ctx) {}

void Processor::operator()() {
    double lastTime = glfwGetTime(); // Use high-precision timing hooks [cite: 1304]
    std::vector<EngineEvent> frameEvents;

    double accumulator = 0.0;
    const double timestep = 1.0 / 100.0;

    while (m_ctx->isRunning.load(std::memory_order_relaxed)) {
        double currentTime = m_ctx->getTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Prevent accumulator explosion if you move/drag the window
        if (deltaTime > 0.1) deltaTime = 0.1;

        accumulator += deltaTime;

        // 1. Safe, concurrent event acquisition
        m_ctx->eventSystem.acquireEvents(frameEvents);
        m_ctx->input.advance_frame_history();

        m_ctx->layerStack->processDeferredCommands(m_ctx);

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

        // 4. Fixed Timestep Physics Simulation Loop
        while (accumulator >= timestep) {
            for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
                if ((*it)->isSuspended()) continue;
                (*it)->update(timestep, m_ctx);
                if ((*it)->blocksUpdates()) break;
            }
            accumulator -= timestep;
        }

        // 5. Serialize Graphics and Build Render Stream (Zero Allocations)
        RenderData& writeBuffer = m_ctx->renderBuffer.getWriteBuffer();
        writeBuffer.reset();
        m_ctx->collisionWorld.clear_world();

        writeBuffer.physicsAlpha = static_cast<float>(accumulator / timestep);

        // --- PASS A: Calculate input permission top-to-bottom ---
        std::vector<bool> layerInputEnabled(layers.size(), true);
        bool isBlocked = false;

        for (int i = static_cast<int>(layers.size()) - 1; i >= 0; --i) {
            if (isBlocked) {
                layerInputEnabled[i] = false; // Block layers underneath
            }
            else {
                layerInputEnabled[i] = !layers[i]->isSuspended();
                if (layers[i]->isVisible() && layers[i]->blocksEvents()) {
                    isBlocked = true; // Block all layers below this one
                }
            }
        }

        // --- PASS B: Populate render stream bottom-to-top ---
        for (size_t i = 0; i < layers.size(); ++i) {
            auto& layer = layers[i];
            if (layer->isVisible()) {
                writeBuffer.current_stack_index = static_cast<int32_t>(i); // Set layer stack level
                m_ctx->ui.input_enabled = layerInputEnabled[i];
                layer->populateRenderStream(writeBuffer, m_ctx);
            }
        }
        m_ctx->ui.input_enabled = true; // Reset default state

        // 6. Atomic pointer swap across thread boundary
        m_ctx->renderBuffer.swap();

        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}