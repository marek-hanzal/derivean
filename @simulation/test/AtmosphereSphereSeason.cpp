// The season of the sphere's circulation by bands (SphereSeason, S4.3b): the belts' wind and its
// jet by Stokes, the cells' surface branch and the lifting of its convergence, the eddies' growth
// and the thermal wind's shear from the bands' temperature (its inputs: AtmosphereSphereSeasonInputs).
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereSeason.hpp"
#include "chronology/type/Calendar.hpp"
#include "climate/constant/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "core/type/Vec.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Degrees = 180.0 / std::numbers::pi;

std::vector<double> latitudes(const LatitudeBands& bands) {
    std::vector<double> at(size_t(bands.count));
    for (int b = 0; b < bands.count; ++b) at[size_t(b)] = bands.centre[size_t(b)] * Degrees;
    return at;
}

// Air that cools by half a degree per degree of latitude from 30 °C at the equator.
std::vector<double> cooling(const LatitudeBands& bands) {
    std::vector<double> t = latitudes(bands);
    for (double& v : t) v = 30 - 0.5 * std::abs(v);
    return t;
}

// Each half's contrast as the header states it, north then south: the area-weighted mean over the
// bands centred in [15°, 35°) less that over [55°, 75°).
std::array<double, 2> contrastOf(const LatitudeBands& bands, const std::vector<double>& field) {
    double sum[2][2]{}, area[2][2]{};
    for (int b = 0; b < bands.count; ++b) {
        const double latitude = bands.centre[size_t(b)] * Degrees, band = std::abs(latitude);
        const int side = band >= 15 && band < 35 ? 0 : band >= 55 && band < 75 ? 1 : -1;
        if (side < 0) continue;
        const int half = latitude >= 0 ? 0 : 1;
        const double content = bands.area[size_t(b)] * field[size_t(b)];
        sum[half][side] += content;
        area[half][side] += bands.area[size_t(b)];
    }
    return {sum[0][0] / area[0][0] - sum[0][1] / area[0][1], sum[1][0] / area[1][0] - sum[1][1] / area[1][1]};
}

// With the temperature at its annual mean the belts blow JetDrive times beltWind at the shifted
// latitude, poleward of PolarCap times the header's share (the caps: AtmosphereSphereSeasonPoles);
// the jet is their vorticity by Stokes: round every boundary the bands south of it hold
// −rim · u (to 1e-12), and it has no area integral (balancedJet). The surface branch is the belts'
// poleward wind, by the belts' half (days 100 and 220, the belts either side of the equator); the
// cells' lifting has no area integral either.
void checkBelts() {
    const SphereGrid grid = makeSphereGrid(5);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    const std::vector<double> base = cooling(bands);
    int between = 0;   // boundaries between the equator and the belts' equator, where the two signs differ
    for (const double day : {100.0, 220.0}) {
        const SphereSeason season = sphereSeasonOf(bands, day, base, base);
        const double shift = SphereBeltShift * declinationOn(day - SphereBeltLag);
        double worst = 0, held = 0, scale = 0, lifted = 0, lift = 0;
        for (size_t k = 0; k <= size_t(bands.count); ++k) {
            const double latitude = std::asin(bands.boundary[k]) * Degrees;
            const double belt = latitude - shift * std::cos(latitude / Degrees);
            between += (belt >= 0) != (latitude >= 0) && std::abs(beltWind(belt).poleward) > 1e-3;
            const double kept = polarShareOf(bands.boundary[k]);
            worst = std::max(worst, std::abs(season.eastward[k] - SphereJetDrive * beltWind(belt).east * kept));
            worst = std::max(worst, std::abs(season.northward[k] - beltWind(belt).poleward * (belt >= 0 ? 1.0 : -1.0)));
            const double circulation = bands.rim[k] * season.eastward[k];
            held = k == 0 ? 0 : held + bands.area[k - 1] * season.jet[k - 1];
            scale = std::max(scale, std::abs(circulation));
            expect(std::abs(held + circulation) <= 1e-12 * scale, "the jet is not the belts' wind by Stokes", double(k));
        }
        expect(worst <= 1e-12, "the belts do not blow JetDrive times beltWind", worst);
        SphereClosures closures;
        closures.bands = bands;
        closures.jet = season.jet;
        expect(balancedJet(closures), "the season's jet has an area integral");
        for (int b = 0; b < bands.count; ++b) {
            const double content = bands.area[size_t(b)] * season.overturning[size_t(b)];
            lifted += content;
            lift += std::abs(content);
        }
        expect(lift > 0 && std::abs(lifted) <= 1e-12 * lift, "the cells' lifting has an area integral", lifted / lift);
    }
    expect(between > 0, "no boundary lay between the equator and the belts' equator");
}

