#include "volcanic/fn/Volcanic.hpp"

#include "volcanic/constant/Rheology.hpp"

namespace derivean::sim {
namespace {

constexpr int SinkRadius = 14;      // cells; the same reach over which landscape/fn/Uplift.cpp lets the plate answer a load

// Two box passes with clamped edges: the bell over which a plate carries a load.
void spread(std::vector<float>& field, int size, int radius) {
    std::vector<float> work(field.size());
    const float scale = 1.0f / float(2 * radius + 1);
    for (int pass = 0; pass < 2; ++pass) {
        for (int z = 0; z < size; ++z) {
            const auto at = [&](int x) { return field[size_t(z) * size_t(size) + size_t(std::clamp(x, 0, size - 1))]; };
            float sum = 0;
            for (int x = -radius; x <= radius; ++x) sum += at(x);
            for (int x = 0; x < size; ++x) {
                work[size_t(z) * size_t(size) + size_t(x)] = sum * scale;
                sum += at(x + radius + 1) - at(x - radius);
            }
        }
        for (int x = 0; x < size; ++x) {
            const auto at = [&](int z) { return work[size_t(std::clamp(z, 0, size - 1)) * size_t(size) + size_t(x)]; };
            float sum = 0;
            for (int z = -radius; z <= radius; ++z) sum += at(z);
            for (int z = 0; z < size; ++z) {
                field[size_t(z) * size_t(size) + size_t(x)] = sum * scale;
                sum += at(z + radius + 1) - at(z - radius);
            }
        }
    }
}

} // namespace

void allocateVolcanic(Domain& domain) {
    VolcanicDomain& volcanic = domain.volcanic;
    if (!volcanic.floor.empty()) return;
    const size_t cells = domain.cells();
    volcanic.floor.assign(cells, NoPile);
    volcanic.lavaLaid.assign(cells, 0.0f);
    volcanic.allLaid.assign(cells, 0.0f);
    volcanic.laidAt.assign(cells, 0.0f);
    volcanic.vent.assign(cells, 255);
    volcanic.inside.assign(cells, 0);
}

void settleLaying(Domain& domain, const Laying& laying) {
    if (!laying.any) return;
    allocateVolcanic(domain);
    VolcanicDomain& volcanic = domain.volcanic;
    Ground& ground = domain.ground;
    const double area = domain.cell * domain.cell;
    const bool region = domain.key.level == Level::Region;
    const float now = float(domain.time - domain.startTime);
    std::vector<float> load(region ? domain.cells() : 0, 0.0f);
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free) continue;
            const bool owned = domain.owned(x, z);
            if (laying.drop[i] > 0) {
                // The roof of a chamber goes down with its layering.
                ground.rock[i] -= laying.drop[i];
                ground.datum[i] -= laying.drop[i];
                if (!region) domain.trend[i] -= laying.drop[i];
                if (owned) volcanic.ledger.foundered += double(laying.drop[i]) * area;
            }
            const float lava = laying.lava[i], fall = laying.fall[i], ash = laying.ash[i], added = lava + fall + ash;
            if (added <= 0) continue;
            if (owned) volcanic.ledger.built += double(added) * area;
            if (region) load[i] = added;
            else domain.trend[i] += added;
            if (added < WeldDepth) {
                ground.sediment[Coarse][i] += lava + 0.5f * fall;
                ground.sediment[Sand][i] += 0.5f * fall + 0.5f * ash;
                ground.sediment[Fines][i] += 0.5f * ash;
                continue;
            }
            // A pile starts on the rock surface of the day; what lay loose on it is sealed in.
            if (ground.rock[i] <= ground.datum[i] - volcanic.floor[i]) {
                volcanic.floor[i] = ground.datum[i] - ground.rock[i];
                volcanic.lavaLaid[i] = volcanic.allLaid[i] = 0;
            }
            const float loose = domain.sedimentAt(i);
            for (auto& layer : ground.sediment) layer[i] = 0;
            if (owned) volcanic.ledger.sealed += double(loose) * area;
            ground.rock[i] += loose + added;
            volcanic.allLaid[i] += loose + added;
            volcanic.lavaLaid[i] += lava;
            volcanic.laidAt[i] = now;
            volcanic.vent[i] = laying.vent[i];
        }
    }
    if (!region) return;
    // The planet has already raised the province by what its melt added to the crust, lava and
    // ash included (landscape/fn/Forcing.cpp). Where that share came to lie is the pile; the plate
    // under it sinks by all of it, spread as widely as a plate spreads any load, so that the mean
    // rise stays the planet's and the pile only says where the rock is.
    spread(load, domain.size, SinkRadius);
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.hold[i] != Free) continue;
        ground.rock[i] -= load[i];
        ground.datum[i] -= load[i];
    }
}

} // namespace derivean::sim
