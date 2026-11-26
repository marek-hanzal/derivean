#include "ground/fn/Stages.hpp"

#include "climate/fn/Seasons.hpp"
#include "climate/type/Climate.hpp"
#include "ground/constant/Ground.hpp"
#include "ground/fn/Partings.hpp"

namespace derivean::sim {
namespace {

constexpr int WidestHalf = 24;           // cells; a river wider than this is resolved by the site level itself
constexpr float DeepMantle = 3.0f;       // m of loose cover under which rock is out of reach of what works along its joints

} // namespace

void beginGround(Domain& domain, const Setting& setting) {
    const size_t cells = domain.cells();
    domain.bed.assign(cells, Bed{1, 0, Material::Bedrock});
    domain.laidDown.assign(cells, 0.0);
    domain.stream.assign(cells, 0.0f);
    domain.ore.resize(domain.strata.size());
    for (size_t node = 0; node < domain.strata.size(); ++node) domain.ore[node] = oreFieldAt(setting.seed, domain.strata[node], domain.strata[node].material);
    // Where the parent's river enters over a finished neighbour's cells that neighbour already
    // carries it, wherever exactly it runs there; entered again it would count twice.
    for (size_t i = 0; i < cells; ++i)
        if (domain.hold[i] == Settled) domain.inflow[i] = domain.inflowLoad[i] = 0;
    const size_t middle = domain.index(domain.size / 2, domain.size / 2);
    const float height = std::max(0.0f, domain.ground.rock[middle] + domain.sedimentAt(middle) - float(domain.seaLevel));
    domain.halfRange = float(zonalClimate(setting.seed, domain.origin.z + 0.5 * domain.size * domain.cell, setting.age).seasonal);
    domain.frost = float(frostCracking(domain.warmth[middle] - LapseRate * height, domain.halfRange));
}

void refreshPartings(Domain& domain, uint64_t seed) {
    // A hand-built domain of one rock carries no layering to read partings from.
    if (domain.bed.empty()) return;
    // Every cell by itself, so rows are worked side by side. The rock mass of the bed last met beside
    // a strata node serves every cell of the same rows that meets the same bed there.
    forRows(domain, domain.size, [&](int begin, int end) {
    size_t massNode = SIZE_MAX;
    Bed massBed{0, -1, Material::Bedrock};
    double massLaid = 0;
    RockMass mass;
    for (size_t i = size_t(begin) * size_t(domain.size); i < size_t(end) * size_t(domain.size); ++i) {
        const float depth = std::max(0.0f, domain.ground.datum[i] - domain.ground.rock[i]);
        UnitCache& cache = domain.unit[i];
        if (depth >= cache.top && depth < cache.bottom) continue;
        Bed& bed = domain.bed[i];
        const Vec2 material = domain.materialOf(i);
        if (depth < bed.top || depth >= bed.bottom) {
            const StrataColumn column = domain.strataAtCell(i);
            const StrataUnit unit = unitAt(column, depth, domain.fold[i]);
            bed = bedAt(seed, column, unit, depth, material);
            domain.laidDown[i] = unit.laidDown;
        }
        // A vein or a pipe that cuts the beds here is the rock the cell meets. It is not layered,
        // so it is asked again a quarter of a metre further down.
        const size_t node = size_t(int(i / size_t(domain.size)) / domain.stride) * size_t(domain.nodes) + size_t(int(i % size_t(domain.size)) / domain.stride);
        const OreField& ore = domain.ore[node];
        const bool cut = ore.veining != 0 || ore.pipeRadius != 0;
        if (cut) {
            const Material body = cuttingRock(seed, ore, material, depth, domain.laidDown[i]);
            if (body != Material::Air) {
                cache = {depth, depth + 0.25f, traitsOf(body)};
                continue;
            }
        }
        // Joints are lines of weakness where rock is at or near the surface; under a deep mantle nothing reads them.
        float joint = 0;
        if (domain.sedimentAt(i) < DeepMantle) {
            if (node != massNode || bed.rock != massBed.rock || canonicalThickness(bed) != massBed.bottom || domain.laidDown[i] != massLaid) {
                massNode = node, massBed = {0, canonicalThickness(bed), bed.rock}, massLaid = domain.laidDown[i];
                mass = massOf(seed, domain.strata[node], bed, massLaid);
            }
            joint = fractureShare(seed, mass, material, depth, float(domain.cell));
        }
        const Parting parting = partingAt(seed, bed, depth);
        cache = {cut ? depth : std::min(parting.top, depth), cut ? std::min(parting.bottom, depth + 0.25f) : parting.bottom, partingTraits(parting, joint, domain.frost)};
    }
    });
}

void widenChannels(Domain& domain) {
    const int size = domain.size;
    domain.stream = domain.discharge;
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const float water = domain.discharge[i];
            const float half = 0.5f * streamWidth(water) / float(domain.cell);
            if (half < 1.0f || domain.level[i] > domain.surface[i] + 1e-3f) continue;
            const int reach = std::min(WidestHalf, int(half));
            for (int dz = -reach; dz <= reach; ++dz) {
                const int nz = z + dz;
                if (nz < 0 || nz >= size) continue;
                const int span = int(std::sqrt(float(reach * reach - dz * dz)));
                for (int nx = std::max(0, x - span); nx <= std::min(size - 1, x + span); ++nx) {
                    float& worked = domain.stream[domain.index(nx, nz)];
                    worked = std::max(worked, water);
                }
            }
        }
    }
}

} // namespace derivean::sim
