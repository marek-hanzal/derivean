#pragma once

#include "core/service/Workers.hpp"
#include "planet/type/Globe.hpp"

namespace derivean::sim {

// The planet's history on the globe at grid `level` up to `time` (S3-design.md §3–§8), step for step
// the flat evolvePlanet: the primordial body until TectonicsStart, then PlanetStep steps of epochs,
// strata, water, plates with their boundaries, contacts, spreading of thick crust, ocean age, the solid
// surface, the sea every 4 steps, the climate every 16 (on the flat view, read back), the surface
// processes and the marks at every region anchor. The water inventory is the flat 4200 m column, over
// the sphere's area. Serial apart from the climate solve.
Globe evolveGlobe(uint64_t seed, double time, int level, PlateSpeeds speeds, Workers& workers);

// Its parts: the body at min(time, TectonicsStart) in the first epoch with its sea; planet step `step`
// (the globe stands at TectonicsStart + step · PlanetStep); the water, sea, climate, heat and strata
// at the end.
Globe startGlobe(uint64_t seed, double time, int level, PlateSpeeds speeds);
void stepGlobe(Globe& globe, int64_t step, Workers& workers);
void finishGlobe(Globe& globe, Workers& workers);

} // namespace derivean::sim
