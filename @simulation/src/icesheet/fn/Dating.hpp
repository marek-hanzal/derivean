#pragma once

#include "icesheet/type/Continental.hpp"

#include <cstdint>

namespace derivean::sim {

// Dating.cpp — where a date stands among the stages of a world's ice.

// yr, the end of the last termination at or before `age`: the world's present is an interglacial.
double lastTermination(double age);

// °C against the present (`present` as lastTermination gives it): the climate between the
// glaciations of the cycle that ends at or before `time`, and the coldest point of that cycle.
double interglacialCooling(uint64_t seed, double time, double present);
double maximumCooling(uint64_t seed, double time, double present);

// Share of a glacial cycle ending at `time` that its climate spends nearest each stage; the shares
// sum to one. A cooling between two stages counts towards both in proportion.
std::vector<float> stageShares(const ContinentalIce& ice, uint64_t seed, double time);

} // namespace derivean::sim
