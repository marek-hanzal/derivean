#pragma once

#include "coast/type/Shore.hpp"
#include "landscape/fn/Build.hpp"

namespace derivean::sim {

// Shores of seas and lakes, made by waves working on what the land offers. `workShores`
// (landscape/fn/Process.hpp) runs these once per step of the Region, Local and Site levels, after
// the rivers of the step have delivered their load:
//   water   where waves can go: every cell that still water reaches at high water;
//   waves   eight trains raised by the storm wind over the fetch they cross, broken by the depth;
//   cliffs  rock that waves strike harder than it withstands is cut back at its foot and comes down;
//   sand    waves carry sand and gravel up to the slope each grain size holds, along the shore
//           where they arrive at an angle, and drop it where they die.
// A ground tile takes its standing water from its parent and has no shore of its own.

// Seas.cpp — what the open water beyond a domain sends across its rim: for a region the fetch over
// the planet's sea, for a finer level the trains its parent recorded. Called once when a domain is built.
void readSeas(Domain& domain, const Setting& setting, const TileLookup& lookup);
// The shore of a finished domain, with the waves of its last flooding; null where no water stands.
std::shared_ptr<const ShoreRecord> recordShore(const Domain& domain);

// Shore.cpp — the working state, made on first use.
Shore& shoreOf(Domain& domain);
// Still water over `top`, from the water that stood when the step was flooded; false where there is none.
bool spreadWater(const Domain& domain, Shore& shore);

// Waves.cpp
float stormWind(const Domain& domain, size_t cell, int train);   // m/s
void raiseWaves(const Domain& domain, Shore& shore);

// Cliff.cpp — lets up to `years` pass for every face under attack, but no longer than the first
// of them needs to come down; returns the years that passed.
double cutCliffs(Domain& domain, Shore& shore, double years);
// m, the wave height a cell's rock withstands.
float withstandOf(const Domain& domain, size_t cell);

// Drift.cpp — one pass of every train over the sand and gravel within its reach.
void moveSand(Domain& domain, Shore& shore, int round);

} // namespace derivean::sim
