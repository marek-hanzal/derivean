#pragma once

#include "aeolian/type/Regime.hpp"
#include "aeolian/type/SandBed.hpp"

namespace derivean::sim {

// Dunes.cpp — the transport model: the slab model of Werner (1995) with the wind speed-up of Momiji
// et al. (2000). Sand moves as slabs: a slab is lifted from a cell that has loose sand and lies in
// the wind, travels a hop downwind (farther from high ground) and comes to rest or is passed on:
// always in the calm lee of higher ground, more readily on sand than on a bare floor, and at the
// foot of a step too steep to climb. Every move is followed by sliding wherever the sand then
// stands steeper than it can rest. The winds of the rose take turns in proportion to the sand each
// can carry. Dunes, their slip faces, their spacing and their kind are whatever this leaves; none is placed.
void blowBed(SandBed& bed, const WindRegime& regime, const DuneRun& run);

// Rounds each sector of the rose blows in one year of `year` rounds, in sector order; sectors too
// weak for a single round are left out.
std::vector<int> windTurns(const WindRegime& regime, int year);

// Shadow.cpp — marks the cells below the line that falls at ShadowTan from every higher point
// upwind; `line` receives the height of that line over each cell.
void castShadow(const SandBed& bed, Vec2 heading, std::vector<float>& line, std::vector<uint8_t>& shadow);

// Avalanche.cpp — sand at and around `cell` slides until nothing loose stands steeper than repose.
void slideSand(SandBed& bed, size_t cell, std::vector<int32_t>& stack);
// One sweep over every pair of neighbours: enough to keep a bed near rest from round to round.
// `turn` picks the corner the sweep starts from.
void slidePairs(SandBed& bed, int turn);
// To rest everywhere, however long the slides.
void slideAll(SandBed& bed);

} // namespace derivean::sim
