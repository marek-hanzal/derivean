#include "volcanic/fn/Volcanic.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t BedStream = 0x50494c45;
constexpr float Pitch[2] = {14.0f, 34.0f};   // m from the base of one flow to the base of the next, by Magma
constexpr float Youngest = 0.03f;            // least share of a level's settling time that erosion is given on fresh rock

Material lavaOf(const VolcanicTile* tile, const std::vector<Vent>& vents, uint8_t vent) {
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

Material neckAt(const VolcanicTile& volcanic, Vec2 position) {
    for (const Vent& vent : volcanic.vents)
        for (const Feeder& feeder : vent.feeders)
            if (length(position - feeder.site) < feeder.radius) return vent.magma == Magma::Andesite ? Material::Andesite : Material::Basalt;
    return Material::Air;
}

bool volcanicUnit(Domain& domain, size_t i, uint64_t seed) {
    VolcanicDomain& volcanic = domain.volcanic;
    if (volcanic.floor.empty()) return false;
    const float rock = domain.ground.rock[i], datum = domain.ground.datum[i];
    const float height = rock - (datum - volcanic.floor[i]);
    const uint8_t plug = volcanic.plug.empty() ? 0 : volcanic.plug[i];
    if (!plug && height <= 0) {
        // Cut through: the next reader of the cache must not find the pile's traits in it.
        if (volcanic.inside[i]) domain.unit[i] = UnitCache{}, volcanic.inside[i] = 0;
        return false;
    }
    volcanic.inside[i] = 1;
    const bool region = domain.key.level == Level::Region;
    const float depth = std::max(0.0f, datum - rock);
    RockTraits traits;
    float own = 1e9f;
    if (plug) {
        traits = traitsOf(lavaOf(nullptr, volcanic.vents, uint8_t(plug - 1)));
    } else {
        const Material lava = lavaOf(nullptr, volcanic.vents, volcanic.vent[i]);
        const float share = volcanic.allLaid[i] > 0 ? volcanic.lavaLaid[i] / volcanic.allLaid[i] : 1.0f;
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
            traits = traitsOf(bed.rock);
            own = height - bed.bottom;
        }
    }
    if (!region && !volcanic.laidAt.empty()) {
        // A finer level settles for a fixed span whatever the age of the ground. Rock laid within
        // that span has met the weather only since: it is given that share of the span's erosion.
        const double span = domain.endTime - domain.startTime, age = span - double(volcanic.laidAt[i]);
        const float young = float(std::clamp(age / span, double(Youngest), 1.0));
        traits.erodibility *= young;
        traits.weathering *= young;
    }
    domain.unit[i] = {depth, depth + std::max(own, 1e-3f), traits};
    return true;
}

std::shared_ptr<const VolcanicTile> volcanicOf(const Domain& domain) {
    const VolcanicDomain& volcanic = domain.volcanic;
    if (volcanic.vents.empty() && volcanic.floor.empty()) return nullptr;
    auto tile = std::make_shared<VolcanicTile>();
    tile->vents = volcanic.vents;
    tile->ledger = volcanic.ledger;
    if (volcanic.floor.empty()) return tile;
    const size_t cells = size_t(TileCells) * TileCells;
    tile->pile.assign(cells, 0.0f);
    tile->laid.assign(cells, 0.0f);
    tile->lava.assign(cells, 0);
    tile->vent.assign(cells, 255);
    const float span = float(domain.endTime - domain.startTime);
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            const float height = domain.ground.rock[from] - (domain.ground.datum[from] - volcanic.floor[from]);
            if (height <= 0) continue;
            tile->pile[to] = height;
            tile->laid[to] = span - volcanic.laidAt[from];
            tile->lava[to] = uint8_t(std::lround(255.0f * (volcanic.allLaid[from] > 0 ? std::clamp(volcanic.lavaLaid[from] / volcanic.allLaid[from], 0.0f, 1.0f) : 1.0f)));
            tile->vent[to] = volcanic.vent[from];
        }
    }
    return tile;
}

} // namespace derivean::sim
