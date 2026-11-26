#include "GroundwaterFixture.hpp"

#include "groundwater/fn/Pavement.hpp"
#include "groundwater/fn/Recharge.hpp"
#include "groundwater/fn/Swallow.hpp"

#include <cstdio>
#include <cstring>

using namespace derivean::sim;

namespace {

// The closed forms other levels are to read: what soaks in, what a streambed swallows, what a bare
// limestone surface opens into.
void testHooks() {
    const auto soak = [](Material rock, float fines, float slope, float warmth, float ice) { return infiltrationShare({1.0f, warmth, slope, {0, 0, fines}, ice, rock}); };
    expectWater(soak(Material::Shale, 0, 0.1f, 8, 0) < 0.05f, "bare shale soaks up rain", soak(Material::Shale, 0, 0.1f, 8, 0));
    expectWater(soak(Material::Sandstone, 0, 0.1f, 8, 0) > 0.8f, "bare sandstone sheds rain", soak(Material::Sandstone, 0, 0.1f, 8, 0));
    expectWater(soak(Material::Limestone, 0, 0.1f, 8, 0) > 0.7f, "bare limestone sheds rain", soak(Material::Limestone, 0, 0.1f, 8, 0));
    expectWater(soak(Material::Granite, 0.5f, 0.1f, 8, 0) > soak(Material::Granite, 0.5f, 1.2f, 8, 0), "steep ground soaks up as much as gentle ground");
    expectWater(soak(Material::Granite, 0.5f, 0.1f, -6, 0) == 0 && soak(Material::Granite, 0.5f, 0.1f, 8, 30) == 0, "frozen or ice-covered ground soaks up rain");

    const float bare = swallowRate({Material::Limestone, 0, 0, 10, 0}), plugged = swallowRate({Material::Limestone, 4, 0, 10, 0});
    expectWater(swallowRate({Material::Granite, 0, 0.5f, 10, 0}) == 0 && swallowRate({Material::RockSalt, 0, 0, 10, 0}) == 0, "rock that water does not fissure swallows streams");
    expectWater(bare > 0 && plugged < 0.1f * bare, "cover does not plug the swallow holes of limestone", plugged / bare);
    expectWater(swallowed(bare, 1.0e4f, 64) == 1.0e4f, "a rill on bare limestone does not sink whole", swallowed(bare, 1.0e4f, 64));
    expectWater(swallowed(bare, 3.0e9f, 64) < 0.05f * 3.0e9f, "a river loses most of its water to one cell of limestone", swallowed(bare, 3.0e9f, 64) / 3.0e9f);
    expectWater(sinkSolution(45e-6f, 4096.0f, 64) == 45e-6f, "a metre of water sinking in a cell does not dissolve what a metre of runoff does");

    // Pavement: grikes only along the joints, as wide and deep as the catalogue's, clints between.
    RockMass mass;
    mass.sets[0] = {0.4, 0, 2.5f, 0.8f}, mass.sets[1] = {0.4 + 1.5707963, 0, 3.5f, 0.8f};
    mass.domain = 77;
    const Pavement pavement = pavementOf(Material::Limestone, 0, 0.6f);
    expectWater(pavementOf(Material::Granite, 0, 0.6f).halfWidth == 0 && pavementOf(Material::Limestone, 0.5f, 0.6f).halfWidth == 0, "granite or covered limestone is a pavement");
    expectWater(pavement.halfWidth >= 0.05f && pavement.halfWidth <= 0.3f && pavement.depth >= 0.5f && pavement.depth <= 3.0f, "grikes are not of the size limestone pavements have", pavement.depth);
    int open = 0, total = 0;
    float deepest = 0;
    for (double z = 0; z < 60; z += 0.125)
        for (double x = 0; x < 60; x += 0.125) {
            const float depth = grikeDepth(42, pavement, mass, {5000 + x, -3000 + z}, 20);
            const float joint = std::min(jointDistance(42, mass, 0, 0, {5000 + x, -3000 + z}, 20), jointDistance(42, mass, 1, 0, {5000 + x, -3000 + z}, 20));
            expectWater(depth == 0 || joint < 2 * pavement.halfWidth, "a grike lies away from every joint", joint);
            open += depth > 0, ++total, deepest = std::max(deepest, depth);
        }
    const float share = float(open) / float(total), expected = grikeShare(pavement, mass);
    expectWater(share > 0.4f * expected && share < 2.0f * expected && share < 0.4f, "the grikes of a pavement do not take the share of it they should", share);
    std::printf("ground water: bare ground soaks up %.0f %% (shale), %.0f %% (granite under soil), %.0f %% (sandstone), %.0f %% (limestone) of what evaporation leaves; a pavement is %.0f %% grikes %.2f m wide and up to %.1f m deep\n",
                100 * soak(Material::Shale, 0, 0.1f, 8, 0), 100 * soak(Material::Granite, 0.5f, 0.1f, 8, 0), 100 * soak(Material::Sandstone, 0, 0.1f, 8, 0), 100 * soak(Material::Limestone, 0, 0.1f, 8, 0),
                100 * share, 2 * pavement.halfWidth, deepest);
}

} // namespace

