#include "Layer.hpp"
#include <algorithm>

LayerStack::LayerStack() {
    m_layers.reserve(8);
    m_deferredQueue.reserve(8); // Avoid runtime heap churn mid-game
}
LayerStack::~LayerStack() = default;

// --- Queue Registration Gates ---
void LayerStack::deferAttach(std::unique_ptr<IEngineLayer> layer) { m_deferredQueue.push_back(AttachCmd{ std::move(layer) }); }
void LayerStack::deferDetach(IEngineLayer* layer) { m_deferredQueue.push_back(DetachCmd{ layer }); }
void LayerStack::deferSwap(size_t idxA, size_t idxB) { m_deferredQueue.push_back(SwapIdxCmd{ idxA, idxB }); }
void LayerStack::deferSwap(IEngineLayer* layerA, IEngineLayer* layerB) { m_deferredQueue.push_back(SwapPtrCmd{ layerA, layerB }); }
void LayerStack::deferSwapWith(IEngineLayer* self, IEngineLayer* other) { m_deferredQueue.push_back(SwapPtrCmd{ self, other }); }
void LayerStack::deferClear() { m_deferredQueue.push_back(ClearCmd{}); }

// --- Immediate State Fallbacks ---
void LayerStack::pushLayer(std::unique_ptr<IEngineLayer> layer, EngineContext* ctx) {
    layer->onAttach(ctx);
    m_layers.push_back(std::move(layer));
}
void LayerStack::popLayer(EngineContext* ctx) {
    if (!m_layers.empty()) {
        m_layers.back()->onDetach(ctx);
        m_layers.pop_back();
    }
}
void LayerStack::clear(EngineContext* ctx) {
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) { (*it)->onDetach(ctx); }
    m_layers.clear();
    m_deferredQueue.clear();
}

// ============================================================================
// SAFE PIPELINE BOUNDARY BUFFER MUTATOR
// ============================================================================
void LayerStack::processDeferredCommands(EngineContext* ctx) {
    if (m_deferredQueue.empty()) return;

    for (auto& cmd : m_deferredQueue) {
        // Case 0: Clear all layers safely at frame boundary
        if (std::holds_alternative<ClearCmd>(cmd)) {
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                (*it)->onDetach(ctx);
            }
            m_layers.clear();
        }
        // Case 1: Attach new layer
        else if (std::holds_alternative<AttachCmd>(cmd)) {
            auto& action = std::get<AttachCmd>(cmd);
            action.layer->onAttach(ctx);
            m_layers.push_back(std::move(action.layer));
        }
        // Case 2: Detach a specific layer
        else if (std::holds_alternative<DetachCmd>(cmd)) {
            auto target = std::get<DetachCmd>(cmd).layer_ptr;
            auto it = std::find_if(m_layers.begin(), m_layers.end(),
                [target](const auto& item) { return item.get() == target; });

            if (it != m_layers.end()) {
                (*it)->onDetach(ctx);
                m_layers.erase(it);
            }
        }
        // Case 3: Swap indices
        else if (std::holds_alternative<SwapIdxCmd>(cmd)) {
            auto action = std::get<SwapIdxCmd>(cmd);
            if (action.index_a < m_layers.size() && action.index_b < m_layers.size()) {
                std::swap(m_layers[action.index_a], m_layers[action.index_b]);
            }
        }
        // Case 4: Swap pointers
        else if (std::holds_alternative<SwapPtrCmd>(cmd)) {
            auto action = std::get<SwapPtrCmd>(cmd);
            auto itA = std::find_if(m_layers.begin(), m_layers.end(), [action](const auto& item) { return item.get() == action.layer_a; });
            auto itB = std::find_if(m_layers.begin(), m_layers.end(), [action](const auto& item) { return item.get() == action.layer_b; });

            if (itA != m_layers.end() && itB != m_layers.end()) {
                std::swap(*itA, *itB);
            }
        }
    }
    m_deferredQueue.clear();
}