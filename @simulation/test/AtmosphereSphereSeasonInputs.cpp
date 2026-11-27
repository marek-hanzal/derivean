// The season's inputs (SphereSeason, S4.3b): a profile read onto the bands, and the fields, bands
// and base a season refuses, in release builds too.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereSeason.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Degrees = 180.0 / std::numbers::pi;

// Air that cools by half a degree per degree of latitude from 30 °C at the equator.
std::vector<double> cooling(const LatitudeBands& bands) {
    std::vector<double> t(size_t(bands.count));
    for (int b = 0; b < bands.count; ++b) t[size_t(b)] = 30 - 0.5 * std::abs(bands.centre[size_t(b)] * Degrees);
    return t;
}

bool refused(const auto& call) {
    try {
        (void)call();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

// A profile reads linearly onto the bands, from rows running south to north or north to south, and
// holds its ends past them.
void checkProfile() {
    const SphereGrid grid = makeSphereGrid(4);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    std::vector<double> rows, values;
    for (double latitude = 80; latitude >= -80; latitude -= 2.5) rows.push_back(latitude), values.push_back(3 * latitude + 7);
    std::vector<double> south(rows.rbegin(), rows.rend()), southValues(values.rbegin(), values.rend());
    const std::vector<double> down = bandProfile(bands, rows, values), up = bandProfile(bands, south, southValues);
    for (int b = 0; b < bands.count; ++b) {
        const double latitude = std::clamp(bands.centre[size_t(b)] * Degrees, -80.0, 80.0);
        const double exact = 3 * latitude + 7;
        expect(std::abs(down[size_t(b)] - exact) < 1e-12 && std::abs(up[size_t(b)] - exact) < 1e-12, "a profile did not read onto the bands",
               down[size_t(b)] - exact);
    }
    // The flat world's rows run 90 → −90 → 90: refused whole, taken as one half.
    std::vector<double> wrapped = rows, wrappedValues = values;
    wrapped.insert(wrapped.end(), south.begin() + 1, south.end());
    wrappedValues.insert(wrappedValues.end(), southValues.begin() + 1, southValues.end());
    expect(refused([&] { return bandProfile(bands, wrapped, wrappedValues); }), "a profile that is not monotone was read");
    expect(refused([&] { return bandProfile(bands, {10.0}, {1.0}); }) && refused([&] { return bandProfile(bands, rows, {1.0, 2.0}); }) &&
               refused([&] { return bandProfile(bands, {0.0, 10.0}, {1.0, std::numeric_limits<double>::infinity()}); }) &&
               refused([&] { return bandProfile(bands, {-std::numeric_limits<double>::infinity(), 0.0, 90.0}, {1.0, 2.0, 3.0}); }),
           "a profile without two points and a finite value each was read");
}

// A season needs one finite value per band in both fields, a band in each window of each half
// (seven bands have none in [55°, 75°): centres 0°, ±25.7°, ±51.4°, ±77.1°), a base whose subtropics
// are warmer than its subpolar bands (not uniform, not reversed) and a finite day; it is refused
// otherwise.
void checkRefusals() {
    const SphereGrid grid = makeSphereGrid(4);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side), few = makeLatitudeBands(grid, 7);
    const std::vector<double> base = cooling(bands);
    std::vector<double> nan = base, flat(base.size(), 15.0);
    nan[3] = std::numeric_limits<double>::quiet_NaN();
    const std::vector<double> longer(base.size() + 1, 15.0);
    expect(refused([&] { return sphereSeasonOf(few, 100, cooling(few), cooling(few)); }), "a season without subpolar bands was taken");
    expect(refused([&] { return sphereSeasonOf(bands, 100, longer, base); }) && refused([&] { return sphereSeasonOf(bands, 100, base, {}); }),
           "a field not one value per band was taken");
    expect(refused([&] { return sphereSeasonOf(bands, 100, nan, base); }) && refused([&] { return sphereSeasonOf(bands, 100, base, nan); }),
           "a temperature not finite was taken");
    std::vector<double> reversed = base;
    for (double& v : reversed) v = -v;
    expect(refused([&] { return sphereSeasonOf(bands, 100, base, flat); }) && refused([&] { return sphereSeasonOf(bands, 100, base, reversed); }),
           "a base without a positive contrast was taken");
    expect(refused([&] { return sphereSeasonOf(bands, std::numeric_limits<double>::quiet_NaN(), base, base); }), "a day not finite was taken");
    expect(!refused([&] { return sphereSeasonOf(bands, 100, flat, base); }), "a uniform temperature over a base was refused");
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkSeasonInputs() {
    checkProfile();
    checkRefusals();
}
