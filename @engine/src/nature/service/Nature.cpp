#include "nature/service/Nature.hpp"
#include "threads/type/Threads.hpp"

#include <algorithm>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

namespace derivean {
namespace {

struct Gate {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned arrived{};
    bool open{};
};

// Parks every pool thread behind the last queued task. Once all of them have arrived nothing else
// is running or queued, and the pool stays idle after the gate opens. This holds only while pool
// tasks never run other queued tasks from inside themselves (sim::Workers::helpUntil): a thread
// parked here from inside a tile build would never finish that build. Tile builds do not; the loops
// they split with parallelFor are worked off by the build's own thread when no other is free.
void quiesce(sim::Workers& pool) {
    const unsigned count = pool.threads();
    const auto gate = std::make_shared<Gate>();
    for (unsigned thread = 0; thread < count; ++thread) {
        pool.submit([gate] {
            std::unique_lock lock(gate->mutex);
            ++gate->arrived;
            gate->changed.notify_all();
            gate->changed.wait(lock, [&] { return gate->open; });
        }, std::numeric_limits<int>::min());
    }
    std::unique_lock lock(gate->mutex);
    gate->changed.wait(lock, [&] { return gate->arrived == count; });
    gate->open = true;
    gate->changed.notify_all();
}

// The spin-up of the first weather on the Mac: 3.1 s on two threads, 1.9 s on four, 1.1 s on eight.
constexpr unsigned StartThreads = 8;

} // namespace

Nature::Nature(NatureOptions options)
    : world(sim::WorldOptions{options.seed, options.age, options.threads}) {
    crew.emplace(options.threads ? options.threads : backgroundThreads());
    air.emplace(options.threads ? options.threads : 2u);
    startThreads = options.threads ? options.threads : std::clamp(std::thread::hardware_concurrency(), 1u, StartThreads);
}

Nature::~Nature() {
    air.reset();
    crew.reset();
    quiesce(world.workers);
}

} // namespace derivean
