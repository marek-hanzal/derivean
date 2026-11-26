#include "atmosphere/fn/Kernels.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"

namespace derivean::sim {
namespace {

constexpr double MeanRelaxation = 4 * DaySeconds;   // s over which the zonal-mean wind returns to its belts
constexpr double SurfaceDrag = 12 * DaySeconds;     // s, friction on all vorticity at sea level
constexpr double RoughHeight = 1500.0;              // m of terrain that doubles the friction
constexpr double TerrainDepth = 10000.0;            // m of air column the terrain squeezes
constexpr double TerrainLimit = 2500.0;             // m of terrain the column still feels
constexpr double GridDamping = 6 * 3600.0;          // s, biharmonic decay time of the shortest wave
constexpr double FilterStrength = 0.1;              // Robert–Asselin–Williams filter of the leapfrog step
constexpr double FilterSplit = 0.53;
constexpr double EddyLimit = 1.5e-5;                // 1/s, root-mean-square vorticity at which weather systems stop growing
constexpr double EddyWave = 7.0;                    // cycles around the planet of the fastest-growing weather system
constexpr double EddyBand = 3.5;                    // cycles on either side that still grow
constexpr int EnvelopeReach = 4;                    // rows over which the strength of the eddies is averaged
constexpr double Disturbance = 4e-6;                // 1/s, seeded vorticity the weather grows from
constexpr int DisturbanceLattice = 16;              // noise cells across the planet
constexpr uint64_t WeatherStream = 0x57545852;

} // namespace

void shapeCirculation(Atmosphere& atmosphere) {
    const int n = AirCells;
    // Waves of weather-system size that vary along X; zonal bands are left to the belts.
    const int waves = n / 2 + 1;
    for (int z = 0; z < n; ++z) {
        for (int x = 1; x < waves; ++x) {
            const double cycles = std::hypot(double(x), double(z > n / 2 ? z - n : z));
            const double offset = (cycles - EddyWave) / EddyBand;
            atmosphere.poisson.pass[size_t(z) * size_t(waves) + size_t(x)] = std::max(0.0, 1.0 - offset * offset) / (double(n) * n);
        }
    }
    for (int z = 0; z < n; ++z) {
        const double planetary = -Coriolis * std::sin(atmosphere.vertexLatitude[size_t(z)] * 3.141592653589793 / 180.0);
        for (int x = 0; x < n; ++x) {
            const double height = 0.25 * (atmosphere.ground.at(x, z) + atmosphere.ground.at(wrapAir(x - 1), z) +
                                          atmosphere.ground.at(x, wrapAir(z - 1)) + atmosphere.ground.at(wrapAir(x - 1), wrapAir(z - 1)));
            const double friction = (1.0 + height / RoughHeight) / SurfaceDrag;
            atmosphere.dragKeep.at(x, z) = 1.0 / (1.0 + 2 * WindStep * friction);
            atmosphere.absolute.at(x, z) = planetary * (1.0 + std::min(height, TerrainLimit) / TerrainDepth);
        }
    }
}

void settleCirculation(Atmosphere& atmosphere) {
    const uint64_t seed = stream(atmosphere.seed, WeatherStream);
    const double scale = double(DisturbanceLattice) / AirCells;
    for (int z = 0; z < AirCells; ++z)
        for (int x = 0; x < AirCells; ++x)
            atmosphere.vorticity.at(x, z) = atmosphere.season.jet[size_t(z)] +
                                            Disturbance * fractalNoise(seed, x * scale, z * scale, 3, 0.5, DisturbanceLattice);
    // Two equal levels make the first leapfrog step a forward step.
    atmosphere.vorticityBefore = atmosphere.vorticity;
    refreshWind(atmosphere, true);
}

// Leapfrog step of the barotropic vorticity equation. Advection uses Arakawa's Jacobian, which
// keeps energy and enstrophy from piling up at the grid scale; friction is implicit; relaxation
// and scale-selective damping act on the older level.
//
// A single layer cannot turn a temperature contrast into motion, which is what feeds real weather
// systems. That conversion is a closure: vorticity of weather-system size grows at the rate the
// simulated temperature gradient of its latitude sets, until the eddies of that latitude reach
// EddyLimit. Everything the eddies then do, they do through the advection above.
void stepCirculation(Atmosphere& atmosphere) {
    const int n = AirCells;
    const double span = 2 * WindStep;
    const double jacobianScale = 1.0 / (12 * AirCell * AirCell);
    const double biharmonic = 1.0 / (64 * GridDamping);   // per cell⁴, with the Laplacians left in cell units
    AirField& potential = atmosphere.work.potential;
    AirField& bend = atmosphere.work.bend;
    AirField& next = atmosphere.work.next;
    const AirField& before = atmosphere.vorticityBefore;

    AirRows mean{};
    for (int z = 0; z < n; ++z) {
        const int north = wrapAir(z - 1), south = wrapAir(z + 1);
        double sum = 0;
        for (int x = 0; x < n; ++x) {
            const int west = wrapAir(x - 1), east = wrapAir(x + 1);
            potential.at(x, z) = atmosphere.vorticity.at(x, z) + atmosphere.absolute.at(x, z);
            bend.at(x, z) = before.at(west, z) + before.at(east, z) + before.at(x, north) + before.at(x, south) - 4 * before.at(x, z);
            sum += before.at(x, z);
        }
        mean[size_t(z)] = sum / n;
    }

    AirRows strength{}, growth{};
    for (int z = 0; z < n; ++z) {
        double squares = 0;
        for (int x = 0; x < n; ++x) squares += atmosphere.eddy.at(x, z) * atmosphere.eddy.at(x, z);
        strength[size_t(z)] = squares / n;
    }
    for (int z = 0; z < n; ++z) {
        double envelope = 0;
        for (int offset = -EnvelopeReach; offset <= EnvelopeReach; ++offset)
            envelope += (EnvelopeReach + 1 - std::abs(offset)) * strength[size_t(wrapAir(z + offset))];
        envelope /= (EnvelopeReach + 1) * (EnvelopeReach + 1);
        growth[size_t(z)] = atmosphere.season.growth[size_t(z)] * std::max(0.0, 1.0 - envelope / (EddyLimit * EddyLimit));
    }

    for (int z = 0; z < n; ++z) {
        const int north = wrapAir(z - 1), south = wrapAir(z + 1);
        const double* p0 = &atmosphere.stream.cells[size_t(north) * n];
        const double* p1 = &atmosphere.stream.cells[size_t(z) * n];
        const double* p2 = &atmosphere.stream.cells[size_t(south) * n];
        const double* q0 = &potential.cells[size_t(north) * n];
        const double* q1 = &potential.cells[size_t(z) * n];
        const double* q2 = &potential.cells[size_t(south) * n];
        const double* b0 = &bend.cells[size_t(north) * n];
        const double* b1 = &bend.cells[size_t(z) * n];
        const double* b2 = &bend.cells[size_t(south) * n];
        const double relaxed = (atmosphere.season.jet[size_t(z)] - mean[size_t(z)]) / MeanRelaxation;
        const double* eddy = &atmosphere.eddy.cells[size_t(z) * n];
        const double feeding = growth[size_t(z)];
        for (int x = 0; x < n; ++x) {
            const int w = wrapAir(x - 1), e = wrapAir(x + 1);
            const double jacobian =
                (p1[e] - p1[w]) * (q2[x] - q0[x]) - (p2[x] - p0[x]) * (q1[e] - q1[w]) +
                p1[e] * (q2[e] - q0[e]) - p1[w] * (q2[w] - q0[w]) - p2[x] * (q2[e] - q2[w]) + p0[x] * (q0[e] - q0[w]) +
                q2[x] * (p2[e] - p2[w]) - q0[x] * (p0[e] - p0[w]) - q1[e] * (p2[e] - p0[e]) + q1[w] * (p2[w] - p0[w]);
            const double damping = biharmonic * (b1[w] + b1[e] + b0[x] + b2[x] - 4 * b1[x]);
            const double tendency = relaxed + feeding * eddy[x] - jacobian * jacobianScale - damping;
            next.at(x, z) = (before.at(x, z) + span * tendency) * atmosphere.dragKeep.at(x, z);
        }
    }

    for (size_t cell = 0; cell < next.size(); ++cell) {
        const double curve = 0.5 * FilterStrength * (before.cells[cell] - 2 * atmosphere.vorticity.cells[cell] + next.cells[cell]);
        atmosphere.vorticityBefore.cells[cell] = atmosphere.vorticity.cells[cell] + FilterSplit * curve;
        atmosphere.vorticity.cells[cell] = next.cells[cell] - (1.0 - FilterSplit) * curve;
    }
}

void refreshWind(Atmosphere& atmosphere, bool eddies) {
    const int n = AirCells;
    solvePoisson(atmosphere.poisson, atmosphere.vorticity, atmosphere.stream, eddies ? &atmosphere.eddy : nullptr);
    const double perCell = 1.0 / AirCell;
    for (int z = 0; z < n; ++z) {
        const int south = wrapAir(z + 1);
        for (int x = 0; x < n; ++x) {
            const double here = atmosphere.stream.at(x, z);
            atmosphere.faceX.at(x, z) = -(atmosphere.stream.at(x, south) - here) * perCell;
            atmosphere.faceZ.at(x, z) = (atmosphere.stream.at(wrapAir(x + 1), z) - here) * perCell;
        }
    }
    for (int z = 0; z < n; ++z) {
        const int south = wrapAir(z + 1);
        const double drift = 0.5 * (atmosphere.season.drift[size_t(z)] + atmosphere.season.drift[size_t(south)]);
        for (int x = 0; x < n; ++x) {
            atmosphere.windX.at(x, z) = 0.5 * (atmosphere.faceX.at(x, z) + atmosphere.faceX.at(wrapAir(x + 1), z));
            atmosphere.windZ.at(x, z) = 0.5 * (atmosphere.faceZ.at(x, z) + atmosphere.faceZ.at(x, south)) + drift;
        }
    }
}

} // namespace derivean::sim
