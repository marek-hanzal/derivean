#include "ground/fn/Stages.hpp"

#include "ground/constant/Ground.hpp"
#include "ground/fn/Frost.hpp"
#include "ground/fn/Hydrothermal.hpp"
#include "ground/fn/Stones.hpp"

namespace derivean::sim {
namespace {

constexpr float AlteredToClay = 0.35f, AlteredBare = 0.15f;
constexpr float ThinFines = 0.08f;   // m of fines under which they are a film on what lies below, not a soil

size_t ownIndex(int x, int z) { return size_t(z - Halo) * TileCells + size_t(x - Halo); }

double coarseOf(const Domain& domain) {
    double total = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) total += domain.ground.sediment[Coarse][domain.index(x, z)];
    return total * domain.cell * domain.cell;
}

// What the coarse and the fine layer read as, and where the fine layer carries no soil: under
// water, on ground the frost keeps turning or hot water soaks, and where the fines are a film too
// thin to be a soil.
void nameCover(const Domain& domain, const Tile& tile, const std::vector<uint8_t>& sorted, const std::vector<float>& altered, GroundDetail& detail) {
    const auto top = [&](int x, int z) { return domain.ground.rock[domain.index(x, z)] + domain.sedimentAt(domain.index(x, z)); };
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) {
            const size_t i = domain.index(x, z), own = ownIndex(x, z);
            const float coarse = domain.ground.sediment[Coarse][i], fines = domain.ground.sediment[Fines][i], sand = domain.ground.sediment[Sand][i];
            const float slope = std::hypot(top(x + 2, z) - top(x - 2, z), top(x, z + 2) - top(x, z - 2)) / float(4 * domain.cell);
            const bool till = domain.iceAge[i] > LeastIce && coarse > 0.2f && fines + sand > 0.25f * coarse && domain.stream[i] < BedFlow;
            const bool shattered = coarse > 0.5f * (coarse + fines + sand) && frostAt(domain, i) >= FrostWork;
            detail.coarseKind[own] = uint8_t(till ? Material::Till : slope > 0.45f || shattered ? Material::Scree : Material::Gravel);
            detail.fineKind[own] = uint8_t(altered[i] > AlteredToClay ? Material::Clay : Material::Silt);
            detail.bare[own] = tile.water[own] > top(x, z) + 0.01f || sorted[i] || altered[i] > AlteredBare || fines < ThinFines;
        }
}

// The stones in the order of the index cells they lie in.
void indexStones(std::vector<Clast>& stones, GroundDetail& detail) {
    const auto binOf = [](const Clast& stone) {
        return size_t(std::clamp(int(stone.z / float(ClastBinSpan)), 0, ClastBins - 1)) * ClastBins + size_t(std::clamp(int(stone.x / float(ClastBinSpan)), 0, ClastBins - 1));
    };
    std::stable_sort(stones.begin(), stones.end(), [&](const Clast& a, const Clast& b) { return binOf(a) < binOf(b); });
    detail.binStart.assign(size_t(ClastBins) * ClastBins + 1, 0);
    for (const Clast& stone : stones) ++detail.binStart[binOf(stone) + 1];
    for (size_t bin = 0; bin < size_t(ClastBins) * ClastBins; ++bin) detail.binStart[bin + 1] += detail.binStart[bin];
    detail.clasts = std::move(stones);
}

} // namespace

void finishGround(Domain& domain, FlowScratch& scratch, const Setting& setting, const TileLookup& lookup, Tile& tile) {
    auto detail = std::make_shared<GroundDetail>();
    const size_t cells = size_t(TileCells) * TileCells;
    detail->crust.assign(cells, 0.0f);
    detail->crustKind.assign(cells, uint8_t(Crust::None));
    detail->coarseKind.assign(cells, uint8_t(Material::Gravel));
    detail->fineKind.assign(cells, uint8_t(Material::Silt));
    detail->bare.assign(cells, 0);

    const auto reroute = [&] {
        floodLevels(domain, scratch);
        routeWater(domain, scratch);
        widenChannels(domain);
    };
    reroute();
    std::vector<uint8_t> sorted;
    if (workFrozenGround(domain, scratch, sorted)) reroute();

    const SiteView sites = readSites(domain.key, lookup);
    StoneBed bed{domain, setting.seed, {}, std::vector<uint8_t>(domain.cells(), 0), {}, {}};
    bed.ledger.coarseBefore = coarseOf(domain);
    dropBlocks(bed, sites, setting);
    packFabric(bed, scratch, sites, lookup);
    organiseBeds(bed, scratch);

    // The one thing a tile asks of another model: the springs of the groundwater within reach of it.
    std::vector<Spring> springs;
    if (setting.world) {
        const double span = TileCells * domain.cell;
        const Vec2 corner{domain.origin.x + Halo * domain.cell, domain.origin.z + Halo * domain.cell};
        springs = springsIn(*setting.world, {corner.x - SpringReach, corner.z - SpringReach}, {corner.x + span + SpringReach, corner.z + span + SpringReach});
        // Where two deposits overlap the order they are laid in shows; it must not depend on the order of the answer.
        std::stable_sort(springs.begin(), springs.end(), [](const Spring& a, const Spring& b) { return a.id < b.id; });
    }
    std::vector<float> altered;
    laySpringDeposits(domain, springs, sites, *detail, altered);
    bed.ledger.matrix = coarseOf(domain);
    detail->ledger = bed.ledger;
    detail->stepGaps = std::move(bed.stepGaps);

    standWater(domain, scratch, bed.stones, *detail, tile);
    nameCover(domain, tile, sorted, altered, *detail);
    indexStones(bed.stones, *detail);
    tile.detail = std::move(detail);
}

} // namespace derivean::sim
