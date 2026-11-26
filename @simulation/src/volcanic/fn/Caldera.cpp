#include "volcanic/fn/Volcanic.hpp"

#include "volcanic/fn/Emplace.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace derivean::sim {
namespace {

constexpr double RingCells = 2.0;   // cells beyond the rim over which the ground around a pit is read

// m from the centre at which the roof's share has fallen to nothing on cells of this size
// (fn/Collapse.cpp).
double rimOf(const Caldera& caldera, double cell) {
    const double radius = calderaRadius(caldera.volume, caldera.magma);
    return radius + std::max(0.5 * cell, 0.06 * radius);
}

double surfaceAt(const Domain& domain, size_t i) { return double(domain.ground.rock[i] + domain.sedimentAt(i)); }

// How deep the pit stands, as a volume over its block: how far the surface over the founder's
// cells lies below where it stood before the step, by each cell's share of the block, less the same
// over a ring of ground just beyond the rim. A cone built later inside it, or a wall slumped into
// it, takes its share of the volume and no more; what the edifice around it gained or lost
// meanwhile, the ring takes out; so do the blocks of `later` calderas that went down over it since,
// which are their pits, not this one's.
float pitOf(const Domain& domain, const Caldera& caldera, const std::vector<Caldera>& later = {}) {
    Emplacement box;
    box.resize(caldera.side, caldera.side);
    box.cell = domain.cell;
    box.origin = domain.origin + Vec2{caldera.x * domain.cell, caldera.z * domain.cell};
    const std::vector<float> share = founder(box, caldera.site, caldera.volume, caldera.magma);
    std::vector<float> since(share.size(), 0.0f);
    const double span = caldera.side * domain.cell;
    for (const Caldera& other : later) {
        if (other.time <= caldera.time || length(other.site - caldera.site) > span + calderaRadius(other.volume, other.magma) + 2 * domain.cell) continue;
        const std::vector<float> drop = founder(box, other.site, other.volume, other.magma);
        for (size_t k = 0; k < since.size(); ++k) since[k] += drop[k];
    }
    const double rim = rimOf(caldera, domain.cell), outer = rim + RingCells * domain.cell;
    double inside = 0, weight = 0, ring = 0, around = 0;
    for (int z = 0; z < caldera.side; ++z) {
        for (int x = 0; x < caldera.side; ++x) {
            const int dx = caldera.x + x, dz = caldera.z + z;
            if (dx < 0 || dz < 0 || dx >= domain.size || dz >= domain.size) continue;
            const size_t k = box.index(x, z);
            const double lowered = caldera.before[k] - surfaceAt(domain, domain.index(dx, dz)) - since[k];
            const double distance = length(box.center(x, z) - caldera.site);
            if (share[k] > 0) inside += share[k] * lowered, weight += share[k];
            else if (distance >= rim && distance <= outer) ring += lowered, ++around;
        }
    }
    if (weight <= 0) return 0;
    return float(inside / weight - (around > 0 ? ring / around : 0.0));
}

} // namespace

Caldera calderaOf(const Domain& domain, const Vent& vent, const Episode& episode) {
    Caldera caldera{vent.id, episode.site, episode.paroxysm, vent.magma, domain.time + domain.step};
    const double reach = rimOf(caldera, domain.cell) + (RingCells + 1) * domain.cell;
    caldera.x = int(std::floor((caldera.site.x - reach - domain.origin.x) / domain.cell));
    caldera.z = int(std::floor((caldera.site.z - reach - domain.origin.z) / domain.cell));
    caldera.side = int(std::ceil(2 * reach / domain.cell)) + 1;
    caldera.before.assign(size_t(caldera.side) * size_t(caldera.side), 0.0f);
    for (int z = 0; z < caldera.side; ++z) {
        for (int x = 0; x < caldera.side; ++x) {
            const int dx = caldera.x + x, dz = caldera.z + z;
            if (dx >= 0 && dz >= 0 && dx < domain.size && dz < domain.size) caldera.before[size_t(z) * size_t(caldera.side) + size_t(x)] = float(surfaceAt(domain, domain.index(dx, dz)));
        }
    }
    return caldera;
}

// Each caldera keeps its own record, a later one nested in an earlier one too: each was a block of
// its own volume, and what is left of each is read over its own block.
void recordCalderas(Domain& domain, std::vector<Caldera>&& cut) {
    for (Caldera& caldera : cut) {
        caldera.cut = pitOf(domain, caldera);
        domain.volcanic.calderas.push_back(std::move(caldera));
    }
}

std::vector<Caldera> finishCalderas(Domain& domain) {
    std::vector<Caldera> own;
    const double lowX = domain.origin.x + Halo * domain.cell, lowZ = domain.origin.z + Halo * domain.cell, span = TileCells * domain.cell;
    for (const Caldera& caldera : domain.volcanic.calderas) {
        const Vec2 site = caldera.site;
        if (site.x < lowX || site.z < lowZ || site.x >= lowX + span || site.z >= lowZ + span) continue;
        Caldera kept{caldera.vent, site, caldera.volume, caldera.magma, caldera.time, caldera.cut};
        kept.kept = caldera.cut > 1.0f ? std::clamp(pitOf(domain, caldera, domain.volcanic.calderas) / caldera.cut, 0.0f, 1.0f) : 0.0f;
        own.push_back(kept);
    }
    return own;
}

