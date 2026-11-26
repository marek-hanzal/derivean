#pragma once

#include "climate/fn/Seasons.hpp"
#include "icesheet/constant/Ice.hpp"

namespace derivean::sim {

// Closed forms of the climate an ice surface meets. Nothing here steps through time.

// °C, annual mean of the air on a surface `surface` metres above the sea.
inline double iceSurfaceWarmth(double seaLevelWarmth, double cooling, double surface) {
    return seaLevelWarmth + cooling - LapseRate * std::max(0.0, surface);
}

// m of ice per year the climate adds to that surface; negative where the summer melts more than
// falls. `rain` is the present precipitation of the bare ground, `ground` metres above the sea.
inline double iceBalance(double seaLevelWarmth, double cooling, double halfRange, double rain, double surface, double ground) {
    const double mean = iceSurfaceWarmth(seaLevelWarmth, cooling, surface);
    const double colder = std::min(0.0, cooling - LapseRate * (std::max(0.0, surface) - std::max(0.0, ground)));
    return annualSnowBalance(mean, halfRange, rain * std::exp(ThinAir * colder)).net / IceToWater;
}

// 0..1: how much of the bed under `thickness` metres of ice is at the melting point. The base is
// warmer than the surface by what the heat from below builds up through the ice, and that heat is
// the rock's plus what the ice makes by shearing over its bed (basal stress times the speed of its
// creep). Snow that is carried down cools the column, so a thin slow sheet under heavy snowfall on
// cold ground stays frozen to its bed, while thick ice and fast ice thaw it (Robin's steady
// profile). `gradient` is that of the rock, `slope` the fall of the ice surface.
inline double bedThaw(double surfaceWarmth, double thickness, double accumulation, double gradient, double slope) {
    if (thickness <= 0) return 0;
    const double stress = IceWeight * thickness * slope, creep = CreepFlow * thickness * thickness * thickness * thickness * slope * slope * slope;
    const double shearing = stress * creep / (SecondsPerYear * IceConductivity);
    const double reach = std::sqrt(2.0 * IceDiffusivity * thickness / std::max(0.01, accumulation));
    const double warming = (BedToIce * gradient + shearing) * reach * 0.886226925 * std::erf(thickness / reach);
    const double overMelting = std::min(0.0, surfaceWarmth) + warming + MeltingFall * thickness;
    return smoothstep(-ThawBand, 0.0, overMelting);
}

} // namespace derivean::sim