// A world of rolling ground given cell by cell: sandstone on a shale that crops out along the
// slopes, streams in the hollows, a patch of limestone. Two neighbouring tiles are worked out from
// it, each on its own, and must say the same of the cells along the edge they share.
void testGroundwaterEdge() {
    const auto world = [](int x, int z) {
        HandColumn hand;
        hand.ground = 400.0f + 90.0f * std::sin(x * 0.043f + 0.3f) * std::cos(z * 0.037f) + 40.0f * std::sin((x + 2 * z) * 0.11f);
        hand.rain = 0.25f + 0.1f * std::sin(z * 0.05f);
        const float shaleTop = std::min(hand.ground, 395.0f + 0.2f * x - 0.1f * z - 60.0f * std::floor((0.2f * x - 0.1f * z) / 60.0f));
        const bool lime = (x / 40 + z / 56) % 3 == 0;
        if (hand.ground > shaleTop + 0.1f) hand.layers[hand.count++] = handLayer(lime ? Material::Limestone : Material::Sandstone, hand.ground, shaleTop, lime ? 3e-5f : 1e-5f, 7);
        hand.layers[hand.count++] = handLayer(Material::Shale, shaleTop, shaleTop - 12.0f, 1e-10f, 9);
        hand.layers[hand.count++] = handLayer(Material::Granite, shaleTop - 12.0f, hand.ground - 250.0f, 3e-7f);
        return hand;
    };
    const auto dress = [](WaterField& field) {
        for (int z = 0; z < field.size; ++z)
            for (int x = 0; x < field.size; ++x) {
                AquiferColumn& column = field.column[field.index(x, z)];
                const int cellX = int(field.firstX) + x, cellZ = int(field.firstZ) + z;
                column.dipX = -0.2f / float(WaterCell), column.dipZ = 0.1f / float(WaterCell);
                column.flowTo = uint8_t((cellX * 7 + cellZ * 3) & 7);
                column.stream = column.ground < 340.0f ? 4.0e5f : 2.0e3f;
                if (column.ground < 330.0f) column.drain = Drain::Stream;
                column.swallow = column.upperSoluble > 0 ? 3000.0f : 0.0f;
                column.crush = (cellX + cellZ) % 97 == 0 ? 0.9f : 0.0f;
            }
    };
    WaterField west = handTile({Level::Local, 0, 0}, world), east = handTile({Level::Local, 1, 0}, world);
    dress(west), dress(east);
    const std::shared_ptr<GroundwaterTile> first = solveField(west), second = solveField(east);
    expectWater(std::abs(imbalance(first->budget)) < 1e-6 && std::abs(imbalance(second->budget)) < 1e-6, "a tile with neighbours does not account for its water", imbalance(first->budget));
    expectWater(first->budget.lateralOut > 0 && first->budget.underflow < 0.2 * first->budget.recharge, "no water crosses the edge of a tile, or most is lost on the way", first->budget.underflow);
    int compared = 0;
    for (int column = TileCells - 1; column <= TileCells; ++column)
        for (int z = 0; z < TileCells; ++z) {
            const size_t a = west.index(WaterReach + column, WaterReach + z), b = east.index(WaterReach + column - TileCells, WaterReach + z);
            for (size_t node = 0; node < 2; ++node) {
                const NodeFate &f = west.fate[a * 2 + node], &g = east.fate[b * 2 + node];
                const bool same = west.level[a * 2 + node] == east.level[b * 2 + node] && west.flux[a * 2 + node] == east.flux[b * 2 + node] &&
                                  west.sideways[a * 2 + node] == east.sideways[b * 2 + node] && f.emerges == g.emerges && f.outcrop == g.outcrop && f.lost == g.lost &&
                                  f.captured == g.captured && f.lag == g.lag && west.column[a].sunk == east.column[b].sunk &&
                                  std::memcmp(&west.borne[a * 2 + node], &east.borne[b * 2 + node], sizeof(Borne)) == 0;
                expectWater(same, "two tiles disagree about the ground water of a cell on their shared edge", z);
                compared += west.flux[a * 2 + node] > 0;
            }
        }
    expectWater(compared > TileCells, "the shared edge of two tiles carries no ground water to compare", compared);
    // What leaves one tile eastward is what enters the other: the same sum from both fields.
    double out = 0, in = 0;
    for (int z = 0; z < TileCells; ++z)
        for (int storey = Upper; storey <= Lower; ++storey)
            for (const int k : {7, 0, 1}) {
                const size_t a = west.index(WaterReach + TileCells - 1, WaterReach + z), b = east.index(WaterReach - 1, WaterReach + z);
                if (z + NeighborZ[k] < 0 || z + NeighborZ[k] >= TileCells) continue;
                if (storey == Lower || Way(west.way[a * 8 + size_t(k)]) != Way::Outcrop) out += double(west.sideways[a * 2 + size_t(storey)]) * west.share[(a * 2 + size_t(storey)) * 8 + size_t(k)];
                if (storey == Lower || Way(east.way[b * 8 + size_t(k)]) != Way::Outcrop) in += double(east.sideways[b * 2 + size_t(storey)]) * east.share[(b * 2 + size_t(storey)) * 8 + size_t(k)];
            }
    expectWater(out > 0 && out == in, "water crossing a tile edge is not the same amount on both sides", out - in);
    testHooks();
    std::printf("ground water: two tiles worked out apart agree on %d wet nodes along their shared edge; %.3g m3/yr cross it eastward; %.1f %% of the western tile's recharge is followed no farther\n",
                compared, out, 100 * first->budget.underflow / first->budget.recharge);
}

