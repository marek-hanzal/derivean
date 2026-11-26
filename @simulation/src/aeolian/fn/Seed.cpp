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
    wind.workers = setting.workers;
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
    record.labels.resize(size_t(TileCells) * TileCells);
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            const float fines = std::max(0.0f, domain.ground.sediment[Fines][from]);
            record.labels.blown[to] = std::min(wind.labels.blown[from], std::max(0.0f, domain.ground.sediment[Sand][from]));
            record.labels.lag[to] = std::min(wind.labels.lag[from], std::max(0.0f, domain.ground.sediment[Coarse][from]));
            record.labels.salt[to] = std::min(wind.labels.salt[from], fines);
            record.labels.loess[to] = std::min(wind.labels.loess[from], fines - record.labels.salt[to]);
        }
    }
    return record;
}

} // namespace derivean::sim
