#include "MeasureSphere.hpp"

#include "atmosphere/fn/EddyBand.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>

using namespace derivean::sim;

namespace {

constexpr double Degrees = 180 / std::numbers::pi;

// The strongest westerly of a half (sign +1 north, −1 south) and its latitude, and the strongest
// easterly within 30° of the equator (the trades), from the zonal wind per boundary. The halves go by
// the boundary's index, not its sine: the equator's boundary (the middle one of an even count), whose
// sine is a rounding error either side of zero (−0.0000° on the Mac, above zero on Windows), belongs
// to neither, so both read the same boundaries.
struct HalfJets {
    double westerly{}, latitude{}, trades{};
};

HalfJets jetsOf(const LatitudeBands& bands, const std::vector<double>& eastward, int sign) {
    HalfJets half{-1e30, 0, 0};
    const size_t count = eastward.size() - 1;
    for (size_t k = 1; k < count; ++k) {
        if (sign > 0 ? 2 * k <= count : 2 * k >= count) continue;
        const double latitude = std::asin(bands.boundary[k]) * Degrees;
        if (eastward[k] > half.westerly) half.westerly = eastward[k], half.latitude = latitude;
        if (std::abs(latitude) <= 30) half.trades = std::min(half.trades, eastward[k]);
    }
    return half;
}

} // namespace

// The integrals per unit area: energy −½ ψ ζ, enstrophy ½ ζ², absolute enstrophy ½ (ζ + f)², the
// area mean. The zonal wind per boundary by Stokes, rim_k u_k = −Σ_{b<k} B_b ζ̄_b (the inverse of
// zonalWindVorticity). The eddies: the probe's band of ζ, its rms and energy −½ ψ_e ζ_e with Δψ_e =
// ζ_e. The largest normal wind |flux| / length. Per place (interior, face edge, cube corner) ζ's rms
// and its grid noise, ζ less the mean of its four neighbours, rms and largest: the seams' check.
void printSphereDay(SphereProbe& probe, const SphereGrid& grid, const SphereFlow& flow, double day) {
    double energy = 0, enstrophy = 0, absolute = 0, mean = 0, area = 0, top = 0;
    double rms[3] = {}, noise[3] = {}, count[3] = {}, worst[3] = {};
    for (int c = 0; c < grid.cells; ++c) {
        const double a = grid.area[size_t(c)], z = flow.vorticity[size_t(c)];
        energy += -0.5 * a * flow.stream[size_t(c)] * z;
        enstrophy += 0.5 * a * z * z;
        const double q = z + flow.coriolis[size_t(c)];
        absolute += 0.5 * a * q * q;
        mean += a * z, area += a;
        top = std::max(top, std::abs(z));
        double around = 0;
        for (int k = 0; k < 4; ++k) around += flow.vorticity[size_t(neighbourOf(grid, c, k))];
        const double high = z - 0.25 * around;
        const int p = int(placeOf(grid, c));
        rms[p] += z * z, noise[p] += high * high, count[p] += 1, worst[p] = std::max(worst[p], std::abs(high));
    }

    bandMeans(probe.bands, grid, flow.vorticity, probe.mean);
    std::vector<double> eastward(probe.mean.size() + 1, 0.0);
    double south = 0;
    for (size_t k = 1; k < probe.mean.size(); ++k) {
        south += probe.bands.area[k - 1] * probe.mean[k - 1];
        eastward[k] = -south / probe.bands.rim[k];
    }
    const HalfJets north = jetsOf(probe.bands, eastward, 1), southern = jetsOf(probe.bands, eastward, -1);

    filterEddyBand(probe.band, flow.vorticity, probe.eddy);
    probe.stream.assign(size_t(grid.cells), 0.0);
    const PoissonReport report = solveSpherePoisson(probe.poisson, grid, probe.eddy, probe.stream, 1e-10, 400);
    double eddySquare = 0, eddyEnergy = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double a = grid.area[size_t(c)], e = probe.eddy[size_t(c)];
        eddySquare += a * e * e;
        eddyEnergy += -0.5 * a * probe.stream[size_t(c)] * e;
    }

    double wind = 0;
    for (size_t e = 0; e < flow.flux.size(); ++e) wind = std::max(wind, std::abs(flow.flux[e]) / grid.edge[e].length);

    std::printf("day %6.2f  E %.10e  Z %.10e  Zabs %.10e  mean %+.2e  max|ζ| %.4e  wind max %.2f  jet N %.2f at %.1f S %.2f at %.1f"
                "  trades N %.2f S %.2f  eddy rms %.4e E %.6e%s",
                day, energy / area, enstrophy / area, absolute / area, mean / area, top, wind, north.westerly, north.latitude, southern.westerly,
                southern.latitude, north.trades, southern.trades, std::sqrt(eddySquare / area), eddyEnergy / area, report.converged ? "" : " (eddy ψ unconverged)");
    const char* names[3] = {"in", "edge", "corner"};
    for (int p = 0; p < 3; ++p)
        std::printf("  %s ζ %.3e noise %.3e/%.3e", names[p], std::sqrt(rms[p] / count[p]), std::sqrt(noise[p] / count[p]), worst[p]);
    std::printf("\n");
}

namespace {

constexpr std::uint64_t MostValues = std::uint64_t(1) << 28;   // 2 GiB of doubles, past level 10's edges

bool put(std::FILE* file, const std::vector<double>& values) {
    const std::uint64_t n = values.size();
    return std::fwrite(&n, sizeof n, 1, file) == 1 && std::fwrite(values.data(), sizeof(double), values.size(), file) == values.size();
}

bool get(std::FILE* file, std::vector<double>& values) {
    std::uint64_t n = 0;
    if (std::fread(&n, sizeof n, 1, file) != 1 || n == 0 || n > MostValues) return false;
    values.resize(size_t(n));
    return std::fread(values.data(), sizeof(double), values.size(), file) == values.size();
}

} // namespace

void writeSphereSnapshot(const std::string& path, const SphereFlow& flow) {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        std::fprintf(stderr, "cannot write %s\n", path.c_str());
        return;
    }
    const bool written = put(file, flow.vorticity) && put(file, flow.flux);
    if (std::fclose(file) != 0 || !written) std::fprintf(stderr, "cannot write %s\n", path.c_str());
}

int compareSphereSnapshots(const std::string& a, const std::string& b) {
    std::FILE* first = std::fopen(a.c_str(), "rb");
    std::FILE* second = std::fopen(b.c_str(), "rb");
    std::vector<double> x, y;
    int status = first && second ? 0 : 2;
    for (const char* name : {"ζ", "flux"}) {
        if (status || !get(first, x) || !get(second, y) || x.size() != y.size()) {
            status = 2;
            break;
        }
        double difference = 0, size = 0, largest = 0, peak = 0;
        for (size_t i = 0; i < x.size(); ++i) {
            const double d = x[i] - y[i];
            difference += d * d, size += x[i] * x[i];
            largest = std::max(largest, std::abs(d)), peak = std::max(peak, std::abs(x[i]));
        }
        if (peak == 0) {
            status = 2;
            break;
        }
        std::printf("%s: relative L2 %.3e  largest %.3e of the first's largest  ", name, std::sqrt(difference / size), largest / peak);
    }
    std::printf("\n");
    if (first) std::fclose(first);
    if (second) std::fclose(second);
    if (status) std::fprintf(stderr, "cannot compare %s with %s\n", a.c_str(), b.c_str());
    return status;
}
