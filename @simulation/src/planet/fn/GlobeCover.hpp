#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// How the footprints cover the mantle grid, from the union in every cell (S3-design.md §4, Codex V2 and
// DREV-CLAUDE-005-01): hole_c = A_c − U_c and overlap_c = Σ_k |P_kc| − U_c, with exact overlaps. One
// measure for the tests and `terra globe`; Σ a_k over the sphere is another quantity and can match with
// a hole in one place and double cover in another.
struct GlobeCover {
    double worstHole{}, worstOverlap{};   // share of a cell
    int worstCell{-1};                    // of the hole
    long cells{};                         // measured
    long holes[3]{}, overlaps[3]{};       // cells over 0.1, 0.2 and 0.5 of their area
    double hole{}, overlap{};             // share of the measured area
};

// Every cell, or the cells where `skip` is zero.
GlobeCover measureCover(const Globe& globe, const std::vector<uint8_t>& skip = {});

} // namespace derivean::sim
