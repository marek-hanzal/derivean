#include "volcanic/fn/Volcanic.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t BedStream = 0x50494c45;
constexpr float Pitch[2] = {14.0f, 34.0f};   // m from the base of one flow to the base of the next, by Magma

Material lavaOf(const VolcanicTile* tile, const std::vector<Vent>& vents, uint16_t vent) {
    const std::vector<Vent>& list = tile ? tile->vents : vents;
    return vent < list.size() && list[vent].magma == Magma::Andesite ? Material::Andesite : Material::Basalt;
}

// Every flow lies on the ash of the eruption before it. How much of a pitch is lava follows the
// cell's own share of lava, so a wall near the vent shows thick flows and one far downwind mostly ash.
PileBed bedIn(uint64_t seed, uint64_t ventId, Material lava, float share, float height) {
    const float pitch = Pitch[lava == Material::Andesite ? 1 : 0];
    const int64_t number = int64_t(std::floor(std::max(0.0f, height) / pitch));
    const float uneven = 0.3f * (float(unit(stream(seed, BedStream) ^ ventId, number)) - 0.5f);
    const float cut = pitch * (share > 0.97f ? 1.0f : share < 0.03f ? 0.0f : std::clamp(share + uneven, 0.05f, 0.95f));
    const float base = float(number) * pitch;
    if (std::max(0.0f, height) - base < cut) return {base, base + cut, lava, true};
    return {base + cut, base + pitch, Material::Tuff, false};
}

} // namespace

PileColumn pileColumn(const Tile& tile, size_t cell) {
    if (!tile.volcanic || tile.volcanic->pile.empty()) return {};
    const VolcanicTile& volcanic = *tile.volcanic;
    return {volcanic.pile[cell], volcanic.laid[cell], volcanic.lava[cell], volcanic.vent[cell]};
}

PileBed pileBedAt(uint64_t seed, const VolcanicTile& volcanic, const PileColumn& column, float height) {
    const uint64_t id = column.vent < volcanic.vents.size() ? volcanic.vents[column.vent].id : 0;
    return bedIn(seed, id, lavaOf(&volcanic, {}, column.vent), float(column.lava) / 255.0f, height);
}

PilePoint pilePoint(const Tile& tile, size_t cell) {
    const PileColumn column = pileColumn(tile, cell);
    if (column.thickness <= 0) return {};
    const std::vector<Vent>& vents = tile.volcanic->vents;
    return {column.thickness, float(column.lava) / 255.0f, lavaOf(tile.volcanic.get(), {}, column.vent), column.vent < vents.size() ? vents[column.vent].id : 0};
}

PileBed pileBed(uint64_t seed, const PilePoint& point, float height) { return bedIn(seed, point.vent, point.rock, point.lava, height); }

bool volcanicUnit(Domain& domain, size_t i, uint64_t seed) {
    VolcanicDomain& volcanic = domain.volcanic;
    if (volcanic.floor.empty()) return false;
    const float rock = domain.ground.rock[i], datum = domain.ground.datum[i];
    const float height = rock - (datum - volcanic.floor[i]);
    const uint16_t plug = volcanic.plug.empty() ? 0 : volcanic.plug[i];
    if (!plug && height <= 0) {
        // Cut through: the next reader of the cache must not find the pile's traits in it.
        if (volcanic.inside[i]) {
            domain.unit[i] = UnitCache{}, volcanic.inside[i] = 0;
            if (!domain.bed.empty()) domain.bed[i] = Bed{1, 0, Material::Bedrock};
        }
        return false;
    }
    volcanic.inside[i] = 1;
    const bool region = domain.key.level == Level::Region;
    const float depth = std::max(0.0f, datum - rock);
    RockTraits traits;
    float own = 1e9f;
    Material kind = Material::Tuff;
    if (plug) {
        traits = traitsOf(kind = lavaOf(nullptr, volcanic.vents, uint16_t(plug - 1)));
    } else {
        const Material lava = lavaOf(nullptr, volcanic.vents, volcanic.vent[i]);
        const float share = lavaShare(volcanic, i);
        if (region) {
            // A region cell is far thicker than a flow: it meets lava and ash together, in their shares.
            const RockTraits& hard = traitsOf(lava);
            const RockTraits& soft = traitsOf(Material::Tuff);
            traits = share >= 0.5f ? hard : soft;
            traits.erodibility = std::exp(share * std::log(hard.erodibility) + (1 - share) * std::log(soft.erodibility));
            traits.weathering = share * hard.weathering + (1 - share) * soft.weathering;
            traits.stableTan = share * hard.stableTan + (1 - share) * soft.stableTan;
            own = height;
        } else {
            const uint64_t id = volcanic.vent[i] < volcanic.vents.size() ? volcanic.vents[volcanic.vent[i]].id : 0;
            const PileBed bed = bedIn(seed, id, lava, share, height - 1e-3f);
            traits = traitsOf(kind = bed.rock);
            own = height - bed.bottom;
        }
    }
    if (!plug && !region) {
        // A finer level settles for a fixed span whatever the age of the ground: rock laid within
        // that span is given only its own share of the span's erosion. A region lives through the
        // dates themselves and needs no such share.
        const double span = domain.endTime - domain.startTime;
        const double age = std::max(0.0, span - double(volcanic.laidAt[i])) + 0.5 * domain.step;
        const float worked = float(std::min(1.0, age / span));
        traits.erodibility *= worked;
        traits.weathering *= worked;
    }
    domain.unit[i] = {depth, depth + std::max(own, 1e-3f), traits};
    // The ground level keeps the bed itself and its date besides its traits (ground/fn/Settle.cpp):
    // the rock mass of a pile is dated by the top the pile was last laid at.
    if (!domain.bed.empty()) domain.bed[i] = Bed{depth, depth + std::max(own, 1e-3f), kind};
    if (!domain.laidDown.empty()) domain.laidDown[i] = domain.startTime + double(volcanic.laidAt[i]);
    return true;
}

