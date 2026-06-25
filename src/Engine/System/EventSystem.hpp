#pragma once
#include <vector>
#include <variant>
#include <mutex>

struct KeyEvent { int key; int scancode; int action; int mods; };
struct MouseBtnEvent { int button; int action; int mods; };
struct MouseMoveEvent { double x; double y; };
struct WindowResizeEvent { int width; int height; };
struct CharInputEvent { uint32_t codepoint; };
struct MouseScrollEvent { double xoffset; double yoffset; };

enum class UIEventType : uint8_t {
    HoverStarted,
    HoverEnded,
    Clicked,
    Released
};

struct UIInteractionEvent {
    uint32_t widget_id;
    UIEventType type;
};

using EngineEvent = std::variant<KeyEvent, MouseBtnEvent, MouseMoveEvent, WindowResizeEvent, CharInputEvent, MouseScrollEvent, UIInteractionEvent>;

class EventSystem {
private:
    std::vector<EngineEvent> m_producingQueue;
    std::vector<EngineEvent> m_consumingQueue;
    std::mutex m_queueMutex;

public:
    EventSystem();
    ~EventSystem();

    void pushEvent(EngineEvent event);
    void acquireEvents(std::vector<EngineEvent>& outQueue);
};