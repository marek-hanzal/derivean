#pragma once

#include <cstdint>
#include <vector>

namespace derivean::sim {

// Dating.cpp — where a date stands among the stages of a world's ice.

// yr, the end of the last termination at or before `age`: the world's present is an interglacial.
double lastTermination(double age);

// °C against the present (`present` as lastTermination gives it): the climate between the
// glaciations of the cycle that ends at or before `time`, and the coldest point of that cycle.
double interglacialCooling(uint64_t seed, double time, double present);
double maximumCooling(uint64_t seed, double time, double present);

// Share of the glacial cycle ending at or before `time` that its climate spends nearest each stage
// (`cooling` per stage, warmest first); the shares sum to one. A cooling between two stages counts
// towards both in proportion.
std::vector<float> stageShares(const std::vector<double>& cooling, uint64_t seed, double time, double present);

// yr between the coldest point of a cycle and its end.
double sinceMaximum(uint64_t seed, double present);

} // namespace derivean::sim
