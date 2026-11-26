#include "landscape/fn/Build.hpp"

#include "ground/fn/Stages.hpp"
#include "landscape/fn/Forcing.hpp"
#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {
namespace {

constexpr int StrataStride[LevelCount] = {1, 2, 8, 8, 64};          // cells between strata nodes, by level
// m between the seeded fracture zones of the ground level. The coarser levels read theirs from the
// rock-mass structure instead (Structure.cpp).
constexpr double FractureSpacing[LevelCount] = {0, 0, 0, 0, 60.0};
// Cells over which the seeded rock quality varies. Its finest part must stay wider than the valleys
// a level draws, or every cell erodes at a pace of its own and the relief is that of the pattern.
constexpr double QualitySpan[LevelCount] = {0, 48.0, 48.0, 48.0, 14.0};

void allocate(Domain& domain) {
    const size_t cells = domain.cells();
    domain.ground.resize(cells);
    domain.hold.assign(cells, Free);
    domain.outlet.assign(cells, 0);
    domain.fold.assign(cells, 0.0f);
    domain.unit.assign(cells, UnitCache{});
    for (auto* field : {&domain.tectonic, &domain.residual, &domain.weakness, &domain.quality, &domain.rebound, &domain.trend, &domain.uplift, &domain.rain,
                        &domain.warmth, &domain.warmthNow, &domain.rainBase, &domain.runoff, &domain.inflow, &domain.inflowLoad, &domain.settledFlow, &domain.settledLoad, &domain.windX, &domain.windZ, &domain.rimSlope, &domain.rimFloor, &domain.surface,
                        &domain.level, &domain.discharge, &domain.ice, &domain.iceAge, &domain.erosionRate, &domain.depositionRate,
                        &domain.eroded, &domain.deposited, &domain.uplifted, &domain.shed})
        field->assign(cells, 0.0f);
    domain.order.reserve(cells);
    domain.rank.assign(cells, 0);
    domain.spill.assign(cells, -1);
}

// Forcing varies over tens of kilometres, so it is read at the strata nodes and interpolated;
// only the fracture pattern, which a level resolves cell by cell, is evaluated everywhere.
void readForcing(Domain& domain, const Setting& setting) {
    const int stride = domain.stride, nodes = domain.nodes;
    std::vector<Forcing> node(size_t(nodes) * size_t(nodes));
    for (int nz = 0; nz < nodes; ++nz)
        for (int nx = 0; nx < nodes; ++nx)
            node[size_t(nz) * size_t(nodes) + size_t(nx)] = forcingAt(setting, {domain.origin.x + (nx * stride + 0.5) * domain.cell,
                                                                              domain.origin.z + (nz * stride + 0.5) * domain.cell});
    domain.strata.resize(node.size());
    for (size_t i = 0; i < node.size(); ++i) domain.strata[i] = node[i].strata;

    const double spacing = FractureSpacing[size_t(domain.key.level)];
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const int nx = std::min(x / stride, nodes - 2), nz = std::min(z / stride, nodes - 2);
            const double tx = double(x - nx * stride) / stride, tz = double(z - nz * stride) / stride;
            const auto blend = [&](float Forcing::*member) {
                const auto at = [&](int ox, int oz) { return double(node[size_t(nz + oz) * size_t(nodes) + size_t(nx + ox)].*member); };
                return float(mix(mix(at(0, 0), at(1, 0), tx), mix(at(0, 1), at(1, 1), tx), tz));
            };
            domain.tectonic[i] = blend(&Forcing::tectonic);
            domain.residual[i] = blend(&Forcing::residual);
            domain.rain[i] = blend(&Forcing::rain);
            domain.warmth[i] = blend(&Forcing::warmth);
            domain.weakness[i] = blend(&Forcing::weakness);
            domain.windX[i] = blend(&Forcing::windX);
            domain.windZ[i] = blend(&Forcing::windZ);
            const StrataColumn& column = domain.strataOf(i);
            const Vec2 here = domain.center(x, z);
            const Vec2 nodeCenter{domain.origin.x + ((x / stride) * stride + 0.5) * domain.cell, domain.origin.z + ((z / stride) * stride + 0.5) * domain.cell};
            const Vec2 material = column.material + (here - nodeCenter);
            domain.fold[i] = foldAt(setting.seed, column, material);
            if (spacing > 0) domain.weakness[i] = std::max(domain.weakness[i], fractureAt(setting.seed, material, column.fabric, spacing));
            // Below a site cell the differences in rock are its partings and joints, not a seeded factor.
            domain.quality[i] = domain.key.level == Level::Ground ? 1.0f : qualityAt(setting.seed, material, QualitySpan[size_t(domain.key.level)] * domain.cell);
        }
    }
}

