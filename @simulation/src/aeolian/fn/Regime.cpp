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
constexpr int Span = 2 * Reach + 1;

// Gaussian weights of the quadrature points and their sum, in the order addMonth visits them.
struct Quadrature {
    double share[Span][Span]{};
    double total{};
};

const Quadrature& quadrature() {
    static const Quadrature weights = [] {
        Quadrature q;
        for (int b = -Reach; b <= Reach; ++b)
            for (int a = -Reach; a <= Reach; ++a) {
                q.share[b + Reach][a + Reach] = std::exp(-0.125 * (a * a + b * b));
                q.total += q.share[b + Reach][a + Reach];
            }
        return q;
    }();
    return weights;
}

// The mean wind of one month at a latitude: the belts there, moved with the sun.
Vec2 monthWind(double latitude, double northZ, int month) {
    const double day = (month + 0.5) * YearDays / Months;
    const double belt = latitude - BeltShift * declinationOn(day - BeltLag) * std::cos(latitude * Pi / 180.0);
    const BeltWind wind = beltWind(belt);
    return {wind.east, wind.poleward * (belt >= 0 ? 1.0 : -1.0) * northZ};
}

} // namespace

double thresholdFriction(double grain) { return 0.1 * std::sqrt((GrainDensity - NearGroundAir) / NearGroundAir * Gravity * grain); }

double saturatedFlux(double speed) {
    static const double threshold = thresholdFriction(DuneGrain);
    static const double profile = std::log(WindHeight / SandRoughness);
    const double friction = 0.4 * speed / profile;
    if (friction <= threshold) return 0;
    return FluxCoefficient * NearGroundAir / Gravity * friction * friction * (friction - threshold) * YearSeconds / SandBulkDensity;
}

void addMonth(WindRegime& regime, Vec2 monthly, double spread, double weight) {
    const Quadrature& weights = quadrature();
    std::array<double, RoseSectors> rose{};
    for (int b = -Reach; b <= Reach; ++b) {
        for (int a = -Reach; a <= Reach; ++a) {
            const double share = weights.share[b + Reach][a + Reach];
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
    for (int k = 0; k < RoseSectors; ++k) regime.drift[size_t(k)] += weight * rose[size_t(k)] / weights.total;
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
    for (int month = 0; month < Months; ++month) addMonth(regime, monthWind(latitude, northZ, month), spread, 1.0 / Months);
    closeRose(regime);
    return regime;
}

Vec2 meanWindAtLatitude(double latitude, double northZ) {
    Vec2 mean;
    for (int month = 0; month < Months; ++month) mean = mean + monthWind(latitude, northZ, month) * (1.0 / Months);
    return mean;
}

WindRegime regimeAt(uint64_t seed, Vec2 position, double time) { return regimeAtPoint(seed, planetPoint(seed, position), time); }

WindRegime regimeAtPoint(uint64_t seed, Vec2 point, double time) {
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
