#pragma once

#include "planet/fn/Band.hpp"

namespace derivean::sim {

// Fatal when one of `near` reaches a deciding cell in the substep it has just turned through (`dt`, yr):
// the band missed it (Codex DREV-CLAUDE-005-04). Checked after the turn, at the piece's end position and
// back along its arc at points at most half the shortest edge apart, the substep's start included: a
// deciding cell within GuardRings of a point is reached when its centre is nearer than the two cells'
// largest chords to a corner (`SphereGrid::reach`) and a quarter of the shortest edge (the envelope ρ_k +
// ρ_c of S3-design.md §4 V3, so a piece born in a large cell that drifts among small ones is caught).
void requireBandHeld(const Globe& globe, const Band& band, const std::vector<int32_t>& near, double dt, double shortest);

} // namespace derivean::sim
