#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>

namespace derivean::sim {

struct Planet;
class Workers;

// The flat world has no poles, so climate bands follow a triangle wave along Z with the planet's
// period. Z = 0 sits at 48° north with north towards negative Z; the wave turns at each pole, so on
// the far side of a pole north is towards positive Z.
double latitudeAt(double z);

// −1 or +1: the Z direction in which latitude grows at this position.
double northwardZ(double z);

// K, global mean surface temperature of the cooling body (closed form).
double coolingSurface(double time);

// Share of the surface water inventory that has rained out of the steam atmosphere.
double condensedShare(double time);

// °C against the temperate baseline: residual early heat plus slow greenhouse swings.
double greenhouseOffset(uint64_t seed, double time);

// °C, never positive: sawtooth glacial cooling that ends in a fast termination every cycle.
double glacialOffset(uint64_t seed, double time);

struct ZonalClimate {
    double latitude{};       // degrees
    double temperature{};    // °C at sea level, annual mean
    double seasonal{};       // °C half-range between the warmest and coldest month
    Vec2 wind;               // m/s, direction of travel
    double rain{};           // m/yr over open water at this latitude
};

ZonalClimate zonalClimate(uint64_t seed, double z, double time);

// Annual-mean wind of the trade, westerly and polar belts at a latitude in degrees. The stepped
// atmosphere relaxes towards these belts and the steady climate carries moisture along them.
struct BeltWind {
    double east{};       // m/s
    double poleward{};   // m/s towards the pole of the latitude's own half
};

BeltWind beltWind(double latitude);

// 0 to 1: how readily a temperature gradient (°C per degree of latitude) at a latitude turns into
// weather systems. What counts is the gradient times the tangent of the latitude, the
// supercriticality of a two-layer atmosphere: the same contrast is stable in the tropics and
// stormy in mid-latitudes.
double storminess(double gradient, double latitude);

inline constexpr double LapseRate = 0.0062;   // °C per m of elevation

// Writes the planet's annual-mean temperature, wind and precipitation. Precipitation comes from
// the steady moisture budget (climate/type/MoistureBalance.hpp): it is water that evaporated from
// the planet's own open water and wet ground, so a planet without surface water stays dry.
void solvePlanetClimate(Planet& planet, Workers& workers);

} // namespace derivean::sim
