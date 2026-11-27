#include "atmosphere/fn/SphereSeason.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "chronology/type/Calendar.hpp"
#include "climate/constant/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "core/type/Vec.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace derivean::sim {
namespace {

constexpr double Degrees = 180.0 / std::numbers::pi;

// Latitude the belts of a latitude correspond to once they have followed the sun.
double beltLatitude(double latitude, double shift) { return latitude - shift * std::cos(latitude / Degrees); }

// The share of the belts' wind kept at a boundary of sine `sine`: 1 to SpherePolarCap, then
// x (3 − x²) / 2 with x = cos φ / cos PolarCap. Linear in cos φ at the pole, so u / cos φ and the
// polar band's vorticity stay finite as the grid refines; with zero slope at PolarCap, so the wind's
// derivative, and with it the jet, has no step there for the closure to relax the flow to.
double polarShare(double sine) {
    const double x = std::min(1.0, std::sqrt(std::max(0.0, 1 - sine * sine)) / SpherePolarCapCosine);
    return 0.5 * x * (3 - x * x);
}

// Each half's contrast in `field`, north then south: the area-weighted mean of its subtropical bands
// less that of its subpolar ones. As differences of means a constant cancels, as it does between the
// flat world's equal counts of rows.
void contrasts(const LatitudeBands& bands, const std::vector<double>& field, double (&contrast)[2]) {
    double sum[2][2]{}, area[2][2]{};   // [half][subtropics, subpolar]
    for (int b = 0; b < bands.count; ++b) {
        const double latitude = bands.centre[size_t(b)] * Degrees, band = std::abs(latitude);
        const int side = band >= 15 && band < 35 ? 0 : band >= 55 && band < 75 ? 1 : -1;
        if (side < 0) continue;
        const int half = latitude >= 0 ? 0 : 1;
        const double content = bands.area[size_t(b)] * field[size_t(b)];
        sum[half][side] += content;
        area[half][side] += bands.area[size_t(b)];
    }
    for (int half = 0; half < 2; ++half) {
        if (!(area[half][0] > 0 && area[half][1] > 0)) throw std::invalid_argument("a half has no subtropical or subpolar band");
        contrast[half] = sum[half][0] / area[half][0] - sum[half][1] / area[half][1];
    }
}

} // namespace

SphereSeason sphereSeasonOf(const LatitudeBands& bands, double day, const std::vector<double>& temperature,
                            const std::vector<double>& base) {
    const size_t count = size_t(bands.count);
    if (!std::isfinite(day)) throw std::invalid_argument("the day is not finite");
    if (temperature.size() != count || base.size() != count) throw std::invalid_argument("a field is not one value per band");
    for (size_t b = 0; b < count; ++b)
        if (!std::isfinite(temperature[b]) || !std::isfinite(base[b])) throw std::invalid_argument("a band's temperature is not finite");
    SphereSeason season;
    season.declination = declinationOn(day);
    const double shift = SphereBeltShift * declinationOn(day - SphereBeltLag);
    double contrast[2], reference[2];
    contrasts(bands, temperature, contrast);
    contrasts(bands, base, reference);
    double strength[2];
    for (int half = 0; half < 2; ++half) {
        if (!(reference[half] > 0)) throw std::invalid_argument("the base's subtropics are not warmer than its subpolar bands");
        strength[half] = std::clamp(contrast[half] / reference[half], 0.5, 1.8);
    }

    season.eastward.resize(count + 1);
    season.northward.resize(count + 1);
    for (size_t k = 0; k <= count; ++k) {
        const double latitude = std::asin(std::clamp(bands.boundary[k], -1.0, 1.0)) * Degrees;
        const double belt = beltLatitude(latitude, shift);
        const double drive = mix(strength[1], strength[0], smoothstep(-10.0, 10.0, belt));
        const BeltWind wind = beltWind(belt);
        season.eastward[k] = SphereJetDrive * drive * wind.east * polarShare(bands.boundary[k]);
        season.northward[k] = wind.poleward * (belt >= 0 ? 1.0 : -1.0);
    }
    zonalWindVorticity(bands, season.eastward, season.jet);

    season.overturning.resize(count);
    season.growth.resize(count);
    season.shear.resize(count);
    const double width = bands.width * Degrees;
    for (size_t b = 0; b < count; ++b) {
        const double south = bands.rim[b] * season.northward[b];
        const double north = bands.rim[b + 1] * season.northward[b + 1];
        const double spreading = (north - south) / bands.area[b];
        season.overturning[b] = -MoistDepth * spreading * LiftTime;

        // Centred; past a pole the band mirrors itself, as the flat world's rows wrap there.
        const size_t below = b > 0 ? b - 1 : b, above = b + 1 < count ? b + 1 : b;
        const double northwardGradient = (temperature[above] - temperature[below]) / (2 * width);
        const double latitude = bands.centre[b] * Degrees;
        season.growth[b] = SphereEddyGrowth * storminess(std::abs(northwardGradient), latitude);
        const double cooling = -(latitude >= 0 ? 1.0 : -1.0) * northwardGradient;
        const double shear = SphereShearPerGradient * cooling * std::sin(2 * std::abs(latitude) / Degrees);
        season.shear[b] = std::clamp(shear, -SphereShearLimit, SphereShearLimit);
    }
    return season;
}

std::vector<double> bandProfile(const LatitudeBands& bands, const std::vector<double>& latitude, const std::vector<double>& value) {
    const size_t n = latitude.size();
    if (value.size() != n || n < 2) throw std::invalid_argument("a profile is not two or more latitudes with a value each");
    for (size_t k = 0; k < n; ++k)
        if (!std::isfinite(latitude[k]) || !std::isfinite(value[k])) throw std::invalid_argument("a profile's point is not finite");
    const bool rising = latitude.back() > latitude.front();
    for (size_t k = 0; k + 1 < n; ++k)
        if (!(rising ? latitude[k + 1] > latitude[k] : latitude[k + 1] < latitude[k]))
            throw std::invalid_argument("a profile's latitudes are not strictly monotone");
    // The profile's points from south to north, whichever way it runs.
    const auto at = [&](size_t k) { return rising ? k : n - 1 - k; };
    std::vector<double> profile(size_t(bands.count));
    for (int b = 0; b < bands.count; ++b) {
        const double place = bands.centre[size_t(b)] * Degrees;
        size_t k = 0;
        while (k + 2 < n && latitude[at(k + 1)] <= place) ++k;
        const double south = latitude[at(k)], north = latitude[at(k + 1)];
        const double t = std::clamp((place - south) / (north - south), 0.0, 1.0);
        profile[size_t(b)] = mix(value[at(k)], value[at(k + 1)], t);
    }
    return profile;
}

} // namespace derivean::sim
