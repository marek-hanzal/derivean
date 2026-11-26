// What follows from the geometry of the three bodies: eclipses against NASA's canon (Espenak,
// eclipse.gsfc.nasa.gov; magnitudes as in eclipsewise.com), the Moon's months and standstills, the
// deep-time state against the rhythmite record (Williams 2000) and the
// standard Sun (Gough 1981).
#include "AstronomyFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "astronomy/fn/DeepTime.hpp"
#include "astronomy/fn/Eclipse.hpp"
#include "astronomy/fn/Moonlight.hpp"
#include "astronomy/fn/Place.hpp"

#include <algorithm>
#include <vector>

using namespace derivean::sim;
using namespace derivean::sim::astronomytest;

namespace {

constexpr double EquatorialRadius = 6378137.0;   // NASA's canons measure the shadow in it
constexpr double Start = 2460310.5;              // 2024-01-01 0h TT
constexpr double Step = 10 / 1440.0;             // d

struct Event {
    double at;      // JD of the greatest phase
    double value;   // its measure
};

// Runs of steps where `inside` holds over 2024 and 2025, each with the instant where `measure` is
// largest, found to 10 s around the best step.
template <class Inside, class Measure>
std::vector<Event> eventsOf(Inside inside, Measure measure) {
    std::vector<Event> events;
    bool open = false;
    for (double d = 0; d < 731; d += Step) {
        const Sky sky = skyOn(Start + d);
        if (!inside(sky)) {
            open = false;
            continue;
        }
        if (!open) events.push_back({Start + d, -1e30});
        open = true;
        if (measure(sky) > events.back().value) events.back() = {Start + d, measure(sky)};
    }
    for (Event& event : events) {
        const double around = event.at;
        for (double d = -Step; d <= Step; d += 10 / JulianDay) {
            const double value = measure(skyOn(around + d));
            if (value > event.value) event = {around + d, value};
        }
    }
    return events;
}

// The point on the sphere where the shadow's axis comes down, if it does.
Vec3 underAxis(const Sky& sky) {
    const Vec3 along = normalized(sky.moon - sky.sun);
    const double reach = -dot(sky.moon, along), miss = length(sky.moon + along * reach);
    return sky.moon + along * (reach - std::sqrt(PlanetRadius * PlanetRadius - miss * miss));
}

// 2024–2025, greatest eclipse in TD (EclipseWise and NASA; the lunar ones of 2024 and of 14 Mar 2025
// from their UT plus 69 s):
//   - solar: 8 Apr 2024 18:18:29 (total, γ 0.3431), 2 Oct 2024 18:46:13 (annular, −0.3509),
//     29 Mar 2025 10:48:36 (partial, 1.0405), 21 Sep 2025 19:43:04 (partial, −1.0651);
//   - lunar: 25 Mar 2024 07:14 (penumbral, umbral −0.130), 18 Sep 2024 02:45:25 (partial, 0.087),
//     14 Mar 2025 06:59:56 (total, 1.180), 7 Sep 2025 18:12:58 (total, 1.362).
// A Moon off by 0.05° moves γ by 0.05 and the magnitude by 0.1, and the time by 6 min at the 0.5°/h the
// Moon gains on the Sun or the shadow; the tolerances are 0.06, 0.1 and 10 min.
void testEclipses() {
    struct Solar {
        double date, gamma;
        int kind;   // 0 partial, 1 total, 2 annular
    };
    constexpr Solar Solars[] = {{2460409.2628, 0.3431, 1}, {2460586.2821, -0.3509, 2}, {2460763.9504, 1.0405, 0}, {2460940.3216, -1.0651, 0}};
    constexpr double Late = 10 / 1440.0;
    const auto seen = [](const Sky& sky) {
        const SolarShadow shadow = solarShadow(sky);
        return shadow.axisDistance < EquatorialRadius + shadow.penumbraRadius;
    };
    const std::vector<Event> solar = eventsOf(seen, [](const Sky& sky) { return -solarShadow(sky).axisDistance; });
    expect(solar.size() == 4, "2024–2025 do not hold four solar eclipses", double(solar.size()));
    for (int i = 0; i < 4; ++i) {
        const Sky sky = skyOn(solar[i].at);
        const SolarShadow shadow = solarShadow(sky);
        const double gamma = (shadow.north ? 1 : -1) * shadow.axisDistance / EquatorialRadius;
        expect(std::abs(solar[i].at - Solars[i].date) < Late, "a solar eclipse's greatest phase is off NASA's (min)", (solar[i].at - Solars[i].date) * 1440);
        expect(std::abs(gamma - Solars[i].gamma) < 0.06, "a solar eclipse's gamma is off NASA's", gamma);
        const int kind = shadow.axisDistance > PlanetRadius ? 0 : shadow.umbraRadius > 0 ? 1 : 2;
        expect(kind == Solars[i].kind, "a solar eclipse is of the wrong kind", kind);
        if (kind == 0) continue;
        // Under the axis the local view agrees: the whole Sun hidden, or a ring of it left.
        const double hidden = sunCoverage(sky, underAxis(sky));
        expect(kind == 1 ? hidden > 0.999 : hidden > 0.8 && hidden < 0.95, "the local eclipse does not match the shadow", hidden);
        expect(lunarAge(sky) < 0.01 || lunarAge(sky) > 0.99, "a solar eclipse falls away from new moon", lunarAge(sky));
    }

    constexpr Event Lunars[] = {{2460394.8014, -0.130}, {2460571.6149, 0.087}, {2460748.7916, 1.180}, {2460926.2590, 1.362}};
    const std::vector<Event> lunar = eventsOf([](const Sky& sky) { return lunarShadow(sky, EquatorialRadius).penumbralMagnitude > 0; },
                                              [](const Sky& sky) { return lunarShadow(sky, EquatorialRadius).umbralMagnitude; });
    expect(lunar.size() == 4, "2024–2025 do not hold four lunar eclipses", double(lunar.size()));
    for (int i = 0; i < 4; ++i) {
        expect(std::abs(lunar[i].at - Lunars[i].at) < Late, "a lunar eclipse's greatest phase is off NASA's (min)", (lunar[i].at - Lunars[i].at) * 1440);
        expect(std::abs(lunar[i].value - Lunars[i].value) < 0.1, "a lunar eclipse's umbral magnitude is off NASA's", lunar[i].value);
    }
    const LunarShadow total = lunarShadow(skyOn(lunar[3].at), EquatorialRadius);
    expect(total.umbralCoverage == 1 && total.penumbralMagnitude > 2, "the Moon is not wholly in the umbra on 7 Sep 2025", total.umbralCoverage);
}

// The synodic month (29.5306 d) follows from the sidereal month and year; the node's 18.6-year
// regression shows in the Moon's reach over a year: 28.722° in 2025 (major standstill) and 18.646° in
// 2015 (near the minor one), from Horizons' hourly geocentric astrometric declination (ICRF).
void testMonths() {
    double first = 0, last = 0, previous = lunarAge(skyOn(Start));
    int newMoons = 0;
    for (double d = Step; d < 731; d += Step) {
        const double age = lunarAge(skyOn(Start + d));
        if (age < previous - 0.5) {
            if (newMoons == 0) first = d;
            last = d, ++newMoons;
        }
        previous = age;
    }
    const double synodic = (last - first) / (newMoons - 1);
    expect(std::abs(synodic - 29.5306) < 0.02, "the synodic month is off (d)", synodic);

    const auto reach = [](double from) {
        double most = 0;
        for (double d = 0; d < 366; d += 1 / 24.0) most = std::max(most, std::abs(normalized(skyOn(from + d).moonInertial).z));
        return std::asin(most) / Degree;
    };
    const double major = reach(2460676.5), minor = reach(2457023.5);
    expect(std::abs(major - 28.722) < 0.1, "the Moon's reach in 2025 is off Horizons (deg)", major);
    expect(std::abs(minor - 18.646) < 0.1, "the Moon's reach in 2015 is off Horizons (deg)", minor);

    // Phase: the full moon of 7 Sep 2025 lit wholly for Mexico City, the new moon of 8 Apr 2024 dark
    // (its parallax tilts the view by up to 1°, so a sliver of 10⁻⁴ may show).
    const Vec3 site = observerAt(19.4, -99.1);
    expect(moonIllumination(skyOn(2460926.259), site) > 0.999, "the eclipsed full moon is not wholly lit");
    expect(moonIllumination(skyOn(2460409.2628), site) < 1e-3, "the new moon is lit", moonIllumination(skyOn(2460409.2628), site));
}

// Today's state is today's constants; 620 Ma meets Williams (2000): a day of 21.9 ± 0.4 h, 400 ± 7
// days and 13.1 ± 0.1 synodic months a year, the Moon at 0.965 a₀; the Moon recedes 1.24 cm/yr between
// 2450 and 620 Ma. The young Sun gives 1/1.4 of today's light (Gough 1981).
void testDeepTime() {
    const DeepTime today = deepTimeAt(PresentAge);
    expect(today.moonAxis == MoonOrbitAxis && std::abs(today.siderealDay - SiderealDay) < 1e-6 && today.luminosity == SolarLuminosity &&
               today.calibrated,
           "today's deep state is not today's sky");
    expect(std::abs(solarDay(today.siderealDay) - JulianDay) < 0.01, "today's solar day is not 86 400 s", solarDay(today.siderealDay));

    const DeepTime elatina = deepTimeAt(PresentAge - ElatinaAge);
    const double day = solarDay(elatina.siderealDay), synodic = 1 / (1 / elatina.siderealMonth - 1 / SiderealYear);
    expect(std::abs(day / Hour - 21.9) < 0.4, "the day at 620 Ma is off Williams (h)", day / Hour);
    expect(std::abs(TropicalYear / day - 400) < 7, "days a year at 620 Ma are off Williams", TropicalYear / day);
    expect(std::abs(SiderealYear / synodic - 13.1) < 0.1, "synodic months a year at 620 Ma are off Williams", SiderealYear / synodic);
    expect(std::abs(elatina.moonAxis / MoonOrbitAxis - 0.965) < 1e-9, "the Moon at 620 Ma is off Williams", elatina.moonAxis / MoonOrbitAxis);
    const DeepTime weeli = deepTimeAt(PresentAge - OldestRhythmite);
    const double recession = (elatina.moonAxis - weeli.moonAxis) / (OldestRhythmite - ElatinaAge);
    expect(std::abs(recession - 0.0124) < 1e-9 && weeli.calibrated, "the Proterozoic recession is off Williams (m/yr)", recession);
    expect(!deepTimeAt(PresentAge - 3e9).calibrated && !deepTimeAt(PresentAge + 1e6).calibrated, "the stated range is not flagged");

    expect(std::abs(deepTimeAt(PresentAge - SunAgeToday).luminosity / SolarLuminosity - 1 / 1.4) < 1e-12, "the young Sun is not 1/1.4 as bright");
    // Across the world's ages (0 to 12 Gyr) everything moves one way and stays finite.
    DeepTime previous = deepTimeAt(0);
    for (double age = 1e8; age <= 12e9; age += 1e8) {
        const DeepTime next = deepTimeAt(age);
        expect(next.moonAxis > previous.moonAxis && next.siderealDay > previous.siderealDay && next.luminosity >= previous.luminosity,
               "deep time does not run one way", age);
        expect(next.siderealDay < next.siderealMonth && std::isfinite(next.siderealDay), "the day outgrows the month", age);
        previous = next;
    }

    // The sky of 620 Ma turns at that age's rates: mean noon to mean noon is its 22.1 h solar day (the
    // apparent day would add the equation of time's change, up to 30 s).
    const Vec3 up = upAt(0, 0);
    const auto noon = [&](double s) { return std::remainder(meanSolarTime(skyAt({J2000, s, PresentAge - ElatinaAge}), up) - 12, 24); };
    const double first = rising(noon, -2 * Hour, 4 * Hour, 0), second = rising(noon, first + day - 2 * Hour, 4 * Hour, 0);
    expect(std::abs(second - first - day) < 1, "the sky of 620 Ma does not keep its day (s)", second - first - day);
}

} // namespace

void testAstronomyTide();   // AstronomyTide.cpp

void testAstronomyEvents() {
    testEclipses();
    testMonths();
    testDeepTime();
    testAstronomyTide();
}
