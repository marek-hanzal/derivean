#include "astronomy/fn/Orbit.hpp"

#include "astronomy/constant/Sky.hpp"

#include <cmath>
#include <numbers>

namespace derivean::sim {
namespace {

constexpr double Turn = 2 * std::numbers::pi;
constexpr double Degree = std::numbers::pi / 180;

// The equinox regresses by the gap between the tropical and the sidereal year.
constexpr double PrecessionRate = Turn * (1 / TropicalYear - 1 / SiderealYear);   // rad/s

struct Rates {
    double moonLongitude, moonPerigee, moonNode;   // rad/s, inertial
};

Rates ratesOf(double siderealMonth, double apsidalPeriod, double nodalPeriod) {
    return {Turn / siderealMonth, Turn / apsidalPeriod - PrecessionRate, -Turn / nodalPeriod - PrecessionRate};
}

// The Sun's perturbations of the Moon beyond the ellipse (Meeus 1998, tables 47.A and 47.B, terms of
// 0.01° and more): coefficients of sin (longitude, latitude; degrees) and cos (distance; km) of D, M,
// M' and F combined. Terms of M' and F alone come from the ellipse and the inclined plane, which are
// solved exactly here, except sin(M' − 2F): the plane gives −0.012666° of Meeus's +0.010980°, and the
// rest is the Sun's.
struct Term {
    int d, m, moon, f;
    double value;
};

constexpr Term Longitude[] = {
    {2, 0, -1, 0, 1.274027}, {2, 0, 0, 0, 0.658314}, {0, 1, 0, 0, -0.185116}, {2, 0, -2, 0, 0.058793},
    {2, -1, -1, 0, 0.057066}, {2, 0, 1, 0, 0.053322}, {2, -1, 0, 0, 0.045758}, {0, 1, -1, 0, -0.040923},
    {1, 0, 0, 0, -0.034720}, {0, 1, 1, 0, -0.030383}, {0, 0, 1, -2, 0.023646}, {2, 0, 0, -2, 0.015327},
    {4, 0, -1, 0, 0.010675},
};

constexpr Term Latitude[] = {
    {2, 0, 0, -1, 0.173237}, {2, 0, -1, 1, 0.055413}, {2, 0, -1, -1, 0.046271}, {2, 0, 0, 1, 0.032573},
    {2, 0, 1, -1, 0.009266},
};

constexpr Term Distance[] = {
    {2, 0, -1, 0, -3699.111}, {2, 0, 0, 0, -2955.968}, {2, 0, -2, 0, 246.158}, {2, -1, 0, 0, -204.586},
    {2, 0, 1, 0, -170.733}, {2, -1, -1, 0, -152.138}, {0, 1, -1, 0, -129.620}, {1, 0, 0, 0, 108.743},
    {0, 1, 1, 0, 104.755},
};

template <std::size_t N>
double series(const Term (&terms)[N], double d, double m, double moon, double f, bool cosine) {
    double sum = 0;
    for (const Term& term : terms) {
        const double angle = term.d * d + term.m * m + term.moon * moon + term.f * f;
        sum += term.value * (cosine ? std::cos(angle) : std::sin(angle));
    }
    return sum;
}

Vec3 spherical(double longitude, double latitude, double distance) {
    return Vec3{std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(latitude)} * distance;
}

} // namespace

MeanElements meanElementsAt(const Instant& instant, const DeepTime& deep) {
    const double before = (instant.epoch - J2000) * JulianDay, after = instant.seconds, total = before + after;
    const Rates today = ratesOf(SiderealMonth, ApsidalPeriod, NodalPeriod);
    const Rates then = ratesOf(deep.siderealMonth, deep.apsidalPeriod, deep.nodalPeriod);

    MeanElements elements;
    elements.precession = PrecessionRate * total;
    elements.sunAnomaly = SunMeanAnomalyJ2000 * Degree + Turn * total / AnomalisticYear;
    const double sunPerigee = SunPerigeeJ2000 * Degree + Turn * total * (1 / SiderealYear - 1 / AnomalisticYear);
    elements.sunLongitude = sunPerigee + elements.sunAnomaly;

    elements.moonLongitude = MoonMeanLongitudeJ2000 * Degree + today.moonLongitude * before + then.moonLongitude * after;
    const double moonPerigee = (MoonMeanLongitudeJ2000 - MoonMeanAnomalyJ2000) * Degree + today.moonPerigee * before + then.moonPerigee * after;
    elements.moonNode = (MoonMeanLongitudeJ2000 - MoonArgumentOfLatitudeJ2000) * Degree + today.moonNode * before + then.moonNode * after;
    elements.moonAnomaly = elements.moonLongitude - moonPerigee;
    return elements;
}

double eccentricAnomaly(double meanAnomaly, double eccentricity) {
    const double m = std::remainder(meanAnomaly, Turn);
    double e = m + eccentricity * std::sin(m);
    for (int i = 0; i < 8; ++i) e -= (e - eccentricity * std::sin(e) - m) / (1 - eccentricity * std::cos(e));
    return e;
}

namespace {

// True anomaly and radius on an ellipse of unit axis.
struct Ellipse {
    double anomaly, radius;
};

Ellipse ellipse(double meanAnomaly, double eccentricity) {
    const double e = eccentricAnomaly(meanAnomaly, eccentricity);
    const double anomaly = 2 * std::atan2(std::sqrt(1 + eccentricity) * std::sin(e / 2), std::sqrt(1 - eccentricity) * std::cos(e / 2));
    return {anomaly, 1 - eccentricity * std::cos(e)};
}

} // namespace

Vec3 sunFromBarycentre(const MeanElements& elements) {
    const Ellipse orbit = ellipse(elements.sunAnomaly, EarthOrbitEccentricity);
    const double perigee = elements.sunLongitude - elements.sunAnomaly;
    return spherical(perigee + orbit.anomaly, 0, EarthOrbitAxis * orbit.radius);
}

Vec3 moonFromEarth(const MeanElements& elements, const DeepTime& deep) {
    const Ellipse orbit = ellipse(elements.moonAnomaly, MoonEccentricity);
    const double perigee = elements.moonLongitude - elements.moonAnomaly;
    const double inclination = MoonInclination * Degree;
    // Argument of latitude: from the ascending node along the inclined plane.
    const double u = perigee + orbit.anomaly - elements.moonNode;
    double longitude = elements.moonNode + std::atan2(std::cos(inclination) * std::sin(u), std::cos(u));
    double latitude = std::asin(std::sin(inclination) * std::sin(u));

    const double d = elements.moonLongitude - elements.sunLongitude, m = elements.sunAnomaly;
    const double moon = elements.moonAnomaly, f = elements.moonLongitude - elements.moonNode;
    longitude += series(Longitude, d, m, moon, f, false) * Degree;
    latitude += series(Latitude, d, m, moon, f, false) * Degree;
    const double distance = deep.moonAxis * orbit.radius + series(Distance, d, m, moon, f, true) * 1e3 * deep.moonAxis / MoonOrbitAxis;
    return spherical(longitude, latitude, distance);
}

} // namespace derivean::sim
