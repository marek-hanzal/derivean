#include "simulation/type/World.hpp"

#include "hydrology/fn/Ocean.hpp"

namespace derivean::sim {

World::World(WorldOptions chosen)
    : options(chosen),
      workers(chosen.threads),
      planet(evolvePlanet(chosen.seed, chosen.age, workers)),
      backdrop(readBackdrop(planet, chosen.age, &workers)),
      atlas(Setting{.seed = chosen.seed, .age = chosen.age, .planet = &planet, .backdrop = &backdrop,
                    .frostMovement = chosen.frostMovement}, workers),
      ocean(fillOcean(planet, chosen.age)) {
    interventions.calendar.era = chosen.age;
    // Tiles of the finest level ask the world for what lies outside the landscape (springs, caves).
    atlas.bind(*this);
}

// Queued atlas builds refer to the stores that are destroyed before the pool.
World::~World() { workers.stop(); }

} // namespace derivean::sim
