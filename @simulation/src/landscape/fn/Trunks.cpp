#include "landscape/fn/Trunks.hpp"

#include "core/service/Workers.hpp"
#include "landscape/fn/Forcing.hpp"
#include "landscape/fn/Process.hpp"
#include "landscape/fn/TrunkFlow.hpp"
#include "planet/fn/Sample.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

namespace derivean::sim {
namespace {

// Erosion steps over the region window; the implicit scheme is stable at any step.
constexpr int ErosionSteps = 12;
// Stream power coefficient of a mid-range rock (sandstone 3e-6, gneiss 1e-6), m^(1-2m)/yr.
constexpr float TrunkErodibility = 2e-6f;
// Radius in region cells of the two box passes over which the plate answers a region's erosion with
// isostatic rise (landscape/fn/Uplift.cpp).
constexpr int SinkRegionCells = 14;
// m by which the isostatic answer to the erosion beside a coast may lift a shelf over a window
// (no cell of seed 42 rises to land from further below).
constexpr float ReboundReach = 3000.0f;
// Columns a worker sweeps together in the column pass of the blur, so that it reads whole rows.
constexpr int BlurStrip = 64;

// Rows (or strips) of `body` over `workers` when given; the result does not depend on it.
void spread(Workers* workers, int count, const std::function<void(int, int)>& body, int grain = 16) {
    if (workers) workers->parallelFor(count, body, grain);
    else body(0, count);
}

// Two passes of a wrapped box mean of `radius` cells along both axes.
void blur(std::vector<float>& field, std::vector<float>& work, int radius, Workers* workers) {
    constexpr int n = Trunks::Cells;
    const float scale = 1.0f / float(2 * radius + 1);
    const auto wrap = [](int i) { return i < 0 ? i + n : i >= n ? i - n : i; };
    for (int pass = 0; pass < 2; ++pass) {
        spread(workers, n, [&](int first, int last) {
            for (int z = first; z < last; ++z) {
                const float* row = &field[size_t(z) * n];
                double sum = 0;
                for (int x = -radius; x <= radius; ++x) sum += row[wrap(x)];
                for (int x = 0; x < n; ++x) {
                    work[size_t(z) * n + size_t(x)] = float(sum) * scale;
                    sum += row[wrap(x + radius + 1)] - row[wrap(x - radius)];
                }
            }
        });
        spread(workers, n / BlurStrip, [&](int first, int last) {
            double sum[BlurStrip];
            for (int strip = first; strip < last; ++strip) {
                const size_t x0 = size_t(strip) * BlurStrip;
                std::fill(sum, sum + BlurStrip, 0.0);
                for (int z = -radius; z <= radius; ++z)
                    for (int k = 0; k < BlurStrip; ++k) sum[k] += work[size_t(wrap(z)) * n + x0 + size_t(k)];
                for (int z = 0; z < n; ++z) {
                    const float* enter = &work[size_t(wrap(z + radius + 1)) * n + x0];
                    const float* leave = &work[size_t(wrap(z - radius)) * n + x0];
                    float* row = &field[size_t(z) * n + x0];
                    for (int k = 0; k < BlurStrip; ++k) row[k] = float(sum[k]) * scale, sum[k] += enter[k] - leave[k];
                }
            }
        }, 1);
    }
}

// Radius in trunk cells of the box whose spread matches the region's: (2r+1)^2 - 1 is twelve times
// the variance of a box of r cells.
int sinkRadius(double regionCell) {
    const double spread = (std::pow(2 * SinkRegionCells + 1, 2) - 1) * regionCell * regionCell;
    return std::max(1, int(std::lround((std::sqrt(spread / (Trunks::Cell * Trunks::Cell) + 1) - 1) / 2)));
}

} // namespace

Trunks routeTrunks(const Setting& setting, Workers* workers) {
    const Planet& planet = *setting.planet;
    const Backdrop& backdrop = *setting.backdrop;
    const int n = Trunks::Cells;
    const size_t cells = size_t(n) * size_t(n);
    const double window = std::max(0.0, planet.time - backdrop.regionStart);
    const double dt = window / ErosionSteps;
    float lowestSea = float(seaLevelAt(setting, backdrop.regionStart));
    for (int step = 1; step <= ErosionSteps; ++step) lowestSea = std::min(lowestSea, float(seaLevelAt(setting, backdrop.regionStart + step * dt)));
    Trunks out;
    out.ground.resize(cells);
    out.discharge.assign(cells, 0.0f);
    out.load.assign(cells, 0.0f);
    out.flowTo.assign(cells, uint8_t(255));
    std::vector<float> quality(cells, 1.0f), uplift(cells);
    TrunkRouting routing;
    routing.owner.resize(cells);
    const double regionCell = specOf(Level::Region).cell;
    const int sub = int(std::lround(Trunks::Cell / regionCell));
    spread(workers, n, [&](int first, int last) {
        for (int z = first; z < last; ++z)
            for (int x = 0; x < n; ++x) {
                const Vec2 here{(x + 0.5) * Trunks::Cell, (z + 0.5) * Trunks::Cell};
                const Vec2 point = planetPoint(planet.seed, here);
                const size_t i = size_t(z) * size_t(n) + size_t(x);
                const int px = wrapIndex(int(std::floor(point.x / PlanetCell)), PlanetCells), pz = wrapIndex(int(std::floor(point.z / PlanetCell)), PlanetCells);
                routing.owner[i] = uint16_t(pz * PlanetCells + px);
                // The rock uplift a region gets over the window, structure included: ranges, grabens
                // and tilted blocks tens of kilometres across, which the planet grid cannot hold.
                const Forcing forcing = forcingAt(setting, here);
                uplift[i] = forcing.tectonic + forcing.residual;
                const float base = float(samplePlanet(backdrop.pastElevation, point));
                out.ground[i] = base;
                // Ground that neither the uplift nor the isostatic answer to erosion beside it can raise
                // to the lowest sea of the window stays sea whatever its roughness.
                if (base + RoughHeight[1] + std::max(0.0f, uplift[i]) * float(window) + ReboundReach < lowestSea) continue;
                // The roughness and rock quality a region seeds, read in the crust's frame as a region
                // reads them, as their mean over the region cells this cell covers: what is finer than
                // this grid averages out instead of standing in for it. Without them the flood runs the
                // rivers of a smooth surface down parallel lines a cell apart.
                double rough = 0, rock = 0;
                for (int s = 0; s < sub * sub; ++s) {
                    const Vec2 at = forcing.strata.material + Vec2{(s % sub + 0.5 - 0.5 * sub) * regionCell, (s / sub + 0.5 - 0.5 * sub) * regionCell};
                    rough += roughnessAt(planet.seed, at, RoughLength[1]);
                    rock += qualityAt(planet.seed, at, QualitySpan[1] * regionCell);
                }
                out.ground[i] = base + RoughHeight[1] * float(rough / (sub * sub));
                quality[i] = float(rock / (sub * sub));
            }
    });

    const std::vector<float> own = ownYield(backdrop);
    float sea = float(seaLevelAt(setting, backdrop.regionStart));
    markSea(out, routing, sea);
    shareYield(out, routing, planet, own);
    routeTrunkWater(out, routing, sea);

    // The region window, outlets first: each cell is lowered toward the cell it drains to at the pace
    // its water cuts, never below it (implicit, n = 1); a cell lower than the one it drains to does
    // not cut. Then every cell rises by the forcing's uplift and by the plate's isostatic answer to the
    // erosion around it, as a region's ground does, and the water finds its way anew at the sea level
    // of the step's end. The field deposits nothing, so its basins do not sink under their fill.
    const int sink = sinkRadius(regionCell);
    std::vector<float> cut(cells), work(cells);
    for (int step = 0; step < ErosionSteps && dt > 0; ++step) {
        std::fill(cut.begin(), cut.end(), 0.0f);
        for (const uint32_t cell : routing.order) {
            const uint8_t to = out.flowTo[cell];
            if (to > 7 || routing.sea[cell]) continue;
            const float floor = std::max(out.ground[trunkNeighbour(cell, to)], sea);
            if (out.ground[cell] <= floor) continue;
            const float f = TrunkErodibility * quality[cell] * std::pow(out.discharge[cell], float(StreamExponent)) * float(dt) / float(Trunks::Cell * NeighborReach[to]);
            const float lowered = floor + (out.ground[cell] - floor) / (1 + f);
            cut[cell] = out.ground[cell] - lowered;
            out.ground[cell] = lowered;
        }
        blur(cut, work, sink, workers);
        spread(workers, n, [&](int first, int last) {
            for (size_t i = size_t(first) * n; i < size_t(last) * n; ++i) out.ground[i] += uplift[i] * float(dt) + float(IsostaticSink) * cut[i];
        });
        sea = float(seaLevelAt(setting, backdrop.regionStart + (step + 1) * dt));
        markSea(out, routing, sea);
        shareYield(out, routing, planet, own);
        routeTrunkWater(out, routing, sea);
    }
    // Load goes with the water at the concentration the planet's river has there.
    const std::vector<float> concentration = loadConcentration(backdrop);
    for (size_t i = 0; i < cells; ++i) out.load[i] = out.discharge[i] * concentration[routing.owner[i]];
    return out;
}

} // namespace derivean::sim
