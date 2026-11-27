#pragma once

#include "groundwater/service/Aquifers.hpp"
#include "hydrology/type/Ocean.hpp"
#include "intervention/type/Overlay.hpp"
#include "landscape/service/Atlas.hpp"

namespace derivean::sim {

struct WorldOptions {
    uint64_t seed{42};
    double age{4.6e9};      // yr since the hot initial body
    unsigned threads{0};    // 0 picks one less than the machine has
    FrostMovement frostMovement{FrostMovement::Apply};
};

// One world: the evolved planet and the landscapes derived from it on demand, which stay untouched,
// and over them what the session has changed: the sea as a finite stock and the edited ground with
// its live water. `planet` and the atlas are never written after construction.
struct World {
    explicit World(WorldOptions options);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    WorldOptions options;
    Workers workers;
    Planet planet;
    Backdrop backdrop;
    Atlas atlas;
    Aquifers groundwater;
    Ocean ocean;
    Interventions interventions;
};

} // namespace derivean::sim
