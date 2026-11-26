#include "GroundFixture.hpp"

#include "ground/type/Detail.hpp"
#include "landscape/fn/Forcing.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
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

namespace {

bool sameFloats(const std::vector<float>& a, const std::vector<float>& b) {
    return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
}

bool sameClast(const Clast& a, const Clast& b) {
    const auto values = [](const Clast& s) {
        return std::array{s.x, s.y, s.z, s.a, s.b, s.c, s.yaw, s.pitch, s.roll, s.roundness, s.travel};
    };
    const auto av = values(a), bv = values(b);
    return std::memcmp(av.data(), bv.data(), av.size() * sizeof(float)) == 0 &&
        std::bit_cast<uint64_t>(a.source.x) == std::bit_cast<uint64_t>(b.source.x) &&
        std::bit_cast<uint64_t>(a.source.z) == std::bit_cast<uint64_t>(b.source.z) &&
        a.rock == b.rock && a.origin == b.origin;
}

bool sameLedger(const ClastLedger& a, const ClastLedger& b) {
    const auto values = [](const ClastLedger& l) {
        return std::array{l.coarseBefore, l.dug, l.matrix, l.stones, l.offered};
    };
    const auto av = values(a), bv = values(b);
    return std::memcmp(av.data(), bv.data(), av.size() * sizeof(double)) == 0 && std::ranges::equal(a.byOrigin, b.byOrigin);
}

bool sameGroundDetail(const GroundDetail& a, const GroundDetail& b) {
    return std::ranges::equal(a.clasts, b.clasts, sameClast) && a.binStart == b.binStart &&
        sameFloats(a.crust, b.crust) && a.crustKind == b.crustKind && a.coarseKind == b.coarseKind &&
        a.fineKind == b.fineKind && a.bare == b.bare && sameFloats(a.stepGaps, b.stepGaps) && sameLedger(a.ledger, b.ledger);
}

} // namespace

bool sameTile(const Tile& a, const Tile& b) {
    for (int c = 0; c < SedimentClasses; ++c)
        if (!sameFloats(a.now.sediment[c], b.now.sediment[c])) return false;
    return sameFloats(a.now.rock, b.now.rock) && sameFloats(a.now.datum, b.now.datum) &&
        sameFloats(a.water, b.water) && sameFloats(a.discharge, b.discharge) && a.flowTo == b.flowTo &&
        a.detail && b.detail && sameGroundDetail(*a.detail, *b.detail);
}
