#include "LandscapeFixture.hpp"

#include "measure/type/Mosaic.hpp"

#include <algorithm>
#include <cstdio>

using namespace derivean::sim;

namespace {

// Rivers that a finished tile sends across its edge into the tile replayed after it: the later
// tile must carry each on with the water the finished one recorded leaving. The cell it enters has
// that water as inflow by construction, so the river is followed four cells on down the later
// tile's own course. `step` is +1 for the finished tile west of `later`, -1 for the one east of it.
void checkHandedOn(const Tile& finished, const Tile& later, int step, int& crossings) {
    const float threshold = float(riverThreshold(later.key.level));
    const int from = step > 0 ? TileCells - 1 : 0, into = step > 0 ? 0 : TileCells - 1;
    for (int z = 0; z < TileCells; ++z) {
        const size_t i = size_t(z) * TileCells + size_t(from);
        const uint8_t way = finished.flowTo[i];
        const float q = finished.discharge[i];
        if (way > 7 || NeighborX[way] != step || q < threshold || finished.water[i] > finished.now.rock[i] + 1.0f) continue;
        int x = into, at = z + NeighborZ[way];
        if (at < 0 || at >= TileCells) continue;
        // A step back into the finished tile drops the river there: that tile passes its own record.
        float carried = later.discharge[size_t(at) * TileCells + size_t(x)];
        for (int on = 0; on < 4; ++on) {
            const uint8_t next = later.flowTo[size_t(at) * TileCells + size_t(x)];
            if (next > 7) break;
            if (NeighborX[next] == -step && x == into) {
                carried = 0;
                break;
            }
            if (x + NeighborX[next] < 0 || x + NeighborX[next] >= TileCells || at + NeighborZ[next] < 0 || at + NeighborZ[next] >= TileCells) break;
            x += NeighborX[next], at += NeighborZ[next];
            carried = later.discharge[size_t(at) * TileCells + size_t(x)];
        }
        ++crossings;
        expectLandscape(carried >= 0.7f * q, "a river a finished tile sends into its later neighbour is lost past the edge", carried / q);
    }
}

} // namespace

// Lakes, rivers and edges are the same features on both sides of a tile edge. A tile replayed
// after its neighbours takes their rivers where they leave them (Domain::settledWay).
void testLevelConsistency() {
    World& world = testWorld();
    const TileKey west = tileAt(Level::Local, testFocus());
    const TileKey later{Level::Local, west.x + 1, west.z}, east{Level::Local, west.x + 2, west.z};
    expectLandscape(passOf(west) == 0 && passOf(east) == 0 && passOf(later) == 1, "the tiles around the focus are not of the passes this check reads");
    const auto a = world.atlas.require(west), b = world.atlas.require(later), c = world.atlas.require(east);
    int crossings = 0;
    checkHandedOn(*a, *b, 1, crossings);
    checkHandedOn(*c, *b, -1, crossings);
    expectLandscape(crossings > 0, "no river crosses from the finished local tiles into the one between them: the check reads nothing");
    std::printf("level consistency: %d rivers handed from finished local tiles to the one between them, all carried on\n", crossings);
}
