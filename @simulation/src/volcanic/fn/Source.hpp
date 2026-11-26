#pragma once

#include "landscape/type/Setting.hpp"
#include "volcanic/type/Vent.hpp"

namespace derivean::sim {

// A magmatic centre of the planet, in the planet's frame.
struct Centre {
    Vec2 position;
    float vigor{};
    uint64_t id{};
};

// Centres.cpp — the centres that lie in one planet cell.
std::vector<Centre> centresIn(const Planet& planet, int cellX, int cellZ);

// Source.cpp — every vent within `reach` of a world rectangle, with its whole history over the
// region's window: what the planet recorded as melt rising under it, the share of that which
// reached the surface, dealt out among the centres of each planet cell by their vigour and over
// the steps of the window by seeded spells of activity. The same vent comes out the same whatever
// rectangle finds it.
std::vector<Vent> ventsNear(const Setting& setting, Vec2 low, Vec2 high, double reach);

// Chamber.cpp — a chamber fills at the rate its centre is supplied and fails when it holds the
// volume of its next eruption. Those volumes are a seeded sequence of the vent, so its whole
// history, past the world's age too, follows from its supply and from nothing else.
// m³ of the vent's eruption with this number.
double eruptionVolume(uint64_t seed, const Vent& vent, int64_t number);
// m³ a vent erupts at a time, in the mean.
double meanEruption(const Vent& vent);
// m³ of eruptible magma a chamber stores when its centre is supplied at `supply` m³/yr.
double chamberOf(double supply);
// A single eruption of this volume empties the vent's chamber faster than the roof can follow.
bool emptiesChamber(const Vent& vent, double volume);
// The vent's eruptions after its recorded history and up to `until` (yr), in order: the first
// comes when the chamber, at the vent's present supply, has gathered what it lacked at the end of
// the record. None for a vent whose supply has stopped.
void eruptionsAfter(uint64_t seed, const Vent& vent, double until, std::vector<Eruption>& into);
// yr and m³ of the next one after the record, whenever that is; false for a vent without supply.
bool nextEruption(uint64_t seed, const Vent& vent, Eruption& next);

} // namespace derivean::sim
