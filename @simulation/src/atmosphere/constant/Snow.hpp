#pragma once

#include <array>

namespace derivean::sim {

// Snow is kept per weather cell at a ladder of elevations, so that a cell with mountains in it has
// a snow line instead of one pack for plain and summit alike. Rung r of every per-cell ladder
// (Atmosphere::snowLadder, CloudField::snowLadder) belongs to ground at SnowLadder[r].
inline constexpr int SnowRungs = 12;
inline constexpr std::array<double, SnowRungs> SnowLadder = {0, 400, 800, 1200, 1600, 2000, 2500, 3000, 4000, 5000, 6500, 9000};   // m above the sea

} // namespace derivean::sim
