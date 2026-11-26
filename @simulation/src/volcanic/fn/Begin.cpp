#include "volcanic/fn/Volcanic.hpp"

#include "volcanic/fn/Source.hpp"

namespace derivean::sim {
namespace {

constexpr double RegionReach = 80000.0;   // m beyond a region's domain within which a vent's ash still reaches it
// m beyond a finer domain within which a vent's conduits are kept: at the local level the reach of
// its small eruptions around a conduit, below it only the conduit's own width.
constexpr double FinerReach[LevelCount] = {0, 0, 9000.0, 700.0, 700.0};

// The parent tiles under a domain, by the parent's global cell index.
struct Parents {
    const TileLookup& lookup;
    Level level;

    const Tile* at(int64_t gx, int64_t gz, size_t& cell) const {
        const int32_t tx = int32_t(std::floor(double(gx) / TileCells)), tz = int32_t(std::floor(double(gz) / TileCells));
        cell = size_t(gz - int64_t(tz) * TileCells) * TileCells + size_t(gx - int64_t(tx) * TileCells);
        return lookup({level, tx, tz});
    }
    // Thickness and shares between cell centres, the shares weighted by the thickness that carries
    // them: a finer cell takes the mix of its parents, not that of the one it lies in.
    float blend(int64_t ix, int64_t iz, float tx, float tz, PileShares& shares) const {
        float pile = 0;
        PileShares sum{0, 0, 0};
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < 2; ++i) {
                size_t cell;
                const Tile* tile = at(ix + i, iz + j, cell);
                if (!tile || !tile->volcanic || tile->volcanic->pile.empty()) continue;
                const float part = (i ? tx : 1 - tx) * (j ? tz : 1 - tz) * tile->volcanic->pile[cell];
                const PileShares own = sharesOf(*tile->volcanic, cell);
                pile += part, sum.lava += part * own.lava, sum.flows += part * own.flows, sum.wet += part * own.wet;
            }
        }
        shares = pile > 0 ? PileShares{sum.lava / pile, sum.flows / pile, sum.wet / pile} : PileShares{};
        return pile;
    }
    // The parent's datum through the same smoothing spline its rock was read by (landscape/fn/Seed.cpp).
    float datum(double fx, double fz) const {
        const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
        double wx[4], wz[4], sum = 0;
        splineWeights(fx - double(ix), wx);
        splineWeights(fz - double(iz), wz);
        for (int j = 0; j < 4; ++j) {
            for (int i = 0; i < 4; ++i) {
                size_t cell;
                const Tile* tile = at(ix - 1 + i, iz - 1 + j, cell);
                if (tile) sum += wx[i] * wz[j] * tile->now.datum[cell];
            }
        }
        return float(sum);
    }
};

// Index in the domain's list of a vent another tile lists; added if it is new and near enough.
uint16_t adopt(Domain& domain, const Vent& vent, double reach) {
    std::vector<Vent>& vents = domain.volcanic.vents;
    for (size_t k = 0; k < vents.size(); ++k)
        if (vents[k].id == vent.id) return uint16_t(k);
    const double span = domain.size * domain.cell;
    bool near = false;
    for (const Feeder& feeder : vent.feeders)
        near = near || (feeder.site.x > domain.origin.x - reach && feeder.site.z > domain.origin.z - reach && feeder.site.x < domain.origin.x + span + reach &&
                        feeder.site.z < domain.origin.z + span + reach);
    if (!near || vents.size() >= NoVent - 1) return NoVent;
    vents.push_back(vent);
    return uint16_t(vents.size() - 1);
}

void takeCell(Domain& domain, size_t i, const VolcanicTile& from, size_t cell, float thickness, const PileShares& shares, float rock, float datum, uint16_t vent) {
    VolcanicDomain& volcanic = domain.volcanic;
    volcanic.floor[i] = datum - (rock - thickness);
    setShares(volcanic, i, thickness, shares);
    volcanic.laidAt[i] = float(domain.endTime - domain.startTime) - from.laid[cell];
    volcanic.vent[i] = vent;
}

