#pragma once

#include "atmosphere/type/Plume.hpp"
#include "volcanic/type/Emplacement.hpp"
#include "volcanic/type/Outbreak.hpp"

namespace derivean::sim {

// Outbreak.cpp — eruptions during play. A function of the seed, the vent and the date alone: the
// same eruptions come out whatever window of time asks for them and however it was cut.

// The eruption with this number of the vent's sequence, starting at `time` (yr of the era).
Outbreak outbreakOf(uint64_t seed, const Vent& vent, int64_t number, double time, double volume);

// Every eruption of the vent that starts at or after `from` and before `to` (yr of the era), in
// order: the last few the record remembers, then those its present supply brings up.
void outbreaksBetween(uint64_t seed, const Vent& vent, double from, double to, std::vector<Outbreak>& into);

// What an eruption puts into the air, as sources of a session that began at the era year `era`
// (atmosphere/type/Plume.hpp): one for the explosive phase at the height of its column, one for the
// plume over its lava. `ground` is m above the sea at the vent. Fragments that fall around the vent
// and the hot currents stay on the ground; the ash the wind takes and the fine ash the currents
// shed rise with the column.
void airSourcesOf(const Outbreak& outbreak, double era, double ground, std::vector<AirSource>& into);

// Lays the eruption's fall, hot currents and lava on `field` (live ground, or any grid holding the
// vent) by the same laws the replay builds with, and founders the roof of an emptying one. The ash
// the wind takes is the atmosphere's to lay (`ashFallAt`). The result depends on the eruption and
// the ground alone, never on how play cut time: lay it when the eruption's phases have ended.
// The grid should be coarse enough to hold the lava (a flow of a cubic kilometre covers tens of
// square kilometres) and lie on the world's lattice of its cell size: its origin a multiple of the
// cell. `field.laid` is cleared first. `field.ground` comes back raised on every cell material
// reached, Passing cells included, as the replay leaves it; only the NearField says what stays.
NearField layOutbreak(Emplacement& field, const Outbreak& outbreak);

// kg of rock for a volume brought up.
double rockMass(double volume);

} // namespace derivean::sim
