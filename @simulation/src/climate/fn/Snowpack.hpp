#pragma once

#include "chronology/type/Calendar.hpp"
#include "climate/fn/Moisture.hpp"

#include <span>

namespace derivean::sim {

struct SnowpackChange {
    double swe{};        // kg/m² of snow water equivalent after the step
    double snowfall{};   // kg/m² added to the pack
    double melt{};       // kg/m² released by the pack
    double rain{};       // kg/m² that fell as liquid
};

// One step of a snowpack under air of `celsius` at the ground: precipitation (kg/m² over the step)
// splits by the snow line, and the pack melts by degree-days, never by more than it holds.
// Nothing is lost: swe' = swe + snowfall − melt and precipitation = snowfall + rain.
inline SnowpackChange stepSnowpack(double swe, double celsius, double precipitation, double days) {
    SnowpackChange change;
    change.snowfall = precipitation * snowShare(celsius);
    change.rain = precipitation - change.snowfall;
    const double pack = swe + change.snowfall;
    change.melt = std::min(pack, DegreeDayMelt * std::max(0.0, celsius) * days);
    change.swe = pack - change.melt;
    return change;
}

// The same step over any grid. `precipitation` is a rate in kg/m²/s and `seconds` the step; `swe`
// is updated in place and `melt` receives the kg/m² released in the step. Liquid water reaching
// the ground is rain plus melt: precipitation · seconds − (swe' − swe).
template<typename Real>
void stepSnowpack(std::span<Real> swe, std::span<const Real> temperature, std::span<const Real> precipitation,
                  double seconds, std::span<Real> melt) {
    const double days = seconds / DaySeconds;
    for (size_t cell = 0; cell < swe.size(); ++cell) {
        const SnowpackChange change = stepSnowpack(double(swe[cell]), double(temperature[cell]),
                                                   double(precipitation[cell]) * seconds, days);
        swe[cell] = Real(change.swe);
        melt[cell] = Real(change.melt);
    }
}

} // namespace derivean::sim
