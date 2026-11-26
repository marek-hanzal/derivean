#include "climate/type/Climate.hpp"

#include "chronology/constant/Timeline.hpp"
#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "planet/constant/Scale.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double OriginLatitude = 48.0;
constexpr uint64_t ClimateStream = 0x434c494d;
constexpr double CalmContrast = 0.45;       // °C per degree, times the tangent of latitude, below which no weather system grows
constexpr double FullContrast = 0.95;       // at which they grow at their full rate
constexpr double SteepestLatitude = 65.0;   // degrees beyond which the tangent stops counting

// Distance from the north pole in periods; the pole lies north (negative Z) of the origin.
double turnFromPole(double z) {
    const double pole = -(90.0 - OriginLatitude) / 360.0 * PlanetSpan;
    const double turn = (z - pole) / PlanetSpan;
    return turn - std::floor(turn);
}

double bell(double value, double centre, double width) {
    const double offset = (value - centre) / width;
    return std::exp(-offset * offset);
}

} // namespace

double latitudeAt(double z) {
    const double turn = turnFromPole(z);
    return 90.0 - 360.0 * std::min(turn, 1.0 - turn);
}

double northwardZ(double z) { return turnFromPole(z) < 0.5 ? -1.0 : 1.0; }

BeltWind beltWind(double latitude) {
    const double band = std::abs(latitude);
    BeltWind wind;
    // Trade easterlies, mid-latitude westerlies and polar easterlies.
    wind.east = -7.0 * bell(band, 14.0, 11.0) + 9.0 * bell(band, 46.0, 10.0) - 4.0 * bell(band, 77.0, 10.0);
    // Surface branches of the three overturning cells: towards the equator in the trades, towards
    // the pole under the westerlies, away from the pole in the cap. Zero at 0°, 30°, 60° and 90°.
    // Only the tropical cell is a steady overturning; the other two are what weather systems leave
    // in the mean, and are weak.
    const double overturning = 0.15 + 0.45 * (1.0 - smoothstep(25.0, 40.0, band));
    wind.poleward = -overturning * std::sin(6.0 * band * Pi / 180.0);
    return wind;
}

double storminess(double gradient, double latitude) {
    const double leverage = std::tan(std::min(std::abs(latitude), SteepestLatitude) * Pi / 180.0);
    return saturate((std::abs(gradient) * leverage - CalmContrast) / (FullContrast - CalmContrast));
}

double coolingSurface(double time) {
    return 288.0 + 1612.0 * std::exp(-time / (24.4 * Myr));
}

double condensedShare(double time) {
    return smoothstep(RainStart, OceansFull, time);
}

double greenhouseOffset(uint64_t seed, double time) {
    const double early = coolingSurface(time) - 288.0;
    const double swing = 5.5 * fractalNoise(stream(seed, ClimateStream), time / (260 * Myr), 0.37, 3);
    return early + swing * smoothstep(OceansFull, 400 * Myr, time);
}

double glacialOffset(uint64_t seed, double time) {
    // Ice ages need a cool world; in a greenhouse the orbital cycle leaves almost no mark.
    const double icehouse = smoothstep(3.0, -1.0, greenhouseOffset(seed, time));
    double phase = time / GlacialCycle;
    phase -= std::floor(phase);
    const double cooling = std::pow(phase, 1.5) * (1.0 - smoothstep(0.86, 1.0, phase));
    return -(1.5 + 5.5 * icehouse) * cooling;
}

ZonalClimate zonalClimate(uint64_t seed, double z, double time) {
    ZonalClimate zone;
    zone.latitude = latitudeAt(z);
    const double band = std::abs(zone.latitude);
    const double sine = std::sin(band * Pi / 180.0);
    zone.temperature = 27.0 - 49.0 * sine * sine + greenhouseOffset(seed, time) + glacialOffset(seed, time);
    zone.seasonal = 2.0 + 14.0 * sine;

    const BeltWind belt = beltWind(zone.latitude);
    const double polewardZ = (zone.latitude >= 0 ? 1.0 : -1.0) * northwardZ(z);
    zone.wind = {belt.east, belt.poleward * polewardZ};

    // Wet equatorial belt, dry subtropics, wet storm track, dry poles.
    const double tropics = 2.3 * std::exp(-(band / 11.0) * (band / 11.0));
    const double stormTrack = 1.15 * std::exp(-((band - 50.0) / 14.0) * ((band - 50.0) / 14.0));
    zone.rain = 0.22 + tropics + stormTrack;
    return zone;
}

} // namespace derivean::sim
