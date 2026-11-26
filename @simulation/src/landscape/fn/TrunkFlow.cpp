#include "landscape/fn/TrunkFlow.hpp"

#include <algorithm>
#include <queue>

namespace derivean::sim {

std::vector<float> ownYield(const Backdrop& backdrop) {
    std::vector<float> own(size_t(PlanetCells) * PlanetCells);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            double received = 0;
            for (int k = 0; k < 8; ++k) {
                const int nx = wrapIndex(x + NeighborX[k], PlanetCells), nz = wrapIndex(z + NeighborZ[k], PlanetCells);
                const uint8_t to = backdrop.flowTo.at(nx, nz);
                if (to < 8 && wrapIndex(nx + NeighborX[to], PlanetCells) == x && wrapIndex(nz + NeighborZ[to], PlanetCells) == z) received += backdrop.discharge.at(nx, nz);
            }
            own[size_t(z) * PlanetCells + size_t(x)] = float(std::max(0.0, backdrop.discharge.at(x, z) - received));
        }
    return own;
}

std::vector<float> loadConcentration(const Backdrop& backdrop) {
    std::vector<float> concentration(size_t(PlanetCells) * PlanetCells);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            const float q = backdrop.discharge.at(x, z);
            concentration[size_t(z) * PlanetCells + size_t(x)] = q > 0 ? backdrop.load.at(x, z) / q : 0.0f;
        }
    return concentration;
}

void markSea(const Trunks& out, TrunkRouting& routing, float sea) {
    const size_t cells = out.ground.size();
    routing.sea.resize(cells);
    for (size_t i = 0; i < cells; ++i) routing.sea[i] = out.ground[i] < sea;
    std::vector<uint8_t> coast(cells, 0);
    for (uint32_t i = 0; i < uint32_t(cells); ++i) {
        if (routing.sea[i]) continue;
        for (int k = 0; k < 8; ++k) {
            const uint32_t other = trunkNeighbour(i, k);
            if (routing.sea[other]) coast[other] = 1;
        }
    }
    routing.coast.clear();
    for (uint32_t i = 0; i < uint32_t(cells); ++i)
        if (coast[i]) routing.coast.push_back(i);
}

// A land cell of the planet shares its own yield evenly over its land cells here, or with none gives
// it to its highest cell, where it reaches the sea at once: every drop the planet's land yields
// enters once, unless no cell here maps to the planet cell at all. A sea cell of the planet yields the
// rain on its area; only what falls on land here enters.
void shareYield(const Trunks& out, TrunkRouting& routing, const Planet& planet, const std::vector<float>& own) {
    const size_t cells = out.ground.size(), planetCells = own.size();
    std::vector<uint32_t> share(planetCells, 0), highest(planetCells, uint32_t(cells));
    for (size_t i = 0; i < cells; ++i) {
        const uint16_t p = routing.owner[i];
        if (!routing.sea[i]) ++share[p];
        if (highest[p] == cells || out.ground[i] > out.ground[highest[p]]) highest[p] = uint32_t(i);
    }
    routing.yield.assign(planetCells, 0.0f);
    routing.drowned.clear();
    for (size_t p = 0; p < planetCells; ++p) {
        if (planet.elevation.cells[p] <= planet.seaLevel) routing.yield[p] = own[p] / float(Trunks::Ratio * Trunks::Ratio);
        else if (share[p]) routing.yield[p] = own[p] / float(share[p]);
        else if (own[p] > 0 && highest[p] < cells) routing.drowned.push_back({highest[p], own[p]});
    }
}

// Each cell drains to the cell it was reached from. Ties go by index, so the result has one order.
// Sea cells away from the coast take no water and are left out. Then the water, upstream first.
void routeTrunkWater(Trunks& out, TrunkRouting& routing, float sea) {
    using Entry = std::pair<float, uint32_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    std::vector<uint8_t> seen = routing.sea;
    std::fill(out.flowTo.begin(), out.flowTo.end(), uint8_t(255));
    routing.order.clear();
    for (const uint32_t cell : routing.coast) open.push({sea, cell});
    if (open.empty()) {   // a dry planet drains to its lowest cell
        size_t lowest = 0;
        for (size_t i = 1; i < out.ground.size(); ++i)
            if (out.ground[i] < out.ground[lowest]) lowest = i;
        open.push({out.ground[lowest], uint32_t(lowest)}), seen[lowest] = 1;
    }
    while (!open.empty()) {
        const auto [level, cell] = open.top();
        open.pop();
        routing.order.push_back(cell);
        for (int k = 0; k < 8; ++k) {
            const uint32_t other = trunkNeighbour(cell, k);
            if (seen[other]) continue;
            seen[other] = 1;
            out.flowTo[other] = uint8_t((k + 4) & 7);
            open.push({std::max(level, out.ground[other]), other});
        }
    }
    std::fill(out.discharge.begin(), out.discharge.end(), 0.0f);
    for (size_t step = routing.order.size(); step-- > 0;) {
        const uint32_t cell = routing.order[step];
        if (!routing.sea[cell]) out.discharge[cell] += routing.yield[routing.owner[cell]];
        const uint8_t to = out.flowTo[cell];
        if (to < 8) out.discharge[trunkNeighbour(cell, to)] += out.discharge[cell];
    }
    for (const auto& [cell, water] : routing.drowned) out.discharge[cell] += water;
}

} // namespace derivean::sim
