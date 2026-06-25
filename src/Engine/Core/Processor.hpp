#pragma once

class EngineContext;

class Processor {
private:
    EngineContext* m_ctx;
public:
    Processor(EngineContext* ctx);
    void operator()();
};