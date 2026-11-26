#pragma once

namespace derivean::sim {

// The slow state of the sky at one planet age: the Moon's distance, the day and month it sets, and
// the Sun's brightness. Every value is today's at `PresentAge`.
//
// Range and accuracy (see docs/handoff/astronomy.md):
//   - 0 to 2450 Ma before today (`calibrated`): the Moon's distance meets the rhythmite record
//     (Williams 2000) by construction; the day follows from conserving the angular momentum of the
//     Earth–Moon pair and comes out 22.1 h at 620 Ma against 21.9 ± 0.4 h measured (solar tides, which
//     also brake the spin, are left out); the Sun (Gough 1981) is good to a few per cent.
//   - Older than 2450 Ma and after today: extrapolations of the same laws, with no record to meet. The
//     world's ages end at 12 Gyr; the spin would meet the month (lock) only near 20 Gyr.
//   - Held at today's at every age: the axis's precession rate (it depends on the spin and the Moon),
//     the obliquity and the eccentricities. Their slow cycles (Laskar et al. 2004) are not an exact
//     ephemeris beyond about 50 Myr (La2010, Laskar et al. 2011), so glacial pacing (S4b) treats them as
//     cycles.
struct DeepTime {
    double moonAxis{};          // m, semi-major axis of the Moon's orbit
    double siderealMonth{};     // s
    double nodalPeriod{};       // s, regression of the Moon's node against the equinox
    double apsidalPeriod{};     // s, advance of the Moon's perigee against the equinox
    double siderealDay{};       // s, one turn of the Earth against the equinox
    double luminosity{};        // W
    bool calibrated{};          // inside the range the record constrains
};

} // namespace derivean::sim