// Each half blows as hard as its contrast over the base's, clamped to 0.5 to 1.8, and the halves
// blend across ±10° of the belts' equator: on a base not linear in |φ|, the north's contrast 0.2, 1.5
// and 3 times the base's (both clamps), and one northern band at a time warmed 2 °C from 3° to 87°,
// inside, beside and outside the windows (so the windows' edges, to the bands' spacing, and the area
// weights are held), every boundary blows what the header's contrasts give, to 1e-12.
void checkStrength() {
    const SphereGrid grid = makeSphereGrid(5);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    const std::vector<double> lat = latitudes(bands);
    std::vector<double> base(lat.size());
    for (size_t b = 0; b < lat.size(); ++b) base[b] = 30 - 0.008 * lat[b] * lat[b];
    const double day = 100, shift = SphereBeltShift * declinationOn(day - SphereBeltLag);
    const SphereSeason plain = sphereSeasonOf(bands, day, base, base);
    const std::array<double, 2> reference = contrastOf(bands, base);
    std::vector<std::vector<double>> cases;
    for (double factor : {0.2, 1.5, 3.0}) {
        cases.push_back(base);
        for (size_t b = 0; b < lat.size(); ++b)
            if (lat[b] > 0) cases.back()[b] *= factor;
    }
    for (size_t b = 0; b < lat.size(); ++b)
        if (lat[b] > 0) cases.push_back(base), cases.back()[b] += 2;
    int blended = 0;
    for (const std::vector<double>& warm : cases) {
        const SphereSeason season = sphereSeasonOf(bands, day, warm, base);
        const std::array<double, 2> contrast = contrastOf(bands, warm);
        const double north = std::clamp(contrast[0] / reference[0], 0.5, 1.8), south = std::clamp(contrast[1] / reference[1], 0.5, 1.8);
        for (size_t k = 0; k <= size_t(bands.count); ++k) {
            if (std::abs(plain.eastward[k]) < 0.1) continue;
            const double latitude = std::asin(bands.boundary[k]) * Degrees;
            const double belt = latitude - shift * std::cos(latitude / Degrees);
            if (std::abs(belt) < 10 && north != south) ++blended;
            const double expected = mix(south, north, smoothstep(-10.0, 10.0, belt));
            expect(std::abs(season.eastward[k] / plain.eastward[k] - expected) < 1e-12, "a half's contrast did not set its belts", latitude);
        }
    }
    expect(blended > 0, "no boundary blended the halves");
}

