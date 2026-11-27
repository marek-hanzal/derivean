// Bands of latitude on the sphere (LatitudeBands, S4.2): their areas and means keep the area
// integral, a solid rotation's wind gives its vorticity by Stokes up to the poles, and a wave that
// is not zonal leaks into the band means only as much as measured here (Codex DREV-CLAUDE-007-01
// point 2).
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/EddyBand.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "core/fn/Hash.hpp"

#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

double latitudeOf(Vec3 centre) { return std::asin(std::clamp(centre.z, -1.0, 1.0)); }

double largest(const std::vector<double>& values) {
    double top = 0;
    for (double v : values) top = std::max(top, std::abs(v));
    return top;
}

// Level 6, a band per two cells along a meridian (side, LatitudeBands): the areas add to the sphere's, a constant's
// mean is the constant, and a band field spread to the cells keeps its integral.
void checkIntegrals() {
    const SphereGrid grid = makeSphereGrid(6);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    double cells = 0, sum = 0;
    for (double a : grid.area) cells += a;
    for (double a : bands.area) sum += a;
    expect(std::abs(sum / cells - 1) < 1e-14, "the bands' areas do not add to the cells'", sum / cells - 1);
    expect(std::abs(sum / (4 * std::numbers::pi * PlanetRadius * PlanetRadius) - 1) < 1e-12, "the bands do not cover the sphere", sum);
    std::vector<double> mean, spread, perBand(size_t(bands.count));
    bandMeans(bands, grid, std::vector<double>(size_t(grid.cells), 3.5), mean);
    for (double m : mean) expect(std::abs(m / 3.5 - 1) < 1e-14, "a constant's band mean is not the constant", m);
    for (int b = 0; b < bands.count; ++b) perBand[size_t(b)] = unit(7, b) - 0.5;
    spreadBands(bands, perBand, spread);
    double onCells = 0, onBands = 0, scale = 0;
    for (int c = 0; c < grid.cells; ++c) onCells += grid.area[size_t(c)] * spread[size_t(c)];
    for (int b = 0; b < bands.count; ++b) onBands += bands.area[size_t(b)] * perBand[size_t(b)], scale += bands.area[size_t(b)] * std::abs(perBand[size_t(b)]);
    expect(std::abs(onCells - onBands) < 1e-14 * scale, "spreading the bands moves their integral", (onCells - onBands) / scale);
}

// A solid rotation u = Ω R cos φ has vorticity 2Ω sin φ. Its wind at the bands' boundaries, through
// Stokes, gives the band means of 2Ω sin φ over the cells to second order, the polar bands as well
// as the others (8.3e-4, 2.1e-4, 5.5e-5 of 2Ω on levels 5 to 7); the bands' vorticities integrate to
// zero, and spread to the cells they keep a zero area mean.
void checkSolidRotation() {
    const double omega = rotationRateAt(PresentAge);
    double previous = 0;
    for (int level = 5; level <= 7; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
        std::vector<double> eastward(size_t(bands.count) + 1), stokes, mean, spread, rotation(size_t(grid.cells));
        for (int b = 0; b <= bands.count; ++b) {
            const double sine = bands.boundary[size_t(b)];
            eastward[size_t(b)] = omega * PlanetRadius * std::sqrt(1 - sine * sine);
        }
        zonalWindVorticity(bands, eastward, stokes);
        for (int c = 0; c < grid.cells; ++c) rotation[size_t(c)] = 2 * omega * grid.centre[size_t(c)].z;
        bandMeans(bands, grid, rotation, mean);
        double worst = 0, net = 0, gross = 0;
        for (int b = 0; b < bands.count; ++b) {
            worst = std::max(worst, std::abs(stokes[size_t(b)] - mean[size_t(b)]) / (2 * omega));
            net += bands.area[size_t(b)] * stokes[size_t(b)], gross += bands.area[size_t(b)] * std::abs(stokes[size_t(b)]);
        }
        expect(worst < 1e-3 / std::pow(4.0, level - 5), "Stokes on the bands is off the solid rotation's vorticity (share of 2Ω)", worst);
        if (previous > 0) expect(previous / worst > 3.5, "Stokes on the bands is not second order", previous / worst);
        previous = worst;
        expect(std::abs(net) < 1e-15 * gross, "the bands' vorticity does not telescope to zero", net / gross);
        spreadBands(bands, stokes, spread);
        double onCells = 0;
        for (int c = 0; c < grid.cells; ++c) onCells += grid.area[size_t(c)] * spread[size_t(c)];
        expect(std::abs(onCells) < 1e-15 * gross, "the spread band vorticity has an area mean", onCells / gross);
    }
}