std::shared_ptr<const VolcanicTile> volcanicOf(Domain& domain) {
    blendPileSeams(domain);
    const VolcanicDomain& volcanic = domain.volcanic;
    if (volcanic.vents.empty() && volcanic.floor.empty()) return nullptr;
    auto tile = std::make_shared<VolcanicTile>();
    tile->vents = volcanic.vents;
    tile->ledger = volcanic.ledger;
    if (domain.key.level == Level::Region) tile->calderas = finishCalderas(domain);
    if (volcanic.floor.empty()) return tile;
    const size_t cells = size_t(TileCells) * TileCells;
    tile->pile.assign(cells, 0.0f);
    tile->laid.assign(cells, 0.0f);
    for (auto* share : {&tile->lava, &tile->flows, &tile->wet}) share->assign(cells, 0);
    tile->vent.assign(cells, NoVent);
    const float span = float(domain.endTime - domain.startTime);
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            const float height = domain.ground.rock[from] - (domain.ground.datum[from] - volcanic.floor[from]);
            if (height <= 0) continue;
            tile->pile[to] = height;
            tile->laid[to] = span - volcanic.laidAt[from];
            const PileShares shares = sharesAt(volcanic, from);
            tile->lava[to] = partsOf(shares.lava), tile->flows[to] = partsOf(shares.flows), tile->wet[to] = partsOf(shares.wet);
            tile->vent[to] = volcanic.vent[from];
        }
    }
    // The tile and its Apron as this replay left them, for a later neighbour's blend; kept only
    // where some pile lies in the Apron beyond the tile's edge, which is all a neighbour reads.
    const auto heightAt = [&](int x, int z) {
        const size_t from = domain.index(x + Halo - Apron, z + Halo - Apron);
        return domain.ground.rock[from] - (domain.ground.datum[from] - volcanic.floor[from]);
    };
    bool beyond = false;
    for (int z = 0; z < WideCells && !beyond; ++z)
        for (int x = 0; x < WideCells && !beyond; ++x)
            beyond = (x < Apron || z < Apron || x >= Apron + TileCells || z >= Apron + TileCells) && heightAt(x, z) >= ThinnestPile;
    if (!beyond) return tile;
    tile->wide.assign(size_t(WideCells) * WideCells, 0.0f);
    for (auto* share : {&tile->wideLava, &tile->wideFlows, &tile->wideWet}) share->assign(size_t(WideCells) * WideCells, 0);
    for (int z = 0; z < WideCells; ++z) {
        for (int x = 0; x < WideCells; ++x) {
            const float height = heightAt(x, z);
            if (height <= 0) continue;
            const size_t to = size_t(z) * WideCells + size_t(x);
            tile->wide[to] = height;
            const PileShares shares = sharesAt(volcanic, domain.index(x + Halo - Apron, z + Halo - Apron));
            tile->wideLava[to] = partsOf(shares.lava), tile->wideFlows[to] = partsOf(shares.flows), tile->wideWet[to] = partsOf(shares.wet);
        }
    }
    return tile;
}

} // namespace derivean::sim
