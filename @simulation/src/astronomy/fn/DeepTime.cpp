#include "astronomy/fn/DeepTime.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "astronomy/constant/Sky.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace derivean::sim {
namespace {

// Before today the distance runs along the record; after it, a constant Q makes a^(13/2) grow
// linearly in time from today's rate.
double moonAxisAt(double before) {
    if (before <= 0) return MoonOrbitAxis * std::pow(1 + 6.5 * MoonRecessionToday * -before / MoonOrbitAxis, 1 / 6.5);
    const double elatina = ElatinaMoonShare * MoonOrbitAxis;
    if (before <= ElatinaAge) return MoonOrbitAxis + (elatina - MoonOrbitAxis) * before / ElatinaAge;
    return elatina - ProterozoicRecession * (before - ElatinaAge);
}

// Angular momentum of the Moon's orbit about the pair's barycentre.
double orbitMomentum(double axis) {
    const double earth = EarthGM / GravitationalConstant, moon = MoonGM / GravitationalConstant;
    const double reduced = earth * moon / (earth + moon);
    return reduced * std::sqrt((EarthGM + MoonGM) * axis * (1 - MoonEccentricity * MoonEccentricity));
}

} // namespace

DeepTime deepTimeAt(double age) {
    const double before = PresentAge - age;
    DeepTime state;
    state.moonAxis = moonAxisAt(before);
    const double ratio = state.moonAxis / MoonOrbitAxis;
    state.siderealMonth = SiderealMonth * std::pow(ratio, 1.5);
    // The Sun drives both precessions at a rate of n☉² / n☾ (Murray & Dermott 1999), so their periods
    // shorten as the Moon recedes.
    state.nodalPeriod = NodalPeriod * std::pow(ratio, -1.5);
    state.apsidalPeriod = ApsidalPeriod * std::pow(ratio, -1.5);

    // The Moon's orbit gains what the Earth's spin loses (the spin's moment of inertia from Yoder 1995);
    // the spin cannot fall below the month (lock).
    const double spinToday = 2 * std::numbers::pi / SiderealDay;
    const double total = EarthMomentOfInertia * spinToday + orbitMomentum(MoonOrbitAxis);
    const double spin = std::max((total - orbitMomentum(state.moonAxis)) / EarthMomentOfInertia, 2 * std::numbers::pi / state.siderealMonth);
    state.siderealDay = 2 * std::numbers::pi / spin;

    const double sunAge = std::clamp(SunAgeToday - before, 0.0, 2 * SunAgeToday);
    state.luminosity = SolarLuminosity / (1 + GoughSlope * (1 - sunAge / SunAgeToday));
    state.calibrated = before >= 0 && before <= OldestRhythmite;
    return state;
}

double solarDay(double siderealDay) { return siderealDay / (1 - siderealDay / TropicalYear); }

} // namespace derivean::sim
