#include "volcanic/fn/Volcanic.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t BedStream = 0x50494c45;
constexpr float Pitch[2] = {14.0f, 34.0f};   // m from the base of one flow to the base of the next, by Magma
// A pitch thinner than `Welding` holds no welded bed: a basalt pile's never does, as basalt's rarely do.
constexpr float PillowShare = 0.6f;   // of a flow that reached water, the pillows at its foot; glass breccia lies over them

Material lavaOf(const VolcanicTile* tile, const std::vector<Vent>& vents, uint16_t vent) {
    const std::vector<Vent>& list = tile ? tile->vents : vents;
    return vent < list.size() && list[vent].magma == Magma::Andesite ? Material::Andesite : Material::Basalt;
}

// Every flow lies on what the eruption before it threw out. How much of a pitch is lava follows the
// cell's own share of lava, so a wall near the vent shows thick flows and one far downwind mostly
// ash. Each pitch draws, by the cell's shares, whether it was laid under water and what its
// fragments came down as. Basalt quenched in water sets as pillows under the glass breccia it
// sheds; stiff andesite shatters to that glass whole. The currents that welded as they were laid
// (`flowLaid`, fn/Settle.cpp) are a sheet where their bed is thick enough, and set columns as lava
// does; the cone's fall and every current that did not weld are lapilli tuff; the rest is tuff.
PileBed bedIn(uint64_t seed, uint64_t ventId, Material lava, const PileShares& shares, float height) {
    const float pitch = Pitch[lava == Material::Andesite ? 1 : 0];
    const int64_t number = int64_t(std::floor(std::max(0.0f, height) / pitch));
    const uint64_t key = stream(seed, BedStream) ^ ventId;
    const float uneven = 0.3f * (float(unit(key, number)) - 0.5f);
    const float share = shares.lava;
    const float cut = pitch * (share > 0.97f ? 1.0f : share < 0.03f ? 0.0f : std::clamp(share + uneven, 0.05f, 0.95f));
    const float base = float(number) * pitch;
    const bool wet = unit(hash(key, number, 1)) < shares.wet;
    const float into = std::max(0.0f, height) - base;
    if (into < cut) {
        if (!wet) return {base, base + cut, lava, true};
        if (lava == Material::Andesite) return {base, base + cut, Material::Hyaloclastite, false};
        const float pillows = PillowShare * cut;
        return into < pillows ? PileBed{base, base + pillows, Material::PillowLava, false} : PileBed{base + pillows, base + cut, Material::Hyaloclastite, false};
    }
    const float rest = 1.0f - share, pick = float(unit(hash(key, number, 2))) * rest;
    if (pick < shares.flows && !wet && pitch - cut >= Welding) return {base + cut, base + pitch, Material::Ignimbrite, true};
    if (pick < shares.flows + shares.fall) return {base + cut, base + pitch, Material::LapilliTuff, false};
    return {base + cut, base + pitch, Material::Tuff, false};
}

} // namespace

PileColumn pileColumn(const Tile& tile, size_t cell) {
    if (!tile.volcanic || tile.volcanic->pile.empty()) return {};
    const VolcanicTile& volcanic = *tile.volcanic;
    return {volcanic.pile[cell], volcanic.laid[cell], volcanic.lava[cell], volcanic.vent[cell], volcanic.flows[cell], volcanic.fall[cell], volcanic.wet[cell]};
}

PileBed pileBedAt(uint64_t seed, const VolcanicTile& volcanic, const PileColumn& column, float height) {
    const uint64_t id = column.vent < volcanic.vents.size() ? volcanic.vents[column.vent].id : 0;
    const auto share = [](uint8_t parts) { return float(parts) / 255.0f; };
    return bedIn(seed, id, lavaOf(&volcanic, {}, column.vent), {share(column.lava), share(column.flows), share(column.fall), share(column.wet)}, height);
}

PilePoint pilePoint(const Tile& tile, size_t cell) {
    const PileColumn column = pileColumn(tile, cell);
    if (column.thickness <= 0) return {};
    const std::vector<Vent>& vents = tile.volcanic->vents;
    const PileShares shares = sharesOf(*tile.volcanic, cell);
    return {column.thickness, shares.lava, lavaOf(tile.volcanic.get(), {}, column.vent), column.vent < vents.size() ? vents[column.vent].id : 0, shares.flows, shares.fall, shares.wet};
}

PileBed pileBed(uint64_t seed, const PilePoint& point, float height) {
    return bedIn(seed, point.vent, point.rock, {point.lava, point.flows, point.fall, point.wet}, height);
}

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
        // As a finished tile keeps them, so that the bed the voxels draw is the one eroded here.
        const auto parts = [](float share) { return float(partsOf(share)) / 255.0f; };
        const PileShares laid = sharesAt(volcanic, i), shares{parts(laid.lava), parts(laid.flows), parts(laid.fall), parts(laid.wet)};
        const float share = shares.lava;
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
            const PileBed bed = bedIn(seed, id, lava, shares, height - 1e-3f);
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
    for (auto* share : {&tile->lava, &tile->flows, &tile->fall, &tile->wet}) share->assign(cells, 0);
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
            tile->lava[to] = partsOf(shares.lava), tile->flows[to] = partsOf(shares.flows), tile->fall[to] = partsOf(shares.fall), tile->wet[to] = partsOf(shares.wet);
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
    for (auto* share : {&tile->wideLava, &tile->wideFlows, &tile->wideFall, &tile->wideWet}) share->assign(size_t(WideCells) * WideCells, 0);
    for (int z = 0; z < WideCells; ++z) {
        for (int x = 0; x < WideCells; ++x) {
            const float height = heightAt(x, z);
            if (height <= 0) continue;
            const size_t to = size_t(z) * WideCells + size_t(x);
            tile->wide[to] = height;
            const PileShares shares = sharesAt(volcanic, domain.index(x + Halo - Apron, z + Halo - Apron));
            tile->wideLava[to] = partsOf(shares.lava), tile->wideFlows[to] = partsOf(shares.flows), tile->wideFall[to] = partsOf(shares.fall), tile->wideWet[to] = partsOf(shares.wet);
        }
    }
    return tile;
}

} // namespace derivean::sim
