#include "Application.hpp"
#include "Processor.hpp"
#include "Layer.hpp"
#include <algorithm>
#include <iostream>

Application::Application(const std::string& title, int w, int h) {
    if (!glfwInit()) throw std::runtime_error("GLFW initialization failed.");

    m_ctx.startTimePoint = std::chrono::steady_clock::now();

    WindowSpecs specs{ .title = title, .width = w, .height = h, .resizable = true, .fullscreen = false, .vSync = false, .fps = 60 };
    m_ctx.window = std::make_shared<Window>(specs);
    m_ctx.window->create(&m_ctx);

    GLFWwindow* nativeWin = m_ctx.window->getHandle();
    glfwSetKeyCallback(nativeWin, keyCallback);
    glfwSetMouseButtonCallback(nativeWin, mouseButtonCallback);
    glfwSetCursorPosCallback(nativeWin, cursorPosCallback);
    glfwSetCharCallback(nativeWin, charInputCallback);
    glfwSetScrollCallback(nativeWin, scrollCallback);
    glfwSetWindowContentScaleCallback(nativeWin, windowContentScaleCallback);

    gl2d::init();
    m_gl2dRenderer.create(0, 4000);
    m_ctx.globalFont.createFromFile(FONT_PATH "BoldPixels.ttf");
    m_ctx.layerStack = std::make_unique<LayerStack>();
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
    ctx->eventSystem.pushEvent(KeyEvent{ key, scancode, action, mods });
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
    ctx->eventSystem.pushEvent(MouseBtnEvent{ button, action, mods });
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
    ctx->eventSystem.pushEvent(MouseMoveEvent{ xpos, ypos });
}

void Application::charInputCallback(GLFWwindow* window, unsigned int codepoint) {
    auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
    ctx->eventSystem.pushEvent(CharInputEvent{ static_cast<uint32_t>(codepoint) });
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
    ctx->eventSystem.pushEvent(MouseScrollEvent{ xoffset, yoffset });
}

void Application::windowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
    auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
    if (ctx) {
        // Sequentially consistent atomics guarantee both logic and render threads catch the scale flip instantly
        ctx->scaleFactorX.store(xscale, std::memory_order_seq_cst);
        ctx->scaleFactorY.store(yscale, std::memory_order_seq_cst);
    }
}

void Application::run() {
    m_logicThread = std::thread(Processor(&m_ctx));

    while (!m_ctx.window->shouldClose() && m_ctx.isRunning.load(std::memory_order_relaxed)) {
        glfwPollEvents();

        glm::vec2 fbSize = m_ctx.window->getFramebufferSize();
        glViewport(0, 0, (int)fbSize.x, (int)fbSize.y);

        m_ctx.fbWidth.store(fbSize.x, std::memory_order_relaxed);
        m_ctx.fbHeight.store(fbSize.y, std::memory_order_relaxed);

        m_gl2dRenderer.updateWindowMetrics((int)fbSize.x, (int)fbSize.y);

        const glm::vec2 DESIGN_RESOLUTION = { 1200.0f, 805.0f };
        float sx = fbSize.x / DESIGN_RESOLUTION.x;
        float sy = fbSize.y / DESIGN_RESOLUTION.y;

        float scale = (std::min)(sx, sy); // Pick the restrictive bounding aspect axis
        glm::vec2 scaledCanvas = DESIGN_RESOLUTION * scale;

        // Store the final computed transform directly into m_ctx so the input thread 
        // can access it instantly for back-mapping clicks!
        m_ctx.currentViewport.scale = scale;
        m_ctx.currentViewport.offset = (fbSize - scaledCanvas) * 0.5f;

        RenderData& renderData = m_ctx.renderBuffer.getReadBuffer();

        m_gl2dRenderer.clearScreen({ 0, 0, 1, 1 }); // Clear frame baseline color

        std::sort(renderData.commands.begin(), renderData.commands.end(),
            [](const RenderCommand& a, const RenderCommand& b) noexcept {
                // 1. Primary Sort: LayerStack level (Lower layers render first, Upper layers overlay on top)
                if (a.stack_index != b.stack_index) {
                    return a.stack_index < b.stack_index;
                }
                // 2. Secondary Sort: Intra-layer depth (Within the same layer)
                if (a.layer_depth != b.layer_depth) {
                    return a.layer_depth < b.layer_depth;
                }
                // 3. Tertiary Sort: Draw call key / Submission order
                return a.sorting_key < b.sorting_key;
            }
        );

        // ============================================================================
        // CLEAN DISPATCH CYCLE (Zero Bloat, Endless Versatility)
        // ============================================================================
        // The core driver has no dependencies on explicit game concepts. It iterates 
        // through a flat vector and fires function delegates blindly across continuous memory vectors.
        for (const auto& cmd : renderData.commands) {
            if (cmd.execute) {
                // Dynamically resolve the absolute stable position inside the raw memory stream
                const void* resolved_payload = &renderData.payload_arena[cmd.payload_offset];

                // Execute the rendering instruction blindly
                cmd.execute(m_gl2dRenderer, resolved_payload, &m_ctx);
            }
        }

        m_gl2dRenderer.flush(); // Consolidate batch arrays and issue drawing hooks to the GPU

        m_ctx.window->update();
    }

    m_ctx.isRunning.store(false, std::memory_order_relaxed);
}

Application::~Application() {
    if (m_logicThread.joinable()) {
        m_logicThread.join(); // Secure clean background thread termination [cite: 802]
    }
    gl2d::cleanup();
}