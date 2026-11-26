#include "landscape/fn/Drainage.hpp"

#include "core/service/Workers.hpp"
#include "landscape/fn/Forcing.hpp"
#include "landscape/type/Terrain.hpp"
#include "planet/fn/Sample.hpp"

#include <queue>

namespace derivean::sim {

Drainage drainPlanet(const Planet& planet, const Backdrop& backdrop, Workers* workers) {
    const int n = Drainage::Cells;
    const size_t cells = size_t(n) * size_t(n);
    Drainage out;
    out.discharge.assign(cells, 0.0f);
    out.load.assign(cells, 0.0f);
    out.flowTo.assign(cells, uint8_t(255));
    // Ground without the roughness a region seeds: valleys of a few kilometres and more decide where
    // water crosses a domain's edge. Each fine cell belongs to the planet cell its point maps to.
    std::vector<float> ground(cells);
    std::vector<uint32_t> owner(cells);
    const auto sample = [&](int first, int last) {
        for (int z = first; z < last; ++z)
            for (int x = 0; x < n; ++x) {
                const Vec2 point = planetPoint(planet.seed, {(x + 0.5) * Drainage::Cell, (z + 0.5) * Drainage::Cell});
                const size_t i = size_t(z) * size_t(n) + size_t(x);
                ground[i] = float(samplePlanet(backdrop.pastElevation, point));
                const int px = wrapIndex(int(std::floor(point.x / PlanetCell)), PlanetCells), pz = wrapIndex(int(std::floor(point.z / PlanetCell)), PlanetCells);
                owner[i] = uint32_t(pz * PlanetCells + px);
            }
    };
    if (workers) workers->parallelFor(n, sample, 16);
    else sample(0, n);

    // What a planet cell yields by itself is what it passes on beyond what its neighbours send it;
    // its land cells here share that evenly, so every drop the planet's land yields enters once.
    const float sea = float(planet.seaLevel);
    std::vector<uint32_t> share(size_t(PlanetCells) * PlanetCells, 0);
    for (size_t i = 0; i < cells; ++i)
        if (ground[i] >= sea) ++share[owner[i]];
    std::vector<float> yield(share.size()), carried(share.size());
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
            yield[p] = share[p] ? float(std::max(0.0, q - received) / share[p]) : 0.0f;
            // Load goes with the water at the concentration the planet's river has there.
            carried[p] = q > 0 ? backdrop.load.at(x, z) / q : 0.0f;
        }

    // Priority flood from the sea (or from the lowest cell of a dry planet); each cell drains to the
    // cell it was reached from. Ties go by index, so the result has one order.
    using Entry = std::pair<float, uint32_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    std::vector<uint8_t> seen(cells, 0);
    std::vector<uint32_t> order;
    order.reserve(cells);
    for (size_t i = 0; i < cells; ++i)
        if (ground[i] < sea) open.push({sea, uint32_t(i)}), seen[i] = 1;
    if (open.empty()) {
        size_t lowest = 0;
        for (size_t i = 1; i < cells; ++i)
            if (ground[i] < ground[lowest]) lowest = i;
        open.push({ground[lowest], uint32_t(lowest)}), seen[lowest] = 1;
    }
    while (!open.empty()) {
        const auto [level, cell] = open.top();
        open.pop();
        order.push_back(cell);
        const int x = int(cell % uint32_t(n)), z = int(cell / uint32_t(n));
        for (int k = 0; k < 8; ++k) {
            const size_t other = out.index(x + NeighborX[k], z + NeighborZ[k]);
            if (seen[other]) continue;
            seen[other] = 1;
            out.flowTo[other] = uint8_t((k + 4) & 7);
            open.push({std::max(level, ground[other]), uint32_t(other)});
        }
    }
    // Sea cells keep 255: the region's own sea takes what reaches them.
    for (size_t step = order.size(); step-- > 0;) {
        const uint32_t cell = order[step];
        if (ground[cell] >= sea) out.discharge[cell] += yield[owner[cell]];
        out.load[cell] = out.discharge[cell] * carried[owner[cell]];
        const uint8_t to = out.flowTo[cell];
        if (to < 8) out.discharge[out.index(int(cell % uint32_t(n)) + NeighborX[to], int(cell / uint32_t(n)) + NeighborZ[to])] += out.discharge[cell];
    }
    return out;
}

} // namespace derivean::sim
