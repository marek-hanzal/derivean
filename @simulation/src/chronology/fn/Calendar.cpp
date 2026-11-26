#include "chronology/type/Calendar.hpp"

#include "planet/constant/Scale.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double Radians = Pi / 180.0;
constexpr double SolsticeLead = 10.0;   // days from the northern winter solstice to day zero

double turns(double value) { return value - std::floor(value); }

// Half the daylight arc in radians: 0 in polar night, π under the midnight sun.
double sunsetAngle(double latitude, double declination) {
    const double reach = -std::tan(std::clamp(latitude, -89.999, 89.999) * Radians) * std::tan(declination * Radians);
    return std::acos(std::clamp(reach, -1.0, 1.0));
}

} // namespace

double yearFraction(const Calendar& calendar) { return turns(calendar.seconds / YearSeconds); }

double dayOfYear(const Calendar& calendar) { return yearFraction(calendar) * YearDays; }

double hourOfDay(const Calendar& calendar) { return turns(calendar.seconds / DaySeconds) * 24.0; }

double declinationOn(double dayOfYear) {
    return -AxialTilt * std::cos(2 * Pi * (dayOfYear + SolsticeLead) / YearDays);
}

double solarDeclination(const Calendar& calendar) { return declinationOn(dayOfYear(calendar)); }

double dayLength(double latitude, const Calendar& calendar) {
    return 24.0 * sunsetAngle(latitude, solarDeclination(calendar)) / Pi;
}

double noonSunElevation(double latitude, const Calendar& calendar) {
    return 90.0 - std::abs(latitude - solarDeclination(calendar));
}

double dailyInsolation(double latitude, double declination) {
    const double arc = sunsetAngle(latitude, declination);
    const double place = latitude * Radians, sun = declination * Radians;
    return std::max(0.0, arc * std::sin(place) * std::sin(sun) + std::cos(place) * std::cos(sun) * std::sin(arc)) / Pi;
}

double lunarPhase(const Calendar& calendar) { return turns(calendar.seconds / (LunarMonth * DaySeconds)); }

double tideHeight(Vec2 position, const Calendar& calendar) {
    // The moon falls behind the sun by one turn per lunar month, which makes its day 24 h 50 min.
    const double days = calendar.seconds / DaySeconds;
    const double moonAngle = 2 * Pi * (days - days / LunarMonth + position.x / PlanetSpan);
    const double phase = lunarPhase(calendar);
    const double range = 0.5 * (SpringTide + NeapTide) + 0.5 * (SpringTide - NeapTide) * std::cos(4 * Pi * phase);
    return range * std::cos(2 * moonAngle);
}

} // namespace derivean::sim
