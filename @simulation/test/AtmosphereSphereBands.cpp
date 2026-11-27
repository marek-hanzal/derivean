// Bands of latitude on the sphere (LatitudeBands, S4.2): their areas and means keep the area
// integral, the winds of a solid rotation and of a pair of jets give their vorticity by Stokes up to
// the poles, and a wave that is not zonal leaks into the band means only as much as measured here
// (Codex DREV-CLAUDE-007-01 point 2).
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

// Level 6, a band per two cells along a meridian (side, LatitudeBands): the areas add to the
// sphere's, a constant's mean is the constant, and a band field spread to the cells keeps its
// integral.
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
    for (int b = 0; b < bands.count; ++b) {
        onBands += bands.area[size_t(b)] * perBand[size_t(b)];
        scale += bands.area[size_t(b)] * std::abs(perBand[size_t(b)]);
    }
    expect(std::abs(onCells - onBands) < 1e-14 * scale, "spreading the bands moves their integral", (onCells - onBands) / scale);
}

// A zonal wind's band vorticity by Stokes against the band means of its vorticity over the cells, on
// levels 5 to 7. The solid rotation u = Ω R cos φ (vorticity 2Ω sin φ) gives on the bands' circles
// Ω (sin φ_south + sin φ_north), its analytic circulation over B_b, to 2e-12 of 2Ω (2.3e-14, 1.2e-13,
// 5.8e-13: √(1 − s²) cancels at the polar circles), which checks Σ A = 4πR² and the rims' rounding;
// against the cells' means it is off at second order (8.3e-4, 2.1e-4, 5.5e-5 of 2Ω). A pair of
// jets, u = U cos φ sin² 2φ, with vorticity (8U / R) sin φ cos² φ (2 sin² φ − cos² φ), is off the
// same way (3.2e-3, 8.2e-4, 2.1e-4 of 8U / R), its worst band the polar one on every level. Each is
// bounded a fifth above its level 5 value, a quarter per level. The bands' vorticities integrate to
// zero, and spread to the cells they keep a zero area mean.
double stokesError(int level, bool jets, double& exact) {
    const double omega = rotationRateAt(PresentAge), jet = 30.0;
    const SphereGrid grid = makeSphereGrid(level);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    std::vector<double> eastward(size_t(bands.count) + 1), stokes, mean, spread, vorticity(size_t(grid.cells));
    for (int b = 0; b <= bands.count; ++b) {
        const double sine = bands.boundary[size_t(b)], cosine = std::sqrt(1 - sine * sine);
        const double twice = 2 * sine * cosine;
        eastward[size_t(b)] = jets ? jet * cosine * twice * twice : omega * PlanetRadius * cosine;
    }
    zonalWindVorticity(bands, eastward, stokes);
    for (int c = 0; c < grid.cells; ++c) {
        const double sine = grid.centre[size_t(c)].z, square = 1 - sine * sine;
        const double shape = sine * square;
        vorticity[size_t(c)] = jets ? 8 * jet / PlanetRadius * shape * (2 * sine * sine - square) : 2 * omega * sine;
    }
    bandMeans(bands, grid, vorticity, mean);
    const double scale = jets ? 8 * jet / PlanetRadius : 2 * omega;
    double worst = 0, net = 0, gross = 0;
    exact = 0;
    for (int b = 0; b < bands.count; ++b) {
        worst = std::max(worst, std::abs(stokes[size_t(b)] - mean[size_t(b)]) / scale);
        const double analytic = omega * (bands.boundary[size_t(b)] + bands.boundary[size_t(b) + 1]);
        if (!jets) exact = std::max(exact, std::abs(stokes[size_t(b)] - analytic) / scale);
        net += bands.area[size_t(b)] * stokes[size_t(b)], gross += bands.area[size_t(b)] * std::abs(stokes[size_t(b)]);
    }
    // The bands' rim · u cancel in pairs exactly; what is left is the subtraction, the division and
    // the running sum, about (count + 3) ε · gross. The cells' sum of the spread, over more terms, is
    // measured at 2.3 % of the same bound.
    const double rounding = bands.count * 2.3e-16 * gross;
    expect(std::abs(net) < rounding, "the bands' vorticity does not telescope to zero", net / gross);
    spreadBands(bands, stokes, spread);
    double onCells = 0;
    for (int c = 0; c < grid.cells; ++c) onCells += grid.area[size_t(c)] * spread[size_t(c)];
    expect(std::abs(onCells) < rounding, "the spread band vorticity has an area mean", onCells / gross);
    return worst;
}

void checkStokes() {
    for (bool jets : {false, true}) {
        double previous = 0;
        for (int level = 5; level <= 7; ++level) {
            double exact = 0;
            const double worst = stokesError(level, jets, exact);
            if (!jets) expect(exact < 2e-12, "Stokes on the bands is not the solid rotation's analytic circulation over B_b", exact);
            expect(worst < (jets ? 3.8e-3 : 1e-3) / std::pow(4.0, level - 5), "Stokes on the bands is off the band means of the vorticity", worst);
            if (previous > 0) expect(previous / worst > 3.5, "Stokes on the bands is not second order", previous / worst);
            previous = worst;
        }
    }
}

// The zonal means of waves that are not zonal, on levels 5 to 7, the waves turned about the
// planet's axis (the cube's z). m = 1 cancels by the cube's symmetry. m = 4, which the cube's quarter
// turns leave, leaks as measured (docs/handoff/atmosphere-sphere.md, bands of latitude): its sin 4λ
// part cancels by the cube's mirror, so a wave turned by α leaks |cos 4α| times the unturned one's,
// none at π/8 and the same at π/4.
void checkZonalLeak() {
    for (int level = 5; level <= 7; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
        std::vector<double> one(size_t(grid.cells)), four(size_t(grid.cells)), mean;
        double unturned = 0;
        for (double turn : {0.0, 0.1, std::numbers::pi / 8, std::numbers::pi / 4}) {
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
            if (turn == 0) unturned = leakFour;
            expect(leakOne < 1e-14, "m = 1 leaks into the band means", leakOne);
            expect(leakFour < 1.2e-3, "m = 4 leaks into the band means more than measured", leakFour);
            const double expected = unturned * std::abs(std::cos(4 * turn));
            expect(std::abs(leakFour - expected) < 1e-14, "m = 4's leak does not follow |cos 4α|", leakFour - expected);
        }
    }
}

// The eddy band's output has no zonal part (m ≠ 0) but for the quadrature: its band means, and those
// after a gain in latitude as the growth applies (a storm track at 45° N), against its RMS, for
// today's 2 397 km and Earth's 4 000 km, at level 6: 2.5e-3 and 2.0e-3 of the RMS, 1.5e-3 and 2.3e-3
// after the gain, each bounded a fifth above.
void checkEddyLeak() {
    const SphereGrid grid = makeSphereGrid(6);
    const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
    std::vector<double> noise(size_t(grid.cells)), eddy, gained(size_t(grid.cells)), mean;
    for (int c = 0; c < grid.cells; ++c) noise[size_t(c)] = unit(11, c) - 0.5;
    constexpr double Bound[2][2] = {{3e-3, 1.8e-3}, {2.4e-3, 2.8e-3}};   // [band][plain, gained]
    int which = 0;
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
            expect(leak < Bound[which][field == &eddy ? 0 : 1], "the eddy band leaks into the band means more than measured", leak);
        }
        ++which;
    }
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkLatitudeBands() {
    checkIntegrals();
    checkStokes();
    checkZonalLeak();
    checkEddyLeak();
}
