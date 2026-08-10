#pragma once
#include "../Graphics/RenderStream.hpp"
#include "../System/EventSystem.hpp"
#include <memory>
#include <vector>

class EngineContext;

class IEngineLayer {
protected:
    bool m_isSuspended = false; // Internal suspension tracking flag

public:
    virtual ~IEngineLayer() = default;
    virtual void onAttach(EngineContext* ctx) = 0;
    virtual void onDetach(EngineContext* ctx) = 0;
    virtual void handleEvent(const EngineEvent& event, EngineContext* ctx) = 0;
    virtual void update(double dt, EngineContext* ctx) = 0;
    virtual void populateRenderStream(RenderData& writeBuffer, EngineContext* ctx) = 0;

    // --- State Queries ---
    [[nodiscard]] virtual bool blocksEvents() const noexcept { return false; }
    [[nodiscard]] virtual bool blocksUpdates() const noexcept { return false; }
    [[nodiscard]] virtual bool isVisible() const noexcept { return !m_isSuspended; } // Hidden automatically if suspended

    [[nodiscard]] inline bool isSuspended() const noexcept { return m_isSuspended; }
    inline void setSuspended(bool suspend) noexcept { m_isSuspended = suspend; }
};

// Internal structures to safely defer structural actions across frame transitions
struct AttachCmd { std::unique_ptr<IEngineLayer> layer; };
struct DetachCmd { IEngineLayer* layer_ptr; };
struct SwapIdxCmd { size_t index_a; size_t index_b; };
struct SwapPtrCmd { IEngineLayer* layer_a; IEngineLayer* layer_b; };
struct ClearCmd {};

using StackCommand = std::variant<AttachCmd, DetachCmd, SwapIdxCmd, SwapPtrCmd, ClearCmd>;

class LayerStack {
private:
    std::vector<std::unique_ptr<IEngineLayer>> m_layers;
    std::vector<StackCommand> m_deferredQueue; // Pre-allocated mutation buffer

public:
    LayerStack();
    ~LayerStack();

    // --- Thread-Safe Deferred Commands (Safe to call mid-update or inside UI triggers) ---
    void deferAttach(std::unique_ptr<IEngineLayer> layer);
    void deferDetach(IEngineLayer* layer);
    void deferSwap(size_t index_a, size_t index_b);
    void deferSwap(IEngineLayer* layer_a, IEngineLayer* layer_b);
    void deferSwapWith(IEngineLayer* self, IEngineLayer* other);
    void deferClear();

    // Drained at a safe structural boundary by the central processing core thread
    void processDeferredCommands(EngineContext* ctx);

    // Immediate Execution fallbacks (Only safe when called outside core simulation loops)
    void pushLayer(std::unique_ptr<IEngineLayer> layer, EngineContext* ctx);
    void popLayer(EngineContext* ctx);
    void clear(EngineContext* ctx);

    [[nodiscard]] std::vector<std::unique_ptr<IEngineLayer>>& getLayers() noexcept { return m_layers; }
};