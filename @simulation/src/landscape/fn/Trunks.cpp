#include "landscape/fn/Trunks.hpp"

#include "core/service/Workers.hpp"
#include "landscape/fn/Forcing.hpp"
#include "landscape/fn/Process.hpp"
#include "planet/fn/Sample.hpp"

#include <cmath>
#include <queue>

namespace derivean::sim {
namespace {

// Erosion steps over the region window; the implicit scheme is stable at any step.
constexpr int ErosionSteps = 12;
// Stream power coefficient of a mid-range rock (sandstone 3e-6, gneiss 1e-6), m^(1-2m)/yr.
constexpr float TrunkErodibility = 2e-6f;

struct Routing {
    std::vector<uint32_t> order;   // outlets first
    std::vector<uint16_t> owner;   // planet cell each cell's point maps to
    std::vector<float> yield;      // m³/yr each land cell of a planet cell yields
};

// Priority flood from the sea (or from the lowest cell of a dry planet); each cell drains to the cell
// it was reached from. Ties go by index, so the result has one order. Then the water, upstream first.
void route(Trunks& out, Routing& routing, float sea) {
    const size_t cells = out.ground.size();
    using Entry = std::pair<float, uint32_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    std::vector<uint8_t> seen(cells, 0);
    std::fill(out.flowTo.begin(), out.flowTo.end(), uint8_t(255));
    routing.order.clear();
    routing.order.reserve(cells);
    for (size_t i = 0; i < cells; ++i)
        if (out.ground[i] < sea) open.push({sea, uint32_t(i)}), seen[i] = 1;
    if (open.empty()) {
        size_t lowest = 0;
        for (size_t i = 1; i < cells; ++i)
            if (out.ground[i] < out.ground[lowest]) lowest = i;
        open.push({out.ground[lowest], uint32_t(lowest)}), seen[lowest] = 1;
    }
    while (!open.empty()) {
        const auto [level, cell] = open.top();
        open.pop();
        routing.order.push_back(cell);
        const int x = int(cell % uint32_t(Trunks::Cells)), z = int(cell / uint32_t(Trunks::Cells));
        for (int k = 0; k < 8; ++k) {
            const size_t other = out.index(x + NeighborX[k], z + NeighborZ[k]);
            if (seen[other]) continue;
            seen[other] = 1;
            out.flowTo[other] = uint8_t((k + 4) & 7);
            open.push({std::max(level, out.ground[other]), uint32_t(other)});
        }
    }
    std::fill(out.discharge.begin(), out.discharge.end(), 0.0f);
    for (size_t step = routing.order.size(); step-- > 0;) {
        const uint32_t cell = routing.order[step];
        if (out.ground[cell] >= sea) out.discharge[cell] += routing.yield[routing.owner[cell]];
        const uint8_t to = out.flowTo[cell];
        if (to < 8) out.discharge[out.index(int(cell % uint32_t(Trunks::Cells)) + NeighborX[to], int(cell / uint32_t(Trunks::Cells)) + NeighborZ[to])] += out.discharge[cell];
    }
}

} // namespace

Trunks routeTrunks(const Planet& planet, const Backdrop& backdrop, Workers* workers) {
    const int n = Trunks::Cells;
    const size_t cells = size_t(n) * size_t(n);
    Trunks out;
    out.ground.resize(cells);
    out.discharge.assign(cells, 0.0f);
    out.load.assign(cells, 0.0f);
    out.flowTo.assign(cells, uint8_t(255));
    std::vector<float> quality(cells);
    Routing routing;
    routing.owner.resize(cells);
    const double regionCell = specOf(Level::Region).cell;
    const auto sample = [&](int first, int last) {
        for (int z = first; z < last; ++z)
            for (int x = 0; x < n; ++x) {
                const Vec2 here{(x + 0.5) * Trunks::Cell, (z + 0.5) * Trunks::Cell};
                const Vec2 point = planetPoint(planet.seed, here);
                const size_t i = size_t(z) * size_t(n) + size_t(x);
                // The roughness and rock quality a region seeds, here at the world position rather than
                // the crust's: without them the flood runs the rivers of a smooth surface down parallel
                // lines a cell apart.
                out.ground[i] = float(samplePlanet(backdrop.pastElevation, point)) + RoughHeight[1] * roughnessAt(planet.seed, here, RoughLength[1]);
                quality[i] = qualityAt(planet.seed, here, QualitySpan[1] * regionCell);
                const int px = wrapIndex(int(std::floor(point.x / PlanetCell)), PlanetCells), pz = wrapIndex(int(std::floor(point.z / PlanetCell)), PlanetCells);
                routing.owner[i] = uint16_t(pz * PlanetCells + px);
            }
    };
    if (workers) workers->parallelFor(n, sample, 16);
    else sample(0, n);

    // What a planet cell yields by itself is what it passes on beyond what its neighbours send it;
    // its land cells here share that evenly, so every drop the planet's land yields enters once.
    const float sea = float(planet.seaLevel);
    std::vector<uint32_t> share(size_t(PlanetCells) * PlanetCells, 0);
    for (size_t i = 0; i < cells; ++i)
        if (out.ground[i] >= sea) ++share[routing.owner[i]];
    routing.yield.resize(share.size());
    std::vector<float> concentration(share.size());
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            double received = 0;
            for (int k = 0; k < 8; ++k) {
                const int nx = wrapIndex(x + NeighborX[k], PlanetCells), nz = wrapIndex(z + NeighborZ[k], PlanetCells);
                const uint8_t to = backdrop.flowTo.at(nx, nz);
                if (to < 8 && wrapIndex(nx + NeighborX[to], PlanetCells) == x && wrapIndex(nz + NeighborZ[to], PlanetCells) == z) received += backdrop.discharge.at(nx, nz);
            }
            const size_t p = size_t(z) * PlanetCells + size_t(x);
            const float q = backdrop.discharge.at(x, z);
            routing.yield[p] = share[p] ? float(std::max(0.0, q - received) / share[p]) : 0.0f;
            concentration[p] = q > 0 ? backdrop.load.at(x, z) / q : 0.0f;
        }
    route(out, routing, sea);

    // The region window's erosion, implicit along each cell's way down (n = 1), outlets first: a cell
    // is lowered toward the cell it drains to at the pace its water cuts, never below it. A cell lower
    // than the one it drains to does not cut.
    const float dt = float(std::max(0.0, planet.time - backdrop.regionStart) / ErosionSteps);
    for (int step = 0; step < ErosionSteps && dt > 0; ++step) {
        for (const uint32_t cell : routing.order) {
            const uint8_t to = out.flowTo[cell];
            if (to > 7 || out.ground[cell] < sea) continue;
            const size_t below = out.index(int(cell % uint32_t(n)) + NeighborX[to], int(cell / uint32_t(n)) + NeighborZ[to]);
            const float floor = std::max(out.ground[below], sea);
            if (out.ground[cell] <= floor) continue;
            const float f = TrunkErodibility * quality[cell] * std::pow(out.discharge[cell], float(StreamExponent)) * dt / float(Trunks::Cell * NeighborReach[to]);
            out.ground[cell] = floor + (out.ground[cell] - floor) / (1 + f);
        }
        route(out, routing, sea);
    }
    // Load goes with the water at the concentration the planet's river has there.
    for (size_t i = 0; i < cells; ++i) out.load[i] = out.discharge[i] * concentration[routing.owner[i]];
    return out;
}

} // namespace derivean::sim