// The zonal means of waves that are not zonal: m = 1 cancels by the cube's symmetry, m = 4, which the
// cube's quarter turns do not cancel, leaks as measured (9.6e-4 at most here, 1.2e-3 at level 4), at
// three orientations about the axis and three levels. The bands of one cell (2 · side) leak 1.4e-2:
// the cells of an equatorial face's row change latitude along it in step with the bands.
void checkZonalLeak() {
    for (int level = 5; level <= 7; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
        std::vector<double> one(size_t(grid.cells)), four(size_t(grid.cells)), mean;
        for (double turn : {0.0, 0.1, std::numbers::pi / 8}) {
            for (int c = 0; c < grid.cells; ++c) {
                const Vec3 d = grid.centre[size_t(c)];
                const double longitude = std::atan2(d.y, d.x) + turn, across = std::sqrt(d.x * d.x + d.y * d.y);
                one[size_t(c)] = across * std::cos(longitude);
                four[size_t(c)] = std::pow(across, 4) * std::cos(4 * longitude);
            }
            bandMeans(bands, grid, one, mean);
            const double leakOne = largest(mean);
            bandMeans(bands, grid, four, mean);
            const double leakFour = largest(mean);
            expect(leakOne < 1e-14, "m = 1 leaks into the band means", leakOne);
            expect(leakFour < 1.2e-3, "m = 4 leaks into the band means more than measured", leakFour);
        }
    }
}

// The eddy band's output has no zonal part (m ≠ 0) but for the quadrature: its band means, and those
// after a gain in latitude as the growth applies (a storm track at 45° N), against its RMS, for
// today's 2 397 km and Earth's 4 000 km, at level 6: 2.5e-3 and 2.0e-3 of the RMS, 1.5e-3 and 2.3e-3
// after the gain.
void checkEddyLeak() {
    const SphereGrid grid = makeSphereGrid(6);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    std::vector<double> noise(size_t(grid.cells)), eddy, gained(size_t(grid.cells)), mean;
    for (int c = 0; c < grid.cells; ++c) noise[size_t(c)] = unit(11, c) - 0.5;
    for (double wavelength : {SphereEddyWavelength, 4.0e6}) {
        EddyBand band = makeEddyBand(grid, wavelength, SphereEddyReach);
        filterEddyBand(band, noise, eddy);
        for (int c = 0; c < grid.cells; ++c) {
            const double offset = (latitudeOf(grid.centre[size_t(c)]) - std::numbers::pi / 4) / (std::numbers::pi / 12);
            gained[size_t(c)] = std::exp(-offset * offset) * eddy[size_t(c)];
        }
        for (const std::vector<double>* field : {&eddy, &gained}) {
            double square = 0, area = 0;
            for (int c = 0; c < grid.cells; ++c) square += grid.area[size_t(c)] * (*field)[size_t(c)] * (*field)[size_t(c)], area += grid.area[size_t(c)];
            bandMeans(bands, grid, *field, mean);
            const double leak = largest(mean) / std::sqrt(square / area);
            expect(leak < 3e-3, "the eddy band leaks into the band means more than measured", leak);
        }
    }
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkLatitudeBands() {
    checkIntegrals();
    checkSolidRotation();
    checkZonalLeak();
    checkEddyLeak();
}
