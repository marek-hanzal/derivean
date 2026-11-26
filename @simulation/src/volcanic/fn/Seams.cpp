#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {
namespace {

// The pile and lava share a finished neighbour's own replay left at a cell of this tile, read from
// the Apron cells its record keeps beyond its edge; a neighbour without a pile there has none.
struct Beyond {
    float pile{}, share{};
};

} // namespace

// Tiles are replayed apart; their surfaces meet because the later tile blends its Apron cells along
// a finished neighbour's edge towards what that neighbour's replay left there (landscape/fn/Extract.cpp
// `blendSeams`). The rock and datum it blends say nothing of where the pile under them ends, so the
// pile is blended with the same weights: its thickness, and its lava share by thickness.
void blendPileSeams(Domain& domain) {
    const auto weight = [](int distance) { return float(smoothstep(0.0, 1.0, (distance + 0.5) / Apron)); };
    bool any = !domain.volcanic.floor.empty();
    for (const Tile* tile : domain.finished) any = any || (tile && tile->volcanic && !tile->volcanic->wide.empty());
    if (!any) return;
    allocateVolcanic(domain);
    VolcanicDomain& volcanic = domain.volcanic;
    const Ground& ground = domain.ground;
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const int sideX = x < Apron ? -1 : x >= TileCells - Apron ? 1 : 0, sideZ = z < Apron ? -1 : z >= TileCells - Apron ? 1 : 0;
            const Tile* alongX = sideX ? domain.finished[4 + sideX] : nullptr;
            const Tile* alongZ = sideZ ? domain.finished[4 + 3 * sideZ] : nullptr;
            if (!alongX && !alongZ) continue;
            const float wx = alongX ? weight(sideX < 0 ? x : TileCells - 1 - x) : 1.0f;
            const float wz = alongZ ? weight(sideZ < 0 ? z : TileCells - 1 - z) : 1.0f;
            const Tile* diagonal = alongX && alongZ ? domain.finished[4 + sideX + 3 * sideZ] : nullptr;
            const size_t i = domain.index(x + Halo, z + Halo);
            const float own = std::max(0.0f, ground.rock[i] - (ground.datum[i] - volcanic.floor[i]));
            const float ownShare = lavaShare(volcanic, i);
            const auto beyond = [&](const Tile* tile, int dx, int dz) {
                if (!tile->volcanic || tile->volcanic->wide.empty()) return Beyond{0.0f, ownShare};
                const size_t at = size_t(z - dz * TileCells + Apron) * WideCells + size_t(x - dx * TileCells + Apron);
                return Beyond{tile->volcanic->wide[at], float(tile->volcanic->wideLava[at]) / 255.0f};
            };
            const Beyond fromX = alongX ? beyond(alongX, sideX, 0) : Beyond{own, ownShare};
            const Beyond fromZ = alongZ ? beyond(alongZ, 0, sideZ) : Beyond{own, ownShare};
            const Beyond fromBoth = diagonal ? beyond(diagonal, sideX, sideZ) : Beyond{0.5f * (fromX.pile + fromZ.pile), 0.5f * (fromX.share + fromZ.share)};
            const float weights[4] = {wx * wz, (1 - wx) * wz, wx * (1 - wz), (1 - wx) * (1 - wz)};
            const Beyond parts[4] = {{own, ownShare}, fromX, fromZ, fromBoth};
            float pile = 0, lava = 0;
            for (int k = 0; k < 4; ++k) pile += weights[k] * parts[k].pile, lava += weights[k] * parts[k].pile * parts[k].share;
            if (pile < ThinnestPile) {
                if (own > 0) volcanic.floor[i] = NoPile, volcanic.allLaid[i] = volcanic.lavaLaid[i] = 0;
                continue;
            }
            volcanic.floor[i] = ground.datum[i] - (ground.rock[i] - pile);
            volcanic.allLaid[i] = pile;
            volcanic.lavaLaid[i] = std::min(pile, lava);
            // A cell the blend gives its first pile takes the top's vent from the nearest cell inward
            // that has one, past the blended band.
            if (volcanic.vent[i] == NoVent) {
                const int innerX = std::clamp(x, Apron, TileCells - 1 - Apron), innerZ = std::clamp(z, Apron, TileCells - 1 - Apron);
                volcanic.vent[i] = volcanic.vent[domain.index(innerX + Halo, innerZ + Halo)];
            }
        }
    }
}

} // namespace derivean::sim
