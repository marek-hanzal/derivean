#pragma once

#include "climate/constant/Moisture.hpp"
#include "core/type/Vec.hpp"

#include <array>

namespace derivean::sim {

// Saturation vapour density by the Magnus formula, tabulated at compile time so that the lookup
// inlines into the loops that call it for every cell of every step.
inline constexpr double SaturationColdest = -100.0;   // °C
inline constexpr double SaturationStep = 0.5;
inline constexpr int SaturationEntries = 361;         // up to 80 °C

constexpr double exponential(double x) {
    int halvings = 0;
    while (x > 0.5 || x < -0.5) {
        x *= 0.5;
        ++halvings;
    }
    double term = 1, sum = 1;
    for (int order = 1; order < 18; ++order) {
        term *= x / order;
        sum += term;
    }
    for (int i = 0; i < halvings; ++i) sum *= sum;
    return sum;
}

constexpr std::array<double, SaturationEntries + 1> tabulateSaturation() {
    std::array<double, SaturationEntries + 1> table{};
    for (int entry = 0; entry < SaturationEntries; ++entry) {
        const double celsius = SaturationColdest + entry * SaturationStep;
        const double pressure = 610.94 * exponential(17.625 * celsius / (celsius + 243.04));   // Pa
        table[size_t(entry)] = pressure / (461.5 * (celsius + 273.15));
    }
    table[SaturationEntries] = table[SaturationEntries - 1];
    return table;
}

inline constexpr std::array<double, SaturationEntries + 1> SaturationTable = tabulateSaturation();

// kg/m³ of vapour in saturated air at a temperature in °C; held constant outside the table.
constexpr double saturationDensity(double celsius) {
    const double at = std::clamp((celsius - SaturationColdest) * (1.0 / SaturationStep), 0.0, double(SaturationEntries - 1));
    const size_t entry = size_t(at);
    return SaturationTable[entry] + (SaturationTable[entry + 1] - SaturationTable[entry]) * (at - double(entry));
}

// kg/m² of vapour a column can hold when the air at its ground has this temperature.
inline double columnCapacity(double celsius) { return saturationDensity(celsius) * VapourDepth; }

// m by which air arriving from sea level has been lifted once it stands over ground `height`
// metres above the sea. Rising ground cools the column and squeezes vapour out of it, which is
// what makes windward slopes wet and their lee dry.
inline double terrainLift(double height) { return BlockedLift * (1.0 - std::exp(-std::max(0.0, height) / BlockedLift)); }

// kg/m³ of vapour in the air next to the surface: the column spread over its depth, at most saturated.
inline double surfaceVapour(double vapour, double airDensity) { return std::min(airDensity, vapour * (1.0 / VapourDepth)); }

// Share of its capacity a column keeps after its air was displaced upwards by `lift` metres, for
// lifts up to LiftDepth. Sinking air (negative lift) warms and can hold more.
inline double liftedCapacity(double lift) {
    const double share = lift * (1.0 / LiftDepth);
    return lift >= 0 ? 1.0 - share * (1.0 - 0.5 * share) : 1.0 - share;
}

// kg/m²/s that condense out of a column holding `excess` more than the onset share of its
// capacity, for a caller that already has 1 / capacity at hand. The rate grows with the square of
// the excess, so its slope is twice the rate over the excess.
inline double condensationOfExcess(double excess, double perCapacity) {
    constexpr double Scale = 1.0 / ((1.0 - CondensationOnset) * (1.0 - CondensationOnset) * CondensationTime);
    return excess * excess * perCapacity * Scale;
}

// kg/m²/s that condense out of a column holding `vapour` against `capacity`.
inline double condensationRate(double vapour, double capacity) {
    const double excess = vapour - CondensationOnset * capacity;
    return excess > 0 ? condensationOfExcess(excess, 1.0 / capacity) : 0.0;
}

// kg/m²/s leaving a wet surface whose skin is saturated at `surfaceDensity` into air that already
// holds `heldDensity` next to it.
inline double evaporationRate(double windSpeed, double surfaceDensity, double heldDensity) {
    return ExchangeCoefficient * (windSpeed + Gustiness) * std::max(0.0, surfaceDensity - heldDensity);
}

// Share of open water that is frozen over at this water temperature and neither evaporates nor
// takes up sunlight like open water.
inline double frozenShare(double celsius) { return saturate((-2.0 - celsius) * (1.0 / 6.0)); }

// Share of precipitation that reaches ground of this air temperature as snow.
inline double snowShare(double celsius) { return saturate((SnowLine + SnowBlend - celsius) / (2 * SnowBlend)); }

} // namespace derivean::sim
