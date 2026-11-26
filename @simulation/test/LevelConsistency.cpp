#include "LandscapeFixture.hpp"

#include "measure/type/Mosaic.hpp"

#include <algorithm>
#include <cstdio>

using namespace derivean::sim;

namespace {

// A finished tile beside the later one, and where it lies from it.
struct Seam {
    const Tile* finished;
    int dx, dz;   // where the finished tile lies from the later one
};

size_t cellOf(int x, int z) { return size_t(z) * TileCells + size_t(x); }

float groundOf(const Tile& tile, size_t i) {
    float ground = tile.now.rock[i];
    for (int c = 0; c < SedimentClasses; ++c) ground += tile.now.sediment[c][i];
    return ground;
}

// Rivers that a finished tile sends across its edge into the tile replayed after it: the later
// tile must carry each on with the water the finished one recorded leaving. The cell it enters has
// that water as inflow by construction, so the river is followed four cells on down the later
// tile's own course. A course that steps back into the finished tile leaves the river to that
// tile's record there, which must then carry it: a finished cell passes what it recorded.
void checkHandedOn(const Seam& seam, const Tile& later, int& crossings) {
    const Tile& finished = *seam.finished;
    const float threshold = float(riverThreshold(later.key.level));
    for (int along = 0; along < TileCells; ++along) {
        // The finished tile's cell on the shared edge, in its own coordinates.
        const int fx = seam.dx ? (seam.dx < 0 ? TileCells - 1 : 0) : along, fz = seam.dz ? (seam.dz < 0 ? TileCells - 1 : 0) : along;
        const size_t i = cellOf(fx, fz);
        const uint8_t way = finished.flowTo[i];
        const float q = finished.discharge[i];
        if (way > 7 || q < threshold || finished.water[i] > groundOf(finished, i)) continue;
        if ((seam.dx && NeighborX[way] != -seam.dx) || (seam.dz && NeighborZ[way] != -seam.dz)) continue;
        int x = fx + NeighborX[way] + seam.dx * TileCells, z = fz + NeighborZ[way] + seam.dz * TileCells;
        if (x < 0 || z < 0 || x >= TileCells || z >= TileCells) continue;
        float carried = later.discharge[cellOf(x, z)];
        for (int on = 0; on < 4; ++on) {
            const uint8_t next = later.flowTo[cellOf(x, z)];
            if (next > 7) break;
            const int nx = x + NeighborX[next], nz = z + NeighborZ[next];
            if (nx < 0 || nz < 0 || nx >= TileCells || nz >= TileCells) {
                // Back across the seam the finished tile's own record carries it, or nothing does. A
                // finished cell that itself sends its water into the later tile carries nothing on.
                const int bx = nx - seam.dx * TileCells, bz = nz - seam.dz * TileCells;
                if (bx >= 0 && bz >= 0 && bx < TileCells && bz < TileCells) {
                    const uint8_t onward = finished.flowTo[cellOf(bx, bz)];
                    const int ox = bx + (onward > 7 ? 0 : NeighborX[onward]), oz = bz + (onward > 7 ? 0 : NeighborZ[onward]);
                    const bool intoLater = onward < 8 && (ox < 0 || oz < 0 || ox >= TileCells || oz >= TileCells);
                    carried = intoLater ? 0.0f : finished.discharge[cellOf(bx, bz)];
                }
                break;
            }
            x = nx, z = nz;
            carried = later.discharge[cellOf(x, z)];
        }
        ++crossings;
        expectLandscape(carried >= 0.7f * q, "a river a finished tile sends into its later neighbour is lost past the edge", carried / q);
    }
}

// Standing water within three cells of the later tile's edges against the rest of it. A replay
// that dams the valleys its finished neighbours drain through, or gathers its water along their
// edge, shows here as a row of lakes along the seam.
void checkSeamLakes(const Tile& later) {
    long near = 0, nearLake = 0, inside = 0, insideLake = 0;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = cellOf(x, z);
            const float ground = groundOf(later, i);
            const bool lake = later.water[i] > ground && later.water[i] > later.seaLevel + 0.5;
            const bool edge = std::min({x, z, TileCells - 1 - x, TileCells - 1 - z}) < 3;
            (edge ? near : inside) += 1;
            (edge ? nearLake : insideLake) += lake;
        }
    const double nearShare = double(nearLake) / double(near), insideShare = double(insideLake) / double(inside);
    std::printf("level consistency: lakes along the seams of the tile replayed last %.2f %% against %.2f %% inside\n", 100 * nearShare, 100 * insideShare);
    expectLandscape(nearShare <= 3 * insideShare + 0.01, "the tile replayed after all its neighbours holds a row of lakes along its seams", nearShare);
}

} // namespace

// Lakes, rivers and edges are the same features on both sides of a tile edge. A tile replayed
// after its neighbours takes their rivers where they leave them (Domain::settledWay). The tile of
// the last pass beside the focus has finished neighbours on all eight sides.
void testLevelConsistency() {
    World& world = testWorld();
    const TileKey focus = tileAt(Level::Local, testFocus());
    const TileKey last{Level::Local, focus.x + 1, focus.z + 1};
    expectLandscape(passOf(focus) == 0 && passOf(last) == 3, "the tiles around the focus are not of the passes this check reads");
    for (int at = 0; at < 9; ++at) world.atlas.request({Level::Local, last.x + at % 3 - 1, last.z + at / 3 - 1});
    std::shared_ptr<const Tile> held[9];
    for (int at = 0; at < 9; ++at) held[at] = world.atlas.require({Level::Local, last.x + at % 3 - 1, last.z + at / 3 - 1});
    const Tile& later = *held[4];
    int crossings = 0;
    for (const Seam seam : {Seam{held[3].get(), -1, 0}, Seam{held[5].get(), 1, 0}, Seam{held[1].get(), 0, -1}, Seam{held[7].get(), 0, 1}})
        checkHandedOn(seam, later, crossings);
    // The pass-1 tile between the focus and its eastern neighbour takes rivers from both sides.
    const Tile& between = *held[1];
    for (const Seam seam : {Seam{held[0].get(), -1, 0}, Seam{held[2].get(), 1, 0}}) checkHandedOn(seam, between, crossings);
    expectLandscape(crossings > 0, "no river crosses from a finished local tile into a later one: the check reads nothing");
    std::printf("level consistency: %d rivers handed from finished local tiles to later ones, all carried on\n", crossings);
    checkSeamLakes(later);
}
