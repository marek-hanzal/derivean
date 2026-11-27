// The sky against Earth's measured one. Positions come from JPL Horizons (Giorgini et al. 1996;
// ssd.jpl.nasa.gov, DE441 of Park et al. 2021, queried 26 Nov 2026; the queries are in .out/astronomy/horizons/*.sh): geocentric astrometric RA/Dec
// in ICRF and topocentric airless apparent elevation, at Julian dates in TT. The model's own error
// budget sets each tolerance.
#include "AstronomyFixture.hpp"

#include "astronomy/fn/Place.hpp"

#include <algorithm>

void testAstronomyEvents();   // AstronomyEvents.cpp

using namespace derivean::sim;
using namespace derivean::sim::astronomytest;

namespace {

constexpr double Dates[] = {2446895.5, 2451545.0, 2460389.625, 2460483.0, 2460926.25, 2461371.25};   // JD, TT

// Horizons, geocentric (500@399), astrometric RA and Dec (ICRF, degrees) and distance (AU), per date.
struct Geocentric {
    double rightAscension, declination, distance;
};
constexpr Geocentric SunFromHorizons[] = {
    {18.263930755, 7.738419892, 1.00166555759740},  {281.288163619, -23.033310040, 0.98332763198913},
    {359.691272731, -0.134018909, 0.99586194712201}, {90.290047393, 23.435661484, 1.01623501943328},
    {166.201317306, 5.903172273, 1.00757672803477},  {242.127551222, -20.966140071, 0.98689530175014},
};
constexpr Geocentric MoonFromHorizons[] = {
    {153.782526504, 14.574959371, 0.00264728099479}, {222.450309522, -10.900636339, 0.00268997545281},
    {126.992605200, 24.255617742, 0.00268207925570}, {262.529296677, -28.021007092, 0.00255535109052},
    {346.214813522, -6.203873434, 0.00247126334149}, {94.585997516, 27.196031763, 0.00240641749415},
};

// Horizons, topocentric (geodetic site, height 0), airless apparent elevation in degrees, per date.
struct Site {
    double latitude, longitude;
    double sun[6], moon[6];
};
constexpr Site Sites[] = {
    {51.4769, 0.0, {-30.853337746, 15.482090493, -27.169901248, 61.952559985, 4.352310361, -18.059458932},
     {38.502416981, 9.387022868, 14.544551828, -66.365562325, -5.582959672, 3.782500768}},   // Greenwich
    {35.6812, 139.7671, {43.363241877, -52.151761492, 54.231852278, -19.298453113, -27.435457216, -42.025993385},
     {-40.265184460, -61.003954016, -11.456512654, 18.073503010, 26.354663355, 70.543570289}},   // Tokyo
    {19.4326, -99.1332, {11.704165853, -16.510842804, -31.574050492, -0.945552517, 73.827011823, 49.067596744},
     {36.390411698, 38.478078050, 84.083466503, -8.123503210, -74.332045212, -31.010782097}},   // Mexico City
    {69.6492, 18.9553, {-11.659406366, -3.570003802, -9.459285768, 42.533793645, -1.125653341, -27.211717658},
     {22.149096831, -6.564510093, 13.751858846, -46.447853183, -0.150163022, 20.835663632}},   // Tromsø
};

// Budgets. Sun: the Kepler orbit leaves out the planets (≤ 0.005°). Moon: the truncated series leaves
// out terms below 0.01° (0.046° at most against Horizons' hourly 2015 and 2025), and the inclination is
// held (0.017°). Elevations add what the geocentric
// frame lacks: aberration 0.006°, nutation 0.005° and ΔT held at 69.2 s (1987: 55.6 s, so 0.06°).
constexpr double SunDirection = 0.02, SunDistance = 1e-4;            // degrees, relative
constexpr double MoonDirection = 0.1, MoonDistance = 2e-3;
constexpr double SunHeight = 0.08, MoonHeight = 0.15;                // degrees

void testAgainstHorizons() {
    for (int i = 0; i < 6; ++i) {
        const Sky sky = skyOn(Dates[i]);
        const Geocentric sun = SunFromHorizons[i], moon = MoonFromHorizons[i];
        const double sunOff = degreesBetween(normalized(sky.sunInertial), equatorial(sun.rightAscension, sun.declination));
        const double moonOff = degreesBetween(normalized(sky.moonInertial), equatorial(moon.rightAscension, moon.declination));
        expect(sunOff < SunDirection, "the Sun's direction is off Horizons (deg)", sunOff);
        expect(moonOff < MoonDirection, "the Moon's direction is off Horizons (deg)", moonOff);
        expect(std::abs(length(sky.sun) / (sun.distance * AstronomicalUnit) - 1) < SunDistance, "the Sun's distance is off Horizons", length(sky.sun));
        expect(std::abs(length(sky.moon) / (moon.distance * AstronomicalUnit) - 1) < MoonDistance, "the Moon's distance is off Horizons", length(sky.moon));
        // The Earth frame carries the same directions.
        expect(std::abs(length(sky.sun) - length(sky.sunInertial)) < 1e-3 && std::abs(length(sky.moon) - length(sky.moonInertial)) < 1e-6,
               "the Earth frame changes a distance");
        for (const Site& site : Sites) {
            const double sunHeight = sunElevation(sky, upAt(site.latitude, site.longitude)) / Degree;
            const double moonHeight = moonElevation(sky, observerAt(site.latitude, site.longitude)) / Degree;
            expect(std::abs(sunHeight - site.sun[i]) < SunHeight, "the Sun's elevation is off Horizons (deg)", sunHeight - site.sun[i]);
            expect(std::abs(moonHeight - site.moon[i]) < MoonHeight, "the Moon's elevation is off Horizons (deg)", moonHeight - site.moon[i]);
        }
    }
    // One instant two ways: an epoch, or seconds run from J2000 at today's rates.
    const Sky direct = skyOn(Dates[4]), run = skyOn(J2000, (Dates[4] - J2000) * JulianDay);
    expect(degreesBetween(direct.moon, run.moon) < 1e-7 && degreesBetween(direct.sun, run.sun) < 1e-7, "epoch and elapsed seconds disagree");
}

// Horizons, airless elevation of the Sun's centre at one-minute steps, interpolated to −0.8333° (34'
// refraction plus 16' semidiameter, the convention of Urban & Seidelmann 2013): minutes after the given
// JD (TT) midnight.
void testSunrise() {
    struct Rise {
        double date, latitude, longitude, minutes;
    };
    constexpr Rise Rises[] = {
        {2460482.5, 50.0755, 14.4378, 2 * 60 + 53.78},     // Prague, 2024-06-21
        {2460665.5, 50.0755, 14.4378, 6 * 60 + 59.85},     // Prague, 2024-12-21
        {2460389.5, 0.0, -78.5, 11 * 60 + 19.15},          // the equator, 2024-03-20
        {2461369.5, 35.6812, 139.7671, 21 * 60 + 28.21},   // Tokyo, 2026-11-25 (06:28 JST on the 26th)
    };
    for (const Rise& rise : Rises) {
        const Vec3 up = upAt(rise.latitude, rise.longitude);
        const auto height = [&](double s) { return sunElevation(skyOn(rise.date, s), up) / Degree; };
        const double guess = rise.minutes * Minute;
        const double found = rising(height, guess - Hour, 2 * Hour, -0.8333) / Minute;
        // The Sun climbs 0.1–0.25° per minute at these places, so 0.08° of elevation is under a minute.
        expect(std::abs(found - rise.minutes) < 1.0, "sunrise is off Horizons (min)", found - rise.minutes);
    }
}

double highestOfDay(double date, Vec3 up, double sign) {
    double best = -1e9;
    for (int m = 0; m < 24 * 60; m += 5) best = std::max(best, sign * sunElevation(skyOn(date, m * Minute), up) / Degree);
    return sign * best;
}

// 70° N sees no sunrise while the declination is below −20.8° (late November to mid January) and no
// sunset around midsummer; the equator keeps twelve hours.
void testPolarNight() {
    const Vec3 north = upAt(70, 19), equator = upAt(0, 19);
    expect(highestOfDay(2460664.5, north, 1) < -0.8333, "70° N sees the Sun on 20 Dec 2024", highestOfDay(2460664.5, north, 1));
    expect(highestOfDay(2460654.5, north, 1) < -0.8333, "70° N sees the Sun on 10 Dec 2024");
    expect(highestOfDay(2460629.5, north, 1) > -0.8333, "70° N has polar night already on 15 Nov 2024");
    expect(highestOfDay(2460700.5, north, 1) > -0.8333, "70° N still has polar night on 25 Jan 2025");
    expect(highestOfDay(2460482.5, north, -1) > -0.8333, "70° N has no midnight sun on 21 Jun 2024");
    int up = 0;
    for (int m = 0; m < 24 * 60; ++m) up += sunElevation(skyOn(2460389.5, m * Minute), equator) > 0;
    expect(std::abs(up - 12 * 60) < 6, "the equator's equinox day is not twelve hours (min)", up);
}

// Equinoxes and apsides of 2024 (Horizons for the apsides, from its 10-minute distance table; USNO
// "Earth's Seasons" for the equinoxes), the seasons' 365.2422-day year, 1/r² and 1361 W/m² at 1 AU.
void testYear() {
    const auto declination = [](double s) { return sunDeclination(skyOn(2460388.5, s)); };
    const double spring = 2460388.5 + rising(declination, 0, 3 * JulianDay, 0) / JulianDay;
    const auto nextDeclination = [](double s) { return sunDeclination(skyOn(2460753.5, s)); };
    const double nextSpring = 2460753.5 + rising(nextDeclination, 0, 3 * JulianDay, 0) / JulianDay;
    // Published equinoxes are of the apparent Sun: 2024-03-20 03:06 UT and 2025-03-20 09:01 UT. The
    // geometric Sun crosses 8.3 min earlier (aberration, 20.5"); nutation (≤ 1.5 min here) and the
    // Sun's 0.005° leave a budget of 10 min.
    constexpr double Aberration = 8.3;
    expect(std::abs(spring - (2460389.5 + (3 * 60 + 7.2 - Aberration) / 1440.0)) < 10 / 1440.0, "the March equinox of 2024 is off (d)", spring - 2460389.5);
    expect(std::abs(nextSpring - (2460754.5 + (9 * 60 + 2.2 - Aberration) / 1440.0)) < 10 / 1440.0, "the March equinox of 2025 is off", nextSpring - 2460754.5);
    // That year was 365.2465 d long. The Earth's 6" swing about the barycentre moves each equinox by up
    // to 2.4 min, so ten years give the tropical year to 5 × 10⁻⁴ d.
    expect(std::abs(nextSpring - spring - 365.2465) < 3 / 1440.0, "the equinoxes of 2024 and 2025 are not 365.2465 d apart", nextSpring - spring);
    const auto laterDeclination = [](double s) { return sunDeclination(skyOn(2460388.5 + 3652, s)); };
    const double laterSpring = 2460388.5 + 3652 + rising(laterDeclination, 0, 3 * JulianDay, 0) / JulianDay;
    expect(std::abs((laterSpring - spring) / 10 - 365.24219) < 5e-4, "equinoxes do not come a tropical year apart (d)", (laterSpring - spring) / 10);

    double nearest = 1e30, nearestAt = 0, farthest = 0, farthestAt = 0;
    for (double d = 0; d < 6; d += 1 / 144.0) {
        const double nearDistance = length(skyOn(2460310.5 + d).sun), farDistance = length(skyOn(2460493.5 + d).sun);
        if (nearDistance < nearest) nearest = nearDistance, nearestAt = 2460310.5 + d;
        if (farDistance > farthest) farthest = farDistance, farthestAt = 2460493.5 + d;
    }
    // Horizons: 2024-01-03 00:40 TT at 0.98330695 AU and 2024-07-05 05:10 TT at 1.01672553 AU. The
    // distance changes by only 10⁻⁵ AU in a day there, so the model's 5 × 10⁻⁵ AU (the Sun's distance
    // budget) allows hours in the date.
    expect(std::abs(nearestAt - (2460312.5 + 40 / 1440.0)) < 0.25, "perihelion of 2024 is off (d)", nearestAt - 2460312.5);
    expect(std::abs(farthestAt - (2460496.5 + 310 / 1440.0)) < 0.25, "aphelion of 2024 is off (d)", farthestAt - 2460496.5);
    expect(std::abs(nearest / AstronomicalUnit - 0.98330695) < 5e-5, "perihelion distance is off (AU)", nearest / AstronomicalUnit);
    expect(std::abs(farthest / AstronomicalUnit - 1.01672553) < 5e-5, "aphelion distance is off (AU)", farthest / AstronomicalUnit);

    const Sky mean = skyOn(J2000);
    const double atUnit = irradiance(mean) * std::pow(length(mean.sun) / AstronomicalUnit, 2);
    expect(std::abs(atUnit - 1361) < 1, "irradiance at 1 AU is not 1361 W/m²", atUnit);
    const double perihelion = irradiance(skyOn(nearestAt)) / atUnit, aphelion = irradiance(skyOn(farthestAt)) / atUnit;
    expect(std::abs(perihelion - 1.0343) < 2e-4 && std::abs(aphelion - 0.9674) < 2e-4, "1/r² does not give +3.4 % and −3.3 %", perihelion);
    const Vec3 up = upAt(48, 15);
    const double level = insolation(mean, up), across = irradiance(mean) * std::sin(sunElevation(mean, up));
    expect(std::abs(level - std::max(0.0, across)) < 1e-9, "level insolation is not irradiance by the Sun's height", level);
}

// Solar time follows the longitude; the equation of time against Meeus (example 28.b, 1992 Oct 13.0
// TD: +13 min 42.6 s) and its yearly extremes (−14.2 min about 11 Feb, +16.4 min about 3 Nov).
void testSolarTime() {
    const Sky sky = skyOn(2460483.0);
    const double greenwich = solarTime(sky, upAt(51.5, 0)), east = solarTime(sky, upAt(-30, 15)), west = solarTime(sky, upAt(10, -150));
    expect(std::abs(east - greenwich - 1) < 1e-9 && std::abs(std::remainder(west - greenwich + 10, 24)) < 1e-9, "solar time does not follow the longitude");
    // At 12:00 TT, UT1 is 69.2 s earlier, and mean solar time at Greenwich is UT1.
    const double mean = meanSolarTime(sky, upAt(51.5, 0));
    expect(std::abs(mean - (12 - DeltaT / Hour)) < 3 / Hour, "mean solar time at Greenwich is not UT1 (h)", mean);
    expect(std::abs(greenwich - mean - equationOfTime(sky) / Hour) < 1e-9, "apparent minus mean time is not the equation of time");

    const double meeus = equationOfTime(skyOn(2448908.5));
    expect(std::abs(meeus - (13 * 60 + 42.6)) < 6, "the equation of time is off Meeus 28.b (s)", meeus);
    double lowest = 1e9, highest = -1e9, lowestDay = 0, highestDay = 0;
    for (int day = 0; day < 366; ++day) {
        const double value = equationOfTime(skyOn(2460310.5 + day));
        if (value < lowest) lowest = value, lowestDay = day;
        if (value > highest) highest = value, highestDay = day;
    }
    expect(std::abs(lowest / Minute + 14.2) < 0.2 && std::abs(lowestDay - 41) <= 3, "the February extreme of the equation of time is off (min)", lowest / Minute);
    expect(std::abs(highest / Minute - 16.4) < 0.2 && std::abs(highestDay - 307) <= 3, "the November extreme of the equation of time is off (min)", highest / Minute);
}

// The closed-form daily mean against the mean of insolation sampled every minute over the 24 h
// centred on the instant, at every 15° of latitude and either side of the polar circles on four dates (solstices,
// equinox, perihelion); and its two ends, polar day (the pole's sin δ) and polar night (zero).
void testDailyInsolation() {
    double worst = 0;
    for (double date : {2460389.625, 2460483.0, 2460312.5, 2461030.5})
        for (int row = 0; row <= 16; ++row) {
            constexpr double Circle[] = {-66.7, -66.4, 66.4, 66.7};
            const double latitude = row < 13 ? -90 + 15 * row : Circle[row - 13];
            const Vec3 up = upAt(latitude, 15);
            double sampled = 0;
            for (int m = 0; m < 24 * 60; ++m) sampled += insolation(skyOn(date, (m + 0.5 - 12 * 60) * Minute), up);
            sampled /= 24 * 60;
            const Sky sky = skyOn(date);
            worst = std::max(worst, std::abs(dailyMeanInsolation(sky, up) - sampled) / irradiance(sky));
        }
    // Measured 8.4e-4 (1.1 W/m²): the declination's move over the sampled day, largest where the
    // half-day is most sensitive to it, near the polar circles.
    expect(worst < 1e-3, "the daily mean is off the sampled day (share of the irradiance)", worst);
    const Sky june = skyOn(2460483.0);
    const double pole = dailyMeanInsolation(june, {0, 0, 1}), sinDeclination = std::sin(sunDeclination(june));
    expect(std::abs(pole - irradiance(june) * sinDeclination) < 1e-9 * irradiance(june), "the pole's polar day is not S sin δ", pole);
    expect(dailyMeanInsolation(june, {0, 0, -1}) == 0 && dailyMeanInsolation(june, upAt(-70, 0)) == 0, "polar night is not dark");
}

} // namespace

void testAstronomy() {
    testAgainstHorizons();
    testSunrise();
    testPolarNight();
    testYear();
    testSolarTime();
    testDailyInsolation();
    testAstronomyEvents();
}
