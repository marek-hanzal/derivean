#include "aeolian/fn/Wind.hpp"

#include "aeolian/fn/Regime.hpp"
#include "climate/type/Climate.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

constexpr int NodeStride = 64;   // cells between the points a Region domain reads its regime at

using Label = std::vector<float> WindLabels::*;
constexpr Label Labels[4] = {&WindLabels::blown, &WindLabels::loess, &WindLabels::lag, &WindLabels::salt};

// A label of the parent level at a world position, between the four parent cells around it. Parent
// tiles the builder was not given count as bare.
float parentLabel(const Domain& domain, const TileLookup& lookup, Label label, Vec2 position) {
    const Level level = Level(int(domain.key.level) - 1);
    const double cell = specOf(level).cell, fx = position.x / cell - 0.5, fz = position.z / cell - 0.5;
    const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
    const double tx = fx - double(ix), tz = fz - double(iz);
    double sum = 0;
    for (int corner = 0; corner < 4; ++corner) {
        const int64_t gx = ix + (corner & 1), gz = iz + (corner >> 1);
        const TileKey key{level, int32_t(std::floor(gx / double(TileCells))), int32_t(std::floor(gz / double(TileCells)))};
        const Tile* tile = lookup(key);
        if (!tile || (tile->aeolian.labels.*label).empty()) continue;
        const size_t at = size_t(gz - int64_t(key.z) * TileCells) * TileCells + size_t(gx - int64_t(key.x) * TileCells);
        sum += ((corner & 1) ? tx : 1 - tx) * ((corner >> 1) ? tz : 1 - tz) * (tile->aeolian.labels.*label)[at];
    }
    return float(sum);
}

// The drift of a Region domain, between regimes read every NodeStride cells.
void readDrift(Domain& domain, const Setting& setting) {
    WindField& wind = domain.aeolian;
    const int nodes = domain.size / NodeStride + 2;
    std::vector<WindRegime> node(size_t(nodes) * size_t(nodes));
    for (int nz = 0; nz < nodes; ++nz)
        for (int nx = 0; nx < nodes; ++nx)
            node[size_t(nz) * size_t(nodes) + size_t(nx)] = regimeAt(setting.seed, {domain.origin.x + nx * NodeStride * domain.cell, domain.origin.z + nz * NodeStride * domain.cell}, setting.age);
    for (auto* field : {&wind.driftX, &wind.driftZ, &wind.potential}) field->resize(domain.cells());
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const double fx = (x + 0.5) / NodeStride, fz = (z + 0.5) / NodeStride;
            const int nx = int(fx), nz = int(fz);
            const double tx = fx - nx, tz = fz - nz;
            const auto blend = [&](auto&& pick) {
                const auto at = [&](int ox, int oz) { return pick(node[size_t(nz + oz) * size_t(nodes) + size_t(nx + ox)]); };
                return float(mix(mix(at(0, 0), at(1, 0), tx), mix(at(0, 1), at(1, 1), tx), tz));
            };
            const size_t i = domain.index(x, z);
            wind.driftX[i] = blend([](const WindRegime& r) { return r.resultant.x; });
            wind.driftZ[i] = blend([](const WindRegime& r) { return r.resultant.z; });
            wind.potential[i] = blend([](const WindRegime& r) { return r.potential; });
        }
    }
}

// Near an edge shared with a finished neighbour the ground fades into what that neighbour's replay
// left for the same cells (`blendSeams`, landscape/fn/Extract.cpp). A label fades the same way and
// with the same weights, or it keeps this tile's value over sediment that is now mostly the
// neighbour's and steps at the edge. The weight must stay equal to the one there.
float seamWeight(int distance) { return float(smoothstep(0.0, 1.0, (distance + 0.5) / Apron)); }

float seamLabel(const Domain& domain, Label label, int x, int z, float own) {
    const int sideX = x < Apron ? -1 : x >= TileCells - Apron ? 1 : 0, sideZ = z < Apron ? -1 : z >= TileCells - Apron ? 1 : 0;
    const Tile* alongX = sideX ? domain.finished[4 + sideX] : nullptr;
    const Tile* alongZ = sideZ ? domain.finished[4 + 3 * sideZ] : nullptr;
    if (!alongX && !alongZ) return own;
    const Tile* diagonal = alongX && alongZ ? domain.finished[4 + sideX + 3 * sideZ] : nullptr;
    const float wx = alongX ? seamWeight(sideX < 0 ? x : TileCells - 1 - x) : 1.0f;
    const float wz = alongZ ? seamWeight(sideZ < 0 ? z : TileCells - 1 - z) : 1.0f;
    // This cell in the frame of a neighbour's wide square; a neighbour without labels counts as bare.
    const auto there = [&](const Tile* tile, int dx, int dz) {
        const std::vector<float>& wide = tile->aeolian.wide.*label;
        return wide.empty() ? 0.0f : wide[size_t(z - dz * TileCells + Apron) * WideCells + size_t(x - dx * TileCells + Apron)];
    };
    const float fromX = alongX ? there(alongX, sideX, 0) : own, fromZ = alongZ ? there(alongZ, 0, sideZ) : own;
    const float fromBoth = diagonal ? there(diagonal, sideX, sideZ) : 0.5f * (fromX + fromZ);
    return own * wx * wz + fromX * (1 - wx) * wz + fromZ * wx * (1 - wz) + fromBoth * (1 - wx) * (1 - wz);
}

} // namespace