// Cells of the halo that a finished neighbour owns take its result and stay fixed.
void adoptNeighbours(Domain& domain, const TileLookup& lookup) {
    domain.settledFrom = domain.ground;
    domain.settledTo = domain.ground;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            const TileKey other{domain.key.level, domain.key.x + dx, domain.key.z + dz};
            if ((dx || dz) && passOf(other) < passOf(domain.key)) domain.finished[(dz + 1) * 3 + dx + 1] = lookup(other);
        }
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const int tx = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, tz = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
            const Tile* tile = domain.finished[(tz + 1) * 3 + tx + 1];
            if (!tile) continue;
            const size_t from = size_t(z - Halo - tz * TileCells) * TileCells + size_t(x - Halo - tx * TileCells), to = domain.index(x, z);
            domain.settledTo.rock[to] = tile->now.rock[from];
            domain.settledTo.datum[to] = tile->now.datum[from];
            for (int c = 0; c < SedimentClasses; ++c) domain.settledTo.sediment[c][to] = tile->now.sediment[c][from];
            domain.settledFlow[to] = tile->discharge[from];
            domain.settledLoad[to] = tile->load[from];
            // The plate under this domain also carries what its finished neighbour lost and gained.
            domain.erosionRate[to] = tile->erosionRate[from];
            domain.depositionRate[to] = tile->depositionRate[from];
            domain.hold[to] = Settled;
        }
    }
}

// The outer ring is where water leaves. In a region a free rim cell keeps cutting at the slope it
// starts with, down to the graded river profile the planet allows. A finer level starts on its
// parent's surface, and its rim stays there: that is the base level its valleys grade to. A rim
// cell inside a finished neighbour follows that neighbour.
void openRim(Domain& domain) {
    const int last = domain.size - 1;
    const bool led = domain.key.level != Level::Region;
    for (int z = 0; z <= last; ++z) {
        for (int x = 0; x <= last; ++x) {
            if (x > 0 && z > 0 && x < last && z < last) continue;
            const size_t i = domain.index(x, z);
            const size_t inner = domain.index(std::clamp(x, 1, last - 1), std::clamp(z, 1, last - 1));
            const float drop = domain.ground.rock[inner] + domain.sedimentAt(inner) - domain.ground.rock[i] - domain.sedimentAt(i);
            const bool follows = led || domain.hold[i] == Settled;
            domain.rimSlope[i] = follows ? 0.0f : std::max(1e-4f, drop / float(domain.cell));
            if (follows) domain.tectonic[i] = domain.residual[i] = 0;
            domain.hold[i] = Rim;
        }
    }
}

// Interpolating a parent whose valleys are one cell wide leaves a chain of hollows along every
// diagonal reach, and a finer level that is held to that surface would keep them as lakes. Hollows
// shallower than the level can tell from an artefact are levelled before the settling starts; a
// deeper basin is a real feature of the parent and stays.
void levelHollows(Domain& domain) {
    constexpr float Artefact[LevelCount] = {0, 0, 45.0f, 6.0f, 0.4f};
    FlowScratch scratch;
    floodLevels(domain, scratch);
    const float limit = Artefact[size_t(domain.key.level)];
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.hold[i] != Free || domain.surface[i] < domain.seaLevel) continue;
        const float fill = std::min(limit, domain.level[i] - domain.surface[i]);
        if (fill <= 0) continue;
        domain.ground.rock[i] += fill;
    }
    for (size_t i = 0; i < domain.cells(); ++i) domain.trend[i] = domain.ground.rock[i] + domain.sedimentAt(i);
}

} // namespace

std::vector<TileKey> dependencies(TileKey key) {
    std::vector<TileKey> needed;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            const TileKey other{key.level, key.x + dx, key.z + dz};
            if ((dx || dz) && passOf(other) < passOf(key)) needed.push_back(other);
        }
    if (key.level == Level::Region) return needed;
    const int margin = parentMargin(key.level);
    const auto low = [&](int index) { return int(std::floor((index * (TileCells / Refinement) - margin) / double(TileCells))); };
    const auto high = [&](int index) { return int(std::floor(((index + 1) * (TileCells / Refinement) + margin - 1) / double(TileCells))); };
    for (int pz = low(key.z); pz <= high(key.z); ++pz)
        for (int px = low(key.x); px <= high(key.x); ++px) needed.push_back({Level(int(key.level) - 1), px, pz});
    return needed;
}

Domain buildDomain(const Setting& setting, TileKey key, const TileLookup& lookup) {
    const LevelSpec& spec = specOf(key.level);
    Domain domain;
    domain.key = key;
    // A ground tile is the last link before the player sees ground, so its own work is spread over
    // the pool; the coarser levels are built whole tiles side by side.
    domain.workers = key.level == Level::Ground ? setting.workers : nullptr;
    domain.cell = spec.cell;
    domain.step = spec.step;
    domain.origin = {key.x * tileSpan(key.level) - Halo * spec.cell, key.z * tileSpan(key.level) - Halo * spec.cell};
    domain.time = domain.startTime = windowAt(key.level, setting.age).start;
    domain.endTime = windowAt(key.level, setting.age).end;
    domain.seaLevel = seaLevelAt(setting, domain.time);
    domain.stride = StrataStride[size_t(key.level)];
    domain.nodes = (domain.size + domain.stride - 1) / domain.stride + 1;
    allocate(domain);
    readForcing(domain, setting);
    if (key.level == Level::Region) seedFromPlanet(domain, setting);
    else seedFromParent(domain, setting, lookup);
    domain.rainBase = domain.rain;
    adoptNeighbours(domain, lookup);
    openRim(domain);
    beginVolcanic(domain, setting, lookup);
    if (key.level != Level::Ground) readStructure(domain, setting);
    if (key.level != Level::Region) levelHollows(domain);
    if (key.level == Level::Ground) beginGround(domain, setting);
    return domain;
}

} // namespace derivean::sim
