#pragma once
#include "../Context/EngineContext.hpp"
#include <thread>

class Application {
private:
    EngineContext m_ctx;
    std::thread m_logicThread;
    gl2d::Renderer2D m_gl2dRenderer;

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void charInputCallback(GLFWwindow* window, unsigned int codepoint);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void windowContentScaleCallback(GLFWwindow* window, float xscale, float yscale);

public:
    Application(const std::string& title, int w, int h);
    ~Application();

    void run();
    EngineContext& getContext() { return m_ctx; }
};