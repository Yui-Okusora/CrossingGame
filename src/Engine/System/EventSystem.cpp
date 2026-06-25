#include "EventSystem.hpp"

EventSystem::EventSystem() {
    m_producingQueue.reserve(64);
    m_consumingQueue.reserve(64);
}

EventSystem::~EventSystem() = default;

void EventSystem::pushEvent(EngineEvent event) {
    std::lock_guard<std::mutex> lock(m_queueMutex); // Thread-safe submission boundary [cite: 142]
    m_producingQueue.push_back(event);
}

void EventSystem::acquireEvents(std::vector<EngineEvent>& outQueue) {
    outQueue.clear();
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_consumingQueue.swap(m_producingQueue); // Lock-free queue swap execution path [cite: 144]
    }
    outQueue = m_consumingQueue;
    m_consumingQueue.clear();
}