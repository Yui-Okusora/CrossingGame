#include "EngineContext.hpp"
#include "../Core/Layer.hpp"

EngineContext::EngineContext()
    : window(nullptr),
    isRunning(true),
    scaleFactorX(1.0f),
    scaleFactorY(1.0f),
    layerStack(std::make_unique<LayerStack>()) // Allocate game pipeline stack securely
{}

// ============================================================================
// DESTRUCTOR IMPLEMENTATION
// ============================================================================
EngineContext::~EngineContext() {
    // CRITICAL MULTI-THREADED DESTRUCTION PATTERN:
    // We must manually intercept and destroy the layerStack up front.
    // If we let the compiler implicitly handle this, layers might execute code in their 
    // destructors when sibling systems (like AudioEngine or AssetManager) are already dead,
    // resulting in catastrophic access violations or dangling thread context memory stalls.
    if (layerStack) {
        layerStack->clear(this); // Gracefully notify layers to run onDetach() routines
        layerStack.reset();      // Force complete physical unique_ptr structural teardown
    }

    // Once the layers are gone, the remaining stack instances (audioEngine, assetManager, 
    // renderBuffer, eventSystem, window) implicitly destroy themselves safely in 
    // the exact reverse order of their class declaration.
}