#pragma once

#include "landscape/type/Setting.hpp"
#include "strata/type/Column.hpp"

namespace derivean::sim {

// Where a world position reads the planet grid. The planet moves crust in whole cells along its
// axes; this smooth seeded offset keeps coasts and belts from inheriting that grid.
Vec2 planetPoint(uint64_t seed, Vec2 position);

// The inverse: the world position that reads the planet grid at `planetPosition`.
Vec2 worldPoint(uint64_t seed, Vec2 planetPosition);

// What the planet imposes on one point of a landscape for the length of a replay.
struct Forcing {
    float tectonic{};     // m/yr the surface rises as crust stacks or stretches, distributed by structure
    float residual{};     // m/yr of direct surface motion, including fault-block tilting
    float weakness{};     // 0..1, regional fault zones
    float rain{};         // m/yr, annual precipitation before local relief redistributes it
    float warmth{};       // °C, annual mean reduced to sea level
    float windX{}, windZ{};   // m/s, direction of travel
    StrataColumn strata;
};

Forcing forcingAt(const Setting& setting, Vec2 position);

// 0..1: crushed rock along fractures spaced about `spacing` metres apart, aligned with the fabric.
float fractureAt(uint64_t seed, Vec2 material, double strike, double spacing);

// Seeded roughness of an initial surface at about `wavelength` metres, in [-1, 1].
float roughnessAt(uint64_t seed, Vec2 material, double wavelength);

// Seeded rock quality: a factor around one on erodibility, varying over about `wavelength` metres.
// Without it a uniform slope has nothing to choose its valleys by and cuts them at even spacing.
float qualityAt(uint64_t seed, Vec2 material, double wavelength);

double seaLevelAt(const Setting& setting, double time);

inline constexpr float StormShare = 0.06f;

// Annual runoff in m/yr from precipitation and the local annual mean temperature.
// The mean balance (Budyko) says how much is left after evaporation; on top of it a fixed share
// of whatever falls runs off in storms before the air can take it back, which is the water that
// shapes a desert.
inline float runoffOf(float rain, float temperature) {
    if (rain <= 1e-4f) return 0;
    const float demand = std::clamp(0.25f + 0.035f * temperature, 0.05f, 1.6f);
    return rain * (StormShare + (1.0f - StormShare) * std::exp(-demand / rain));
}

} // namespace derivean::sim