void inheritParent(Domain& domain, const TileLookup& lookup) {
    const Parents parents{lookup, Level(int(domain.key.level) - 1)};
    const double parentCell = domain.cell * Refinement, reach = FinerReach[size_t(domain.key.level)];
    const int64_t lowX = int64_t(std::floor(domain.origin.x / parentCell)) - 1, lowZ = int64_t(std::floor(domain.origin.z / parentCell)) - 1;
    const int64_t highX = int64_t(std::floor((domain.origin.x + domain.size * domain.cell) / parentCell)) + 1, highZ = int64_t(std::floor((domain.origin.z + domain.size * domain.cell) / parentCell)) + 1;
    bool any = false;
    for (int64_t tz = lowZ / TileCells - 1; tz <= highZ / TileCells + 1; ++tz) {
        for (int64_t tx = lowX / TileCells - 1; tx <= highX / TileCells + 1; ++tx) {
            const Tile* tile = lookup({parents.level, int32_t(tx), int32_t(tz)});
            if (!tile || !tile->volcanic) continue;
            any = any || !tile->volcanic->pile.empty();
            for (const Vent& vent : tile->volcanic->vents) adopt(domain, vent, reach);
        }
    }
    if (!any) return;
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const Vec2 here = domain.center(x, z);
            const double fx = here.x / parentCell - 0.5, fz = here.z / parentCell - 0.5;
            const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
            const float tx = float(fx - double(ix)), tz = float(fz - double(iz));
            PileShares shares;
            const float thickness = parents.blend(ix, iz, tx, tz, shares);
            if (thickness < ThinnestPile) continue;
            size_t cell;
            const Tile* tile = parents.at(int64_t(std::floor(here.x / parentCell)), int64_t(std::floor(here.z / parentCell)), cell);
            if (!tile || !tile->volcanic || tile->volcanic->pile.empty()) continue;
            allocateVolcanic(domain);
            const size_t i = domain.index(x, z);
            Ground& ground = domain.ground;
            // The seeding lifted the datum to the rock wherever rock stood above it. Under a pile that
            // is no artefact of smoothing: the layering stays below, and only the pile's floor bounds it.
            if (ground.datum[i] <= ground.rock[i]) ground.datum[i] = std::max(ground.rock[i] - thickness, parents.datum(fx, fz));
            const uint16_t from = tile->volcanic->vent[cell];
            const uint16_t vent = from < tile->volcanic->vents.size() ? adopt(domain, tile->volcanic->vents[from], 1e30) : NoVent;
            takeCell(domain, i, *tile->volcanic, cell, thickness, shares, ground.rock[i], ground.datum[i], vent);
        }
    }
}

// Under the cells a finished neighbour owns lies the pile that neighbour ended with.
void adoptNeighbours(Domain& domain) {
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const int tx = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, tz = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
            const Tile* tile = domain.finished[(tz + 1) * 3 + tx + 1];
            if (!tile || !tile->volcanic || tile->volcanic->pile.empty()) continue;
            const size_t cell = size_t(z - Halo - tz * TileCells) * TileCells + size_t(x - Halo - tx * TileCells), i = domain.index(x, z);
            const float thickness = tile->volcanic->pile[cell];
            if (thickness < ThinnestPile) continue;
            allocateVolcanic(domain);
            const uint16_t from = tile->volcanic->vent[cell];
            const uint16_t vent = from < tile->volcanic->vents.size() ? adopt(domain, tile->volcanic->vents[from], 1e30) : NoVent;
            takeCell(domain, i, *tile->volcanic, cell, thickness, sharesOf(*tile->volcanic, cell), domain.settledTo.rock[i], domain.settledTo.datum[i], vent);
        }
    }
}

// Cells a conduit comes up through, for levels whose cells are narrower than a conduit.
void markPlugs(Domain& domain) {
    VolcanicDomain& volcanic = domain.volcanic;
    for (size_t index = 0; index < volcanic.vents.size(); ++index) {
        for (const Feeder& feeder : volcanic.vents[index].feeders) {
            const int x0 = int(std::floor((feeder.site.x - feeder.radius - domain.origin.x) / domain.cell)), x1 = int(std::floor((feeder.site.x + feeder.radius - domain.origin.x) / domain.cell));
            const int z0 = int(std::floor((feeder.site.z - feeder.radius - domain.origin.z) / domain.cell)), z1 = int(std::floor((feeder.site.z + feeder.radius - domain.origin.z) / domain.cell));
            for (int z = std::max(0, z0); z <= std::min(domain.size - 1, z1); ++z) {
                for (int x = std::max(0, x0); x <= std::min(domain.size - 1, x1); ++x) {
                    if (length(domain.center(x, z) - feeder.site) >= feeder.radius) continue;
                    allocateVolcanic(domain);
                    if (volcanic.plug.empty()) volcanic.plug.assign(domain.cells(), 0);
                    volcanic.plug[domain.index(x, z)] = uint16_t(index + 1);
                }
            }
        }
    }
}

} // namespace

void beginVolcanic(Domain& domain, const Setting& setting, const TileLookup& lookup) {
    if (domain.key.level == Level::Region) {
        const double span = domain.size * domain.cell;
        domain.volcanic.vents = ventsNear(setting, domain.origin, domain.origin + Vec2{span, span}, RegionReach);
    } else {
        inheritParent(domain, lookup);
        sharpenCalderas(domain, lookup);
        markPlugs(domain);
    }
    adoptNeighbours(domain);
}

} // namespace derivean::sim
