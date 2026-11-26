#include "GroundwaterFixture.hpp"

#include <cstdio>
#include <cstring>

using namespace derivean::sim;

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
                                  std::memcmp(&west.carried[a * 2 + node], &east.carried[b * 2 + node], sizeof(Borne)) == 0;
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
    std::printf("ground water: two tiles worked out apart agree on %d wet nodes along their shared edge; %.3g m3/yr cross it eastward; %.1f %% of the western tile's recharge is followed no farther\n",
                compared, out, 100 * first->budget.underflow / first->budget.recharge);
}

