#pragma once

#include "aeolian/type/Regime.hpp"
#include "aeolian/type/SandBed.hpp"

namespace derivean::sim {

// Dunes.cpp — the transport model: the slab model of Werner (1995) in flux form. In every round
// each cell that has loose sand and lies in the wind gives up a slab, more of it where it faces the
// wind. What is in the air travels a hop downwind and comes to rest or is passed on: all of it in
// the calm lee of higher ground, at the first shadowed cell it passes over; most of it on sand and
// less on a bare floor; and none of it up a step too steep to climb. Then sand slides wherever it
// stands steeper than it can rest. The winds of the rose take turns in proportion to the sand each
// can carry. Dunes, their slip faces, their spacing and their kind are whatever this leaves; none is
// placed. The result depends on the bed and its seed alone.
void blowBed(SandBed& bed, const WindRegime& regime, const DuneRun& run);

// Turns.cpp — rounds each sector of the rose blows in one year of `year` rounds, in sector order;
// sectors too weak for a single round are left out.
std::vector<int> windTurns(const WindRegime& regime, int year);

// Shadow.cpp — marks the cells below the line that falls at ShadowTan from every higher point
// upwind. `top` is the bed's surface, base and sand; `line` and `turned` are its scratch.
void castShadow(const SandBed& bed, const std::vector<float>& top, Vec2 heading, std::vector<float>& line, std::vector<float>& turned, std::vector<uint8_t>& shadow);

// Avalanche.cpp — sand at and around `cell` slides until nothing loose stands steeper than repose.
void slideSand(SandBed& bed, size_t cell, std::vector<int32_t>& stack);
// One sweep over every pair of neighbours: enough to keep a bed near rest from round to round.
// `turn` picks the corner the sweep starts from.
void slidePairs(SandBed& bed, int turn);
// To rest everywhere, however long the slides.
void slideAll(SandBed& bed);

} // namespace derivean::sim
