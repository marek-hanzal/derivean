#pragma once

#include "simulation/type/World.hpp"

#include <napi.h>

#include <condition_variable>
#include <memory>
#include <mutex>

namespace derivean {

// Charts are read on libuv's pool while the session goes on advancing, so a world can have readers
// the session thread knows nothing about. Whoever is about to destroy a world waits here until the
// last of them has left; destroying the gate waits as well, so a member declared after the world
// it guards covers the session's own destruction.
class ChartGate {
public:
    ChartGate();
    ~ChartGate();
    ChartGate(const ChartGate&) = delete;
    ChartGate& operator=(const ChartGate&) = delete;

    void wait();

    struct Readers {
        std::mutex mutex;
        std::condition_variable idle;
        int count{};
    };
    // Shared with every chart in flight: a reader leaves through it without touching the gate.
    const std::shared_ptr<Readers>& readers() const { return readers_; }

private:
    std::shared_ptr<Readers> readers_;
};

// chart({x, z, span, size, unless?}) → Promise<Chart | null>; the shapes are in GameBridge.ts.
// The promise settles on the calling thread, but the reading happens off it.
Napi::Value requestChart(const Napi::CallbackInfo& info, const sim::World& world, ChartGate& gate);

} // namespace derivean
