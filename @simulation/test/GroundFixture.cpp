#include "GroundFixture.hpp"

#include "landscape/fn/Forcing.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

Domain groundDomain(const GroundSpec& spec) {
    Domain domain;
    domain.key = {Level::Ground, 0, 0};
    domain.cell = specOf(Level::Ground).cell;
    domain.step = specOf(Level::Ground).step;
    domain.origin = {-Halo * domain.cell, -Halo * domain.cell};
    domain.seaLevel = -1e4;
    const size_t cells = domain.cells();
    domain.ground.resize(cells);
    domain.hold.assign(cells, Free);
    domain.outlet.assign(cells, 0);
    domain.fold.assign(cells, 0.0f);
    for (auto* field : {&domain.tectonic, &domain.residual, &domain.weakness, &domain.rebound, &domain.trend, &domain.uplift, &domain.rainBase, &domain.rain,
                        &domain.warmth, &domain.runoff, &domain.warmthNow, &domain.inflow, &domain.inflowLoad, &domain.settledFlow, &domain.settledLoad, &domain.windX,
                        &domain.windZ, &domain.rimSlope, &domain.rimFloor, &domain.surface, &domain.level, &domain.discharge, &domain.ice, &domain.iceAge,
                        &domain.erosionRate, &domain.depositionRate, &domain.eroded, &domain.deposited, &domain.uplifted, &domain.shed, &domain.stream})
        field->assign(cells, 0.0f);
    domain.quality.assign(cells, 1.0f);
    domain.standing.assign(cells, -1e9f);
    domain.rank.assign(cells, 0);
    domain.spill.assign(cells, -1);
    domain.stride = 64;
    domain.nodes = (domain.size + domain.stride - 1) / domain.stride + 1;
    domain.strata.assign(size_t(domain.nodes) * size_t(domain.nodes), StrataColumn{});
    domain.unit.assign(cells, UnitCache{-1e30f, 1e30f, traitsOf(spec.rock)});
    domain.frost = spec.frost;
    domain.halfRange = 12.0f;

    const int last = domain.size - 1;
    float lowest = 1e30f;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.rock[i] = domain.ground.datum[i] = spec.ground(x, z);
            domain.ground.sediment[Coarse][i] = spec.coarse;
            domain.ground.sediment[Sand][i] = spec.sand;
            domain.ground.sediment[Fines][i] = spec.fines;
            domain.trend[i] = domain.ground.rock[i] + domain.sedimentAt(i);
            domain.rain[i] = domain.rainBase[i] = spec.rain;
            domain.warmthNow[i] = domain.warmth[i] = spec.warmth;
            domain.runoff[i] = runoffOf(spec.rain, spec.warmth);
            if (x > 0 && z > 0 && x < last && z < last) continue;
            domain.hold[i] = Rim;
            lowest = std::min(lowest, domain.ground.rock[i]);
        }
    // Water leaves over the lowest stretch of the rim.
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x)
            if (domain.hold[domain.index(x, z)] == Rim) domain.outlet[domain.index(x, z)] = domain.ground.rock[domain.index(x, z)] < lowest + 0.5f;
    domain.settledFrom = domain.settledTo = domain.ground;
    return domain;
}

void expectGround(bool passed, const char* message, double value) {
    if (passed) return;
    std::cerr << "Ground: " << message << " (" << value << ")\n";
    std::exit(1);
}
