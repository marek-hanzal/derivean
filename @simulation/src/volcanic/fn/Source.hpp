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

// One eruption of a vent's own sequence. A chamber fills at the rate its centre is supplied and
// fails when it holds the volume of its next eruption; the volumes are a seeded sequence along the
// axis of supplied volume, so the whole history of a vent follows from its supply.
struct Eruption {
    double supplied{};   // m³ the vent had been supplied with when it erupted
    double volume{};     // m³
};

// Chamber.cpp
// m³ supplied to a vent from the start of its window to a date; past the last recorded step the
// supply continues at the vent's present rate.
double suppliedBy(const Vent& vent, double time);
// The date by which a vent has been supplied with `volume` m³; beyond reach (1e30) if it never is.
double dateOf(const Vent& vent, double volume);
// Eruptions that happen while the supply runs from `from` to `to` m³, in order.
void eruptionsBetween(uint64_t seed, const Vent& vent, double from, double to, std::vector<Eruption>& into);
// The largest of them, 0 if there is none.
double largestEruption(uint64_t seed, const Vent& vent, double from, double to);
// m³ a vent of this magma erupts at a time, in the mean.
double meanEruption(Magma magma);

} // namespace derivean::sim
