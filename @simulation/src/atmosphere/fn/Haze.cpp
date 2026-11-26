#include "atmosphere/fn/Kernels.hpp"

#include "climate/constant/Moisture.hpp"
#include "core/service/Workers.hpp"
#include "hydrology/constant/Water.hpp"

namespace derivean::sim {
namespace {

// Dry optics at 550 nm and how each kind takes up water. Mass extinction is Mie theory over the
// size distribution named; hygroscopicity is κ of Petters and Kreidenweis (2007), with which a
// particle at relative humidity h holds 1 + κ·h/(1 − h) times its dry volume. Extinction then grows
// as that volume ratio to the power `swelling`/3, fitted to Mie theory over the swelling particles
// (water lowering their refractive index) for humidities up to 95 %.
struct Optics {
    double extinction;   // m²/kg of dry aerosol
    double kappa;
    double swelling;
    double radius;       // m, mass median of the dry particles
    double density;      // kg/m³ dry
};
constexpr Optics KindOptics[HazeKinds] = {
    {740.0, 0.03, 2.0, 1.25e-6, 2650.0},    // mineral dust, lognormal σ 2 (Koehler et al. 2009 for κ)
    {780.0, 1.1, 1.95, 1.0e-6, 2160.0},     // sea salt of the Gong spectrum (Zieger et al. 2017 for κ)
    {3660.0, 0.61, 2.2, 0.3e-6, 1770.0},    // ammonium sulphate, accumulation mode σ 2
    {4010.0, 0.1, 2.35, 0.3e-6, 1400.0},    // secondary organics, accumulation mode σ 2
};
// Above this humidity haze particles grow into fog and cloud droplets, which the clouds own.
constexpr double HumidityCap = 0.95;

// Every kind here is large enough to become a cloud droplet at a cloud's supersaturation (κ-Köhler),
// so the haze leaves with the water of the low band: the share of that water the step rained out.

// Particles leave the low band by falling (Stokes, at their swollen size and density) and by
// sticking to what the air brushes past near the ground: Wesely et al. (1985) found 0.002 of the
// friction velocity for accumulation-mode sulphate.
constexpr double AirViscosity = 1.8e-5;    // Pa·s
// Mean square radius of the mass over a lognormal of geometric spread 2, against the square of its median: exp(2 ln² 2).
constexpr double SpreadSquare = 2.6152;
constexpr double Brushing = 0.002;
constexpr double LandRoughness = 0.1;      // m, mixed plant cover
constexpr double SeaRoughness = 2e-4;      // m

double humidityOf(const AirColumn& column) { return std::clamp(column.vapour[Low] * profileOf(column).perCapacity[Low], 0.0, HumidityCap); }

double swellOf(int kind, double humidity) { return 1.0 + KindOptics[kind].kappa * humidity / (1.0 - humidity); }

double opticalDepth(int kind, double mass, double humidity) {
    return mass * KindOptics[kind].extinction * std::pow(swellOf(kind, humidity), KindOptics[kind].swelling / 3.0);
}

// Rain takes down a share of everything, the ground a share of each kind, and the same pass
// measures how much the haze left hides.
void depositHaze(Atmosphere& atmosphere, int begin, int end, std::array<double, 2 * HazeKinds>* rows) {
    Haze& haze = atmosphere.haze;
    for (int z = begin; z < end; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = atmosphere.ground.index(x, z);
            const AirColumn column = columnAt(atmosphere, x, z);
            const double humidity = humidityOf(column);
            const double rain = (atmosphere.rainRate.cells[cell] + atmosphere.snowRate.cells[cell]) * AirStep;
            const double washed = rain / (column.vapour[Low] + column.liquid[Low] + column.ice[Low] + rain + 1e-12);
            const double speed = std::hypot(std::hypot(atmosphere.windX.cells[cell], atmosphere.windZ.cells[cell]), Gustiness);
            const double roughness = mix(LandRoughness, SeaRoughness, atmosphere.openWater.cells[cell]);
            const double friction = 0.4 * speed / std::log(10.0 / roughness);
            double depth = 0;
            for (int kind = 0; kind < HazeKinds; ++kind) {
                const Optics& optics = KindOptics[kind];
                const double swell = swellOf(kind, humidity), radius = optics.radius * std::cbrt(swell);
                const double density = (optics.density + (swell - 1.0) * WaterDensity) / swell;
                const double falling = 2.0 / 9.0 * density * Gravity * SpreadSquare * radius * radius / AirViscosity + Brushing * friction;
                double& mass = haze.mass[size_t(kind)].cells[cell];
                const double rained = mass * washed;
                const double settled = (mass - rained) * (1.0 - std::exp(-falling * AirStep / LowDepth));
                mass -= rained + settled;
                rows[z][size_t(kind)] += rained * AirCellArea;
                rows[z][size_t(HazeKinds + kind)] += settled * AirCellArea;
                depth += opticalDepth(kind, mass, humidity);
            }
            haze.depth.cells[cell] = depth;
        }
    }
}

} // namespace

void stepHaze(Atmosphere& atmosphere, Workers& workers) {
    Haze& haze = atmosphere.haze;
    constexpr int RowGrain = 16;
    std::array<std::array<double, HazeKinds>, AirCells> raised{};
    workers.parallelFor(AirCells, [&](int begin, int end) { raiseHaze(atmosphere, begin, end, raised.data()); }, RowGrain);

    // The wind carries the haze in each wind step beside the water (Advance.cpp); what was raised
    // here rides it from the next step on.
    std::array<std::array<double, 2 * HazeKinds>, AirCells> lost{};
    workers.parallelFor(AirCells, [&](int begin, int end) { depositHaze(atmosphere, begin, end, lost.data()); }, RowGrain);
    for (int z = 0; z < AirCells; ++z)
        for (int kind = 0; kind < HazeKinds; ++kind) {
            haze.totals.emitted[size_t(kind)] += raised[size_t(z)][size_t(kind)];
            haze.totals.washed[size_t(kind)] += lost[size_t(z)][size_t(kind)];
            haze.totals.settled[size_t(kind)] += lost[size_t(z)][size_t(HazeKinds + kind)];
        }
}

std::array<double, HazeKinds> hazeDepths(const Atmosphere& atmosphere, int x, int z) {
    const double humidity = humidityOf(columnAt(atmosphere, x, z));
    const size_t cell = atmosphere.ground.index(x, z);
    std::array<double, HazeKinds> depths{};
    for (int kind = 0; kind < HazeKinds; ++kind) depths[size_t(kind)] = opticalDepth(kind, atmosphere.haze.mass[size_t(kind)].cells[cell], humidity);
    return depths;
}

double viewDistance(const Atmosphere& atmosphere, Vec2 position, double height) {
    const int x = wrapAir(floorToInt(position.x / AirCell)), z = wrapAir(floorToInt(position.z / AirCell));
    const size_t cell = atmosphere.ground.index(x, z);
    const double ground = atmosphere.ground.cells[cell], above = std::max(height, ground);
    double extinction = RayleighExtinction * std::exp(-above / RayleighHeight);
    if (above < ground + LowDepth) extinction += (atmosphere.haze.depth.cells[cell] + atmosphere.plume.lowDepth.cells[cell]) / LowDepth;
    return visibilityOf(extinction);
}

} // namespace derivean::sim
