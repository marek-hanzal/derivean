#pragma once

#include "core/type/Vec.hpp"

namespace derivean::sim {

// A session's clock. `era` is the geological date at which the session started (years since the
// hot initial body) and `seconds` is the calendar time that has passed since then. Second zero is
// midnight on day zero of a year, ten days after the northern winter solstice.
struct Calendar {
    double era{};
    double seconds{};
};

inline constexpr double DaySeconds = 86400.0;
inline constexpr double YearDays = 365.0;
inline constexpr double YearSeconds = YearDays * DaySeconds;
inline constexpr double AxialTilt = 23.4;       // degrees
inline constexpr double LunarMonth = 29.53;     // days from new moon to new moon

// Half the range between high and low water when sun and moon pull across each other (neap) and
// when they pull together (spring).
inline constexpr double NeapTide = 0.5;         // m
inline constexpr double SpringTide = 1.0;       // m

double yearFraction(const Calendar& calendar);   // [0, 1)
double dayOfYear(const Calendar& calendar);      // [0, 365), with the fraction of the running day

// [0, 24). The flat world has one sun, so the hour is the same everywhere.
double hourOfDay(const Calendar& calendar);

// Degrees; positive while the sun stands over the northern half.
double declinationOn(double dayOfYear);
double solarDeclination(const Calendar& calendar);

// Latitudes are degrees as returned by latitudeAt(z).
double dayLength(double latitude, const Calendar& calendar);          // hours of daylight
double noonSunElevation(double latitude, const Calendar& calendar);   // degrees; negative in polar night

// Share of the solar constant a horizontal surface receives averaged over one day.
double dailyInsolation(double latitude, double declination);

// [0, 1): 0 at new moon, 0.5 at full moon.
double lunarPhase(const Calendar& calendar);

// m against mean water level. Two crests travel west around the planet with the moon, so the mean
// over the planet is zero at every instant: the tide moves water, it never makes any.
double tideHeight(Vec2 position, const Calendar& calendar);

} // namespace derivean::sim
