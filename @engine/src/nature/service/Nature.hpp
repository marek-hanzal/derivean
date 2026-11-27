#pragma once

#include "nature/type/TileRequests.hpp"
#include "simulation/type/World.hpp"

#include <optional>

namespace derivean {

struct NatureOptions {
    uint64_t seed{42};
    double age{4.6e9};      // yr
    unsigned threads{};     // 0 lets each pool size itself
};

// The simulated world as the engine holds it. The engine's own short tasks (voxel columns, far
// terrain, collider shapes) run on `crew`, never on the simulation's pool: a task there cannot be
// interrupted, so the chunk under the player would wait for a tile build that takes seconds,
// whatever its priority. Two pools the size of the machine share its cores by the scheduler
// instead, and the thread that advances the game is favoured over both (threads/type/Threads.hpp).
// The atmosphere steps on `air`: a calendar far ahead of it owes hours of stepping, which must not
// push in front of the crew's work.
struct Nature {
    explicit Nature(NatureOptions options);
    // Join the engine pools before releasing the World they use. The World stops its own pool
    // before destroying the atlas and aquifers.
    ~Nature();
    Nature(const Nature&) = delete;
    Nature& operator=(const Nature&) = delete;

    sim::World world;
    std::optional<sim::Workers> crew;
    std::optional<sim::Workers> air;
    unsigned startThreads{};   // of the pool the first weather spins up on (nature/type/Weather.hpp)
    TileRequests tiles;
};

} // namespace derivean