// Air cooling poleward in both halves shears westerly everywhere, by 12 m/s · 0.5 · sin 2|φ| (6 m/s
// at 45°); the eddies grow at EddyGrowth · storminess(0.5 °C per degree) there. A polar band takes
// itself for the band past the pole, so its gradient is half: storminess(0.25).
void checkThermalWind() {
    const SphereGrid grid = makeSphereGrid(5);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    const std::vector<double> base = cooling(bands);
    const SphereSeason season = sphereSeasonOf(bands, 100, base, base);
    for (int b = 1; b + 1 < bands.count; ++b) {
        const double latitude = bands.centre[size_t(b)] * Degrees;
        if (bands.centre[size_t(b) - 1] * bands.centre[size_t(b) + 1] <= 0) continue;   // the gradient spans the kink of |φ|
        const double shear = SphereShearPerGradient * 0.5 * std::sin(2 * std::abs(latitude) / Degrees);
        expect(std::abs(season.shear[size_t(b)] - shear) < 1e-12, "the thermal wind's shear moved", season.shear[size_t(b)]);
        const double growth = SphereEddyGrowth * storminess(0.5, latitude);
        expect(std::abs(season.growth[size_t(b)] - growth) <= 1e-12 * growth, "the eddies' growth moved", season.growth[size_t(b)]);
    }
    for (int b : {0, bands.count - 1}) {
        const double growth = SphereEddyGrowth * storminess(0.25, bands.centre[size_t(b)] * Degrees);
        expect(std::abs(season.growth[size_t(b)] - growth) <= 1e-12 * growth, "a polar band's growth moved", season.growth[size_t(b)]);
    }
    // Four times as steep, the thermal wind reaches ShearLimit about 15° from the equator.
    std::vector<double> steep = latitudes(bands);
    for (double& v : steep) v = 30 - 2 * std::abs(v);
    const SphereSeason sheared = sphereSeasonOf(bands, 100, steep, base);
    int clamped = 0;
    for (int b = 1; b + 1 < bands.count; ++b) {
        if (bands.centre[size_t(b) - 1] * bands.centre[size_t(b) + 1] <= 0) continue;
        const double shear = SphereShearPerGradient * 2 * std::sin(2 * std::abs(bands.centre[size_t(b)]));
        clamped += shear > SphereShearLimit;
        expect(std::abs(sheared.shear[size_t(b)] - std::min(shear, SphereShearLimit)) < 1e-12, "the shear is not held to its limit", shear);
    }
    expect(clamped > 0, "no band reached the shear's limit");
}

// The cells lift where the trades meet, at the belts' equator (within 5° of it), and sink in the
// subtropics (25° to 35° from it), in both halves; each band lifts MoistDepth · LiftTime times the
// convergence of the surface branch through its rims.
void checkCells() {
    const SphereGrid grid = makeSphereGrid(5);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    const std::vector<double> base = cooling(bands);
    const double day = 100;
    const SphereSeason season = sphereSeasonOf(bands, day, base, base);
    const double shift = SphereBeltShift * declinationOn(day - SphereBeltLag);
    int lifted = 0, sunk = 0;
    double top = 0;
    for (double v : season.overturning) top = std::max(top, std::abs(v));
    for (int b = 0; b < bands.count; ++b) {
        const size_t i = size_t(b);
        const double out = bands.rim[i + 1] * season.northward[i + 1] - bands.rim[i] * season.northward[i];
        const double lift = -MoistDepth * LiftTime * out / bands.area[i];
        expect(std::abs(season.overturning[i] - lift) <= 1e-12 * top, "a band's lifting is not its convergence", season.overturning[i]);
        const double away = std::abs(bands.centre[size_t(b)] * Degrees - shift);
        if (away < 5) expect(season.overturning[size_t(b)] > 0, "the trades do not lift where they meet", away), ++lifted;
        if (away > 25 && away < 35) expect(season.overturning[size_t(b)] < 0, "the subtropics do not sink", away), ++sunk;
    }
    expect(lifted >= 2 && sunk >= 4, "the cells' test reached too few bands", lifted);
}

// The belts follow the sun a month late: the westerlies' equatorward edge in the north (where the
// belts' wind turns from easterly to westerly) lies further north in August than in February (39°
// against 28° at level 5).
void checkSeasonShift() {
    const SphereGrid grid = makeSphereGrid(5);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    const std::vector<double> base = cooling(bands);
    const auto edge = [&](double day) {
        const SphereSeason season = sphereSeasonOf(bands, day, base, base);
        for (size_t k = 1; k <= size_t(bands.count); ++k) {
            const double latitude = std::asin(bands.boundary[k]) * Degrees;
            if (latitude > 10 && season.eastward[k - 1] < 0 && season.eastward[k] >= 0) return latitude;
        }
        return 0.0;
    };
    const double winter = edge(40), summer = edge(220);
    expect(winter > 20 && summer > winter + 8, "the belts do not follow the sun", summer - winter);
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkSeason() {
    checkBelts();
    checkStrength();
    checkThermalWind();
    checkSeasonShift();
    checkCells();
}