void allocateWind(Domain& domain) {
    domain.aeolian.labels.resize(domain.cells());
    domain.aeolian.parked.assign(domain.cells(), 0.0f);
    domain.aeolian.parkedLoess.assign(domain.cells(), 0.0f);
}

void beginWind(Domain& domain, const Setting& setting, const TileLookup& lookup) {
    if (domain.key.level == Level::Ground) return;
    WindField& wind = domain.aeolian;
    allocateWind(domain);
    const Vec2 middle = domain.center(domain.size / 2, domain.size / 2);
    wind.regime = regimeAt(setting.seed, middle, setting.age);
    wind.halfRange = float(zonalClimate(setting.seed, planetPoint(setting.seed, middle).z, setting.age).seasonal);
    if (domain.key.level == Level::Region) readDrift(domain, setting), readFarDust(domain, setting);

    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.key.level != Level::Region)
                for (const Label label : Labels) (wind.labels.*label)[i] = parentLabel(domain, lookup, label, domain.center(x, z));
            float* sand[3] = {&domain.ground.sediment[Sand][i], &domain.settledFrom.sediment[Sand][i], &domain.settledTo.sediment[Sand][i]};
            const bool parks = domain.key.level != Level::Region;
            // The sand the wind laid at the parent's scale is kept off the ground the tile settles on...
            if (parks)
                for (int k = 0; k < 2; ++k) *sand[k] -= std::min(wind.labels.blown[i], *sand[k]);
            const int tx = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, tz = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
            const Tile* tile = domain.finished[(tz + 1) * 3 + tx + 1];
            if (tile && !tile->aeolian.labels.blown.empty()) {
                const size_t from = size_t(z - Halo - tz * TileCells) * TileCells + size_t(x - Halo - tx * TileCells);
                for (const Label label : Labels) (wind.labels.*label)[i] = (tile->aeolian.labels.*label)[from];
            }
            // ...and so is what a finished neighbour's wind left on its cells.
            if (parks) {
                wind.parked[i] = std::min(wind.labels.blown[i], *sand[2]);
                *sand[2] -= wind.parked[i];
            }
        }
    }
}

WindRecord windRecordOf(const Domain& domain) {
    WindRecord record;
    const WindField& wind = domain.aeolian;
    record.regime = wind.regime, record.span = wind.span, record.rounds = wind.rounds, record.ledger = wind.ledger;
    if (wind.labels.blown.size() != domain.cells()) return record;
    // Every label is read off against the cover the cell is left with: the last step's
    // `settleDebris` moves loose cover after the wind has labelled it.
    const auto clipped = [&](size_t i, float blown, float lag, float salt, float loess, WindLabels& into, size_t to) {
        const float fines = std::max(0.0f, domain.ground.sediment[Fines][i]);
        into.blown[to] = std::min(blown, std::max(0.0f, domain.ground.sediment[Sand][i]));
        into.lag[to] = std::min(lag, std::max(0.0f, domain.ground.sediment[Coarse][i]));
        into.salt[to] = std::min(salt, fines);
        into.loess[to] = std::min(loess, fines - into.salt[to]);
    };
    record.wide.resize(size_t(WideCells) * WideCells);
    for (int z = 0; z < WideCells; ++z)
        for (int x = 0; x < WideCells; ++x) {
            const size_t i = domain.index(x + Halo - Apron, z + Halo - Apron);
            clipped(i, wind.labels.blown[i], wind.labels.lag[i], wind.labels.salt[i], wind.labels.loess[i], record.wide, size_t(z) * WideCells + size_t(x));
        }
    // The tile's own cells are read off after `settleSeams` blended their ground: their own labels
    // are clipped, faded into the neighbours' at the seam, and clipped again to the blended cover.
    record.labels.resize(size_t(TileCells) * TileCells);
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            WindLabels& own = record.labels;
            clipped(from, wind.labels.blown[from], wind.labels.lag[from], wind.labels.salt[from], wind.labels.loess[from], own, to);
            const auto label = [&](Label which) { return seamLabel(domain, which, x, z, (own.*which)[to]); };
            clipped(from, label(&WindLabels::blown), label(&WindLabels::lag), label(&WindLabels::salt), label(&WindLabels::loess), own, to);
        }
    }
    return record;
}

} // namespace derivean::sim
