#include "aeolian/fn/Regime.hpp"

#include "aeolian/constant/Wind.hpp"
#include "chronology/type/Calendar.hpp"
#include "climate/constant/Moisture.hpp"
#include "climate/fn/Seasons.hpp"
#include "climate/type/Climate.hpp"
#include "hydrology/constant/Water.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double FluxCoefficient = 4.2;   // Lettau and Lettau, for grains of DuneGrain
// The stepped atmosphere moves its belts with the sun by these two numbers (atmosphere/fn/Season.cpp);
// the regime follows the same year in closed form.
constexpr double BeltShift = 0.35;        // degrees of belt per degree of solar declination
constexpr double BeltLag = 30.0;          // days
// Root-mean-square wind of the weather systems where the storm track is at full strength; the
// stepped atmosphere's eddies saturate near this.
constexpr double StormWind = 6.0;         // m/s
constexpr int Months = 12;
constexpr int Reach = 6;                  // quadrature points on either side of the mean, half a deviation apart

} // namespace

double thresholdFriction(double grain) { return 0.1 * std::sqrt((GrainDensity - NearGroundAir) / NearGroundAir * Gravity * grain); }

double saturatedFlux(double speed) {
    static const double threshold = thresholdFriction(DuneGrain);
    const double friction = 0.4 * speed / std::log(WindHeight / SandRoughness);
    if (friction <= threshold) return 0;
    return FluxCoefficient * NearGroundAir / Gravity * friction * friction * (friction - threshold) * YearSeconds / SandBulkDensity;
}

void addMonth(WindRegime& regime, Vec2 monthly, double spread, double weight) {
    double total = 0;
    std::array<double, RoseSectors> rose{};
    for (int b = -Reach; b <= Reach; ++b) {
        for (int a = -Reach; a <= Reach; ++a) {
            const double share = std::exp(-0.125 * (a * a + b * b));
            total += share;
            const Vec2 wind{monthly.x + 0.5 * spread * a, monthly.z + 0.5 * spread * b};
            const double flux = saturatedFlux(length(wind));
            if (flux <= 0) continue;
            // Shared between the two sectors the heading lies between.
            double at = std::atan2(wind.z, wind.x) * (RoseSectors / (2 * Pi));
            if (at < 0) at += RoseSectors;
            const int low = int(at) % RoseSectors;
            const double part = at - std::floor(at);
            rose[size_t(low)] += share * flux * (1 - part);
            rose[size_t((low + 1) % RoseSectors)] += share * flux * part;
        }
    }
    for (int k = 0; k < RoseSectors; ++k) regime.drift[size_t(k)] += weight * rose[size_t(k)] / total;
    regime.mean = regime.mean + monthly * weight;
    regime.spread += weight * spread;
}

void closeRose(WindRegime& regime) {
    regime.potential = 0;
    regime.resultant = {};
    for (int k = 0; k < RoseSectors; ++k) {
        regime.potential += regime.drift[size_t(k)];
        regime.resultant = regime.resultant + sectorHeading(k) * regime.drift[size_t(k)];
    }
}

WindRegime regimeAtLatitude(double latitude, double northZ, double gradient) {
    WindRegime regime;
    const double spread = std::hypot(Gustiness, StormWind * storminess(gradient, latitude));
    for (int month = 0; month < Months; ++month) {
        const double day = (month + 0.5) * YearDays / Months;
        const double belt = latitude - BeltShift * declinationOn(day - BeltLag) * std::cos(latitude * Pi / 180.0);
        const BeltWind wind = beltWind(belt);
        addMonth(regime, {wind.east, wind.poleward * (belt >= 0 ? 1.0 : -1.0) * northZ}, spread, 1.0 / Months);
    }
    closeRose(regime);
    return regime;
}

WindRegime regimeAt(uint64_t seed, Vec2 position, double time) {
    const Vec2 point = planetPoint(seed, position);
    const double degree = PlanetSpan / 360.0 * northwardZ(point.z);   // one degree of latitude northwards, along Z
    const ZonalClimate north = zonalClimate(seed, point.z + degree, time);
    const ZonalClimate south = zonalClimate(seed, point.z - degree, time);
    const double across = north.latitude - south.latitude;
    const double gradient = std::abs(across) > 1e-6 ? (north.temperature - south.temperature) / across : 0.0;
    return regimeAtLatitude(latitudeAt(point.z), northwardZ(point.z), gradient);
}

WindRegime regimeOf(int winds, const double* headings, const double* speeds, const double* shares) {
    WindRegime regime;
    for (int k = 0; k < winds; ++k) addMonth(regime, Vec2{std::cos(headings[k]), std::sin(headings[k])} * speeds[k], 0.0, shares[k]);
    closeRose(regime);
    return regime;
}

double dryShare(double rain, double warmth, double halfRange) {
    const double dry = 1.0 / (1.0 + DampGain * std::max(0.0, rain) / evaporationDemand(float(warmth)));
    return dry * (1.0 - shareOfYearBelow(0.0, warmth, halfRange) * (1.0 - dry));
}

float aridityOf(float rain, float celsius) { return std::clamp(1.0f - std::max(0.0f, rain) / evaporationDemand(celsius), 0.0f, 1.0f); }

float runningShareOf(float rain, float celsius) { return 1.0f - aridityOf(rain, celsius) * (1.0f - FloodShare); }

} // namespace derivean::sim
