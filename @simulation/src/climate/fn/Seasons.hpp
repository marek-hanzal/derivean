#pragma once

#include "climate/type/Climate.hpp"

namespace derivean::sim {

// Closed forms over a sinusoidal year, T(t) = mean + halfRange · cos(2π (t − warmest)), with
// precipitation spread evenly over it. Nothing here steps through time.

// °C at ground `elevation` metres above the sea at a fraction [0, 1) of the calendar year.
double seasonalTemperature(const ZonalClimate& zone, double elevation, double yearFraction);

// Share of the year the temperature spends below `threshold`.
double shareOfYearBelow(double threshold, double mean, double halfRange);

// °C·day per year above freezing.
double positiveDegreeDays(double mean, double halfRange);

struct SnowBalance {
    double snowfall{};        // m water equivalent per year falling below the snow line
    double meltPotential{};   // m water equivalent per year the warm season could melt
    double net{};             // snowfall − meltPotential; positive where snow outlasts the summer
};

// `mean` is the annual mean air temperature at the ground and `precipitation` is in m/yr.
SnowBalance annualSnowBalance(double mean, double halfRange, double precipitation);

// Share of the year rock spends between −8 and −3 °C, where ice segregation cracks it fastest.
double frostCracking(double mean, double halfRange);

} // namespace derivean::sim
