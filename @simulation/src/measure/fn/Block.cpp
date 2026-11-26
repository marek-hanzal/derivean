#include "measure/fn/Block.hpp"

#include "climate/type/Climate.hpp"
#include "core/fn/Noise.hpp"
#include "landscape/fn/Build.hpp"
#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Plane.hpp"

namespace derivean::sim {
namespace {

constexpr float RiverLevel = 100.0f;   // m the river's outlet stands above the sea
constexpr float RiverFall = 0.002f;    // rise over run of the river's bed
constexpr float Rough = 0.4f;          // m, seeded unevenness of the plateau
constexpr float DatumAbove = 4.0f;     // m of the top bed that has already gone from the highest ground

// The bed a cell has cut down to: beds of equal thickness counted down from the datum.
void refreshBeds(Domain& domain, const BlockSpec& spec) {
    if (spec.weak == Material::Air) return;
    const float thickness = float(spec.bedThickness);
    for (size_t i = 0; i < domain.cells(); ++i) {
        const float depth = std::max(0.0f, domain.ground.datum[i] - domain.ground.rock[i]);
        UnitCache& cache = domain.unit[i];
        if (depth >= cache.top && depth < cache.bottom) continue;
        const int bed = int(depth / thickness);
        cache = {bed * thickness, (bed + 1) * thickness, traitsOf(bed % 2 ? spec.weak : spec.rock)};
    }
}

} // namespace

Domain blockDomain(const BlockSpec& spec) {
    const LevelSpec& level = specOf(spec.level);
    Domain domain;
    domain.key = {spec.level, 0, 0};
    domain.cell = level.cell;
    domain.step = level.step;
    domain.origin = {-Halo * level.cell, -Halo * level.cell};
    const int steps = spec.steps > 0 ? spec.steps : int(std::llround(level.span / level.step));
    domain.startTime = domain.time = 0;
    domain.endTime = steps * level.step;
    domain.seaLevel = 0;
    allocateBare(domain);

    const int size = domain.size, last = size - 1, middle = size / 2;
    domain.sound.assign(domain.cells(), float(spec.sound));
    domain.unit.assign(domain.cells(), spec.weak == Material::Air ? UnitCache{-1e30f, 1e30f, traitsOf(spec.rock)} : UnitCache{});
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const float bed = RiverLevel + RiverFall * float((last - x) * level.cell);
            const bool river = std::abs(z - middle) <= spec.floorHalf;
            const float plateau = bed + float(spec.height) + Rough * float(fractalNoise(spec.seed, x / 6.1, z / 6.1, 3));
            domain.ground.rock[i] = river ? bed : plateau;
            // The beds are a surface of their own: level, or falling east under a plateau that cuts across them.
            domain.ground.datum[i] = RiverLevel + RiverFall * float(last * level.cell) + float(spec.height) + Rough + DatumAbove + float(spec.dip * (last - x) * level.cell);
            domain.trend[i] = domain.ground.rock[i];
            domain.rain[i] = domain.rainBase[i] = float(spec.rain);
            domain.warmth[i] = float(spec.warmth + LapseRate * domain.ground.rock[i]);
            domain.rimFloor[i] = float(domain.seaLevel);
            if (x > 0 && z > 0 && x < last && z < last) continue;
            domain.hold[i] = Rim;
            domain.outlet[i] = x == last && river;
            if (x == 0 && z == middle) domain.inflow[domain.index(1, middle)] = float(spec.riverFlow);
        }
    }
    domain.settledFrom = domain.settledTo = domain.ground;
    refreshBeds(domain, spec);
    return domain;
}

void stepBlock(Domain& domain, FlowScratch& scratch, const BlockSpec& spec) {
    const float dt = float(domain.step);
    for (size_t i = 0; i < domain.cells(); ++i) {
        domain.surface[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        domain.warmthNow[i] = domain.warmth[i] - float(LapseRate) * std::max(0.0f, domain.surface[i] - float(domain.seaLevel));
        domain.runoff[i] = runoffOf(domain.rain[i], domain.warmthNow[i]);
    }
    if (spec.led) raise(domain, scratch);
    else if (spec.uplift != 0) {
        const int middle = domain.size / 2;
        for (int z = 0; z < domain.size; ++z)
            for (int x = 0; x < domain.size; ++x) {
                const size_t i = domain.index(x, z);
                if (domain.hold[i] == Rim && std::abs(z - middle) <= spec.floorHalf) continue;
                domain.ground.rock[i] += float(spec.uplift) * dt;
                domain.ground.datum[i] += float(spec.uplift) * dt;
            }
    }
    refreshBeds(domain, spec);
    shapeSlopes(domain, scratch);
    if (spec.river) {
        floodLevels(domain, scratch);
        routeWater(domain, scratch);
        cutAndFill(domain, scratch);
        settleDebris(domain, scratch);
    }
    domain.time += domain.step;
    ++domain.taken;
}

Domain replayBlock(const BlockSpec& spec) {
    Domain domain = blockDomain(spec);
    FlowScratch scratch;
    const int steps = int(std::llround((domain.endTime - domain.startTime) / domain.step));
    for (int step = 0; step < steps; ++step) stepBlock(domain, scratch, spec);
    // The water of the surface that is handed back, not of the one the last step began with.
    floodLevels(domain, scratch);
    if (spec.river) routeWater(domain, scratch);
    return domain;
}

double tallestFace(const Domain& domain, double rise) {
    double tallest = 0;
    const float drop = float(rise * domain.cell);
    for (int x = Halo; x < Halo + TileCells; x += 4) {
        // From the river outwards on both sides; a face is an unbroken run of steep steps.
        for (const int sign : {-1, 1}) {
            double run = 0;
            for (int z = domain.size / 2; z > 1 && z < domain.size - 2; z += sign) {
                const size_t here = domain.index(x, z), next = domain.index(x, z + sign);
                const float step = domain.ground.rock[next] + domain.sedimentAt(next) - domain.ground.rock[here] - domain.sedimentAt(here);
                run = step > drop ? run + step : 0.0;
                tallest = std::max(tallest, run);
            }
        }
    }
    return tallest;
}

} // namespace derivean::sim