// A region cuts a caldera into cells a kilometre wide, and a local level is seeded with a spline of
// them: its wall would be a ramp kilometres wide. The local level cuts the ring fault again at its
// own cells in place of that smoothed pit, by the share of the region's pit its record says is
// still there: a pit filled since, or worn down, is not dug out again. Mass is kept, since both cuts
// hold the same volume. Pile and layering go down together, as when the roof first went down; the
// open edge is held to the seeded ground, so that goes down with it, except where a finished
// neighbour, which made its own cut, holds it.
// Every local tile the cut reaches must find the record, in the parent tile that holds the site:
// a caldera cut near a parent tile's edge into a local tile not built from that parent is left as
// the region cut it, by every tile alike.
void sharpenCalderas(Domain& domain, const TileLookup& lookup) {
    if (domain.key.level != Level::Local) return;
    const Level parentLevel = Level(int(domain.key.level) - 1);
    const double parentCell = domain.cell * Refinement, span = domain.size * domain.cell;
    const double tileSpan = TileCells * domain.cell, halo = Halo * domain.cell;
    // Whether every local tile whose cells, halo included, the cut reaches is built from `owner`.
    const auto seenByAll = [&](TileKey owner, Vec2 site, double outer) {
        const int lowX = int(std::floor((site.x - outer - halo) / tileSpan)), highX = int(std::floor((site.x + outer + halo) / tileSpan));
        const int lowZ = int(std::floor((site.z - outer - halo) / tileSpan)), highZ = int(std::floor((site.z + outer + halo) / tileSpan));
        for (int z = lowZ; z <= highZ; ++z) {
            for (int x = lowX; x <= highX; ++x) {
                const std::vector<TileKey> needed = dependencies({domain.key.level, x, z});
                if (std::find(needed.begin(), needed.end(), owner) == needed.end()) return false;
            }
        }
        return true;
    };
    Emplacement fine;
    for (const TileKey key : dependencies(domain.key)) {
        if (key.level != parentLevel) continue;
        const Tile* parent = lookup(key);
        if (!parent || !parent->volcanic) continue;
        for (const Caldera& caldera : parent->volcanic->calderas) {
            // The spline spreads the region's pit two parent cells beyond its rim.
            const double outer = rimOf(caldera, parentCell) + 2.5 * parentCell;
            const Vec2 site = caldera.site;
            if (caldera.kept <= 0 || site.x + outer < domain.origin.x || site.z + outer < domain.origin.z || site.x - outer > domain.origin.x + span || site.z - outer > domain.origin.z + span) continue;
            if (!seenByAll(key, site, outer)) continue;
            // The region's cut, on the region's own cells around it, carried down by the spline the
            // local level is seeded with (landscape/fn/Seed.cpp: cubic weights at x / cell − 0.5).
            const int half = int(std::ceil(outer / parentCell)) + 2;
            const int64_t centreX = int64_t(std::floor(site.x / parentCell)), centreZ = int64_t(std::floor(site.z / parentCell));
            Emplacement coarse;
            coarse.resize(2 * half + 1, 2 * half + 1);
            coarse.cell = parentCell;
            coarse.origin = {double(centreX - half) * parentCell, double(centreZ - half) * parentCell};
            const std::vector<float> wide = founder(coarse, site, caldera.volume, caldera.magma);
            const auto smoothed = [&](Vec2 at) {
                const double fx = at.x / parentCell - 0.5, fz = at.z / parentCell - 0.5;
                const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
                double wx[4], wz[4], sum = 0;
                splineWeights(fx - double(ix), wx);
                splineWeights(fz - double(iz), wz);
                for (int j = 0; j < 4; ++j) {
                    for (int k = 0; k < 4; ++k) {
                        const int64_t x = ix - 1 + k - (centreX - half), z = iz - 1 + j - (centreZ - half);
                        if (x >= 0 && z >= 0 && x < coarse.width && z < coarse.height) sum += wx[k] * wz[j] * wide[coarse.index(int(x), int(z))];
                    }
                }
                return sum;
            };
            if (!fine.cells()) {
                fine.resize(domain.size, domain.size);
                fine.cell = domain.cell;
                fine.origin = domain.origin;
            }
            const std::vector<float> sharp = founder(fine, site, caldera.volume, caldera.magma);
            const int x0 = std::max(0, int((site.x - outer - domain.origin.x) / domain.cell)), x1 = std::min(domain.size - 1, int((site.x + outer - domain.origin.x) / domain.cell));
            const int z0 = std::max(0, int((site.z - outer - domain.origin.z) / domain.cell)), z1 = std::min(domain.size - 1, int((site.z + outer - domain.origin.z) / domain.cell));
            for (int z = z0; z <= z1; ++z) {
                for (int x = x0; x <= x1; ++x) {
                    const size_t i = domain.index(x, z);
                    // A finished neighbour's cells, the open edge over them included, hold its own cut.
                    const int sideX = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, sideZ = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
                    if (domain.hold[i] == Settled || domain.finished[(sideZ + 1) * 3 + sideX + 1]) continue;
                    const float change = caldera.kept * float(sharp[i] - smoothed(domain.center(x, z)));
                    for (Ground* ground : {&domain.ground, &domain.settledFrom, &domain.settledTo}) {
                        if (ground != &domain.ground && domain.hold[i] != Rim) continue;
                        ground->rock[i] -= change;
                        ground->datum[i] -= change;
                    }
                }
            }
        }
    }
}

} // namespace derivean::sim
