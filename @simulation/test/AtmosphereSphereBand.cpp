// The eddy band on the sphere, the replacement of the flat world's PoissonSolver::pass: its degrees
// and gains, and its response to single harmonics inside, at the edges of and outside the band and to
// the zonal ones it leaves to the belts.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/EddyBand.hpp"

#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

// The gain is the flat world's ring, max(0, 1 − ((k − centre) / reach)²), at k = √(l(l + 1)) / R, and
// the band's bounds are the degrees with a positive gain, for today's constants (which are the flat
// world's 7 ± 3.5 cycles across PlanetSpan) and for any other wavelength S4 may choose.
void checkDegrees() {
    const SphereGrid grid = makeSphereGrid(3);
    for (double wavelength : {SphereEddyWavelength, 4.0e6}) {
        const EddyBand band = makeEddyBand(grid, wavelength, SphereEddyReach);
        const double centre = 2 * std::numbers::pi / wavelength, reach = SphereEddyReach * centre;
        for (int l = 0; l <= 40; ++l) {
            const double offset = (std::sqrt(double(l) * (l + 1)) / PlanetRadius - centre) / reach;
            const double gain = std::max(0.0, 1 - offset * offset);
            expect(std::abs(eddyGain(band, l) - gain) < 1e-12, "gain is not the flat world's ring", l);
            expect((gain > 0) == (l >= band.lowest && l <= band.highest), "band bounds are not its positive gains", l);
        }
    }
    const double cycles = PlanetSpan / SphereEddyWavelength;
    expect(std::abs(cycles - 7) < 1e-12, "the band is not the flat world's 7 cycles", cycles);
}

struct Response {
    double inside{};    // worst |filtered − gain · Y| over modes of the band
    double outside{};   // worst |filtered| over zonal modes and modes outside the band
};

Response responseAt(int level) {
    const SphereGrid grid = makeSphereGrid(level);
    EddyBand band = makeEddyBand(grid, SphereEddyWavelength, SphereEddyReach);
    // Order m > 0 multiplies by cos mλ, m < 0 by sin |m|λ.
    const int inBand[][2] = {{16, 5}, {16, -9}, {8, 3}, {12, -12}, {24, 24}, {20, 1}};
    const int outBand[][2] = {{12, 0}, {20, 0}, {4, 2}, {1, 1}, {7, -7}, {25, 3}, {30, 7}, {40, -20}};
    Response response;
    std::vector<double> result;
    for (const auto& mode : inBand) {
        const std::vector<double> y = harmonicField(grid, mode[0], mode[1]);
        filterEddyBand(band, y, result);
        const double gain = eddyGain(band, mode[0]);
        for (int c = 0; c < grid.cells; ++c) response.inside = std::max(response.inside, std::abs(result[size_t(c)] - gain * y[size_t(c)]));
    }
    for (const auto& mode : outBand) {
        filterEddyBand(band, harmonicField(grid, mode[0], mode[1]), result);
        for (double value : result) response.outside = std::max(response.outside, std::abs(value));
    }
    filterEddyBand(band, std::vector<double>(size_t(grid.cells), 1.0), result);
    for (double value : result) response.outside = std::max(response.outside, std::abs(value));
    return response;
}

// The coefficients come from the grid's own area quadrature, so harmonics are reproduced to its
// second-order error: harmonics of unit mean square come back within 9.1e-3 at level 5 and 2.2e-3 at
// level 6, and modes outside the band leak as much, both falling fourfold per level.
void checkResponse() {
    const Response coarse = responseAt(5), fine = responseAt(6);
    expect(coarse.inside / fine.inside > 3.5 && coarse.outside / fine.outside > 3.5, "band response is not second order",
           std::min(coarse.inside / fine.inside, coarse.outside / fine.outside));
    expect(fine.inside < 2.5e-3 && fine.outside < 2.5e-3, "band response is off at level 6", std::max(fine.inside, fine.outside));
}

} // namespace

namespace derivean::sim::atmosphere_sphere_test {

void checkEddyBand() {
    checkDegrees();
    checkResponse();
}

} // namespace derivean::sim::atmosphere_sphere_test
