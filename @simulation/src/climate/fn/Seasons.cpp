#include "climate/fn/Seasons.hpp"

#include "chronology/type/Calendar.hpp"
#include "climate/constant/Moisture.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double WarmestNorth = 0.55;   // year fraction of the northern summer peak, a month after the solstice
constexpr double CrackingLow = -8.0, CrackingHigh = -3.0;   // °C

} // namespace

double seasonalTemperature(const ZonalClimate& zone, double elevation, double yearFraction) {
    const double north = zone.latitude >= 0 ? 1.0 : -1.0;
    const double swing = north * zone.seasonal * std::cos(2 * Pi * (yearFraction - WarmestNorth));
    return zone.temperature + swing - LapseRate * std::max(0.0, elevation);
}

double shareOfYearBelow(double threshold, double mean, double halfRange) {
    if (halfRange <= 0) return mean < threshold ? 1.0 : 0.0;
    return std::acos(std::clamp((mean - threshold) / halfRange, -1.0, 1.0)) / Pi;
}

double positiveDegreeDays(double mean, double halfRange) {
    if (halfRange <= 0) return std::max(0.0, mean) * YearDays;
    // The year is above freezing while its phase lies within ±thaw of the warmest day.
    const double thaw = std::acos(std::clamp(-mean / halfRange, -1.0, 1.0));
    return YearDays / Pi * (mean * thaw + halfRange * std::sin(thaw));
}

SnowBalance annualSnowBalance(double mean, double halfRange, double precipitation) {
    SnowBalance balance;
    balance.snowfall = precipitation * shareOfYearBelow(SnowLine, mean, halfRange);
    balance.meltPotential = DegreeDayMelt / WaterDensity * positiveDegreeDays(mean, halfRange);
    balance.net = balance.snowfall - balance.meltPotential;
    return balance;
}

double frostCracking(double mean, double halfRange) {
    return shareOfYearBelow(CrackingHigh, mean, halfRange) - shareOfYearBelow(CrackingLow, mean, halfRange);
}

} // namespace derivean::sim
