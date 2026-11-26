#include "LandscapeFixture.hpp"
#include "SeamFixture.hpp"

#include "measure/type/Mosaic.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <future>
#include <vector>

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


// m³/yr a row's river carries down the row across the line in front of along-cell `cell` of tile
// `k`; with cell 0 the line is the seam in front of tile k. What the cells behind the line send
// across it, less what the cells in front send back, each by its own tile's record. A cell's whole
// discharge goes along `flowTo`, the main of its shares: exact for a channel, a main-direction
// estimate for split sheet flow.
double carriedAcross(const SeamRow& row, int k, int cell) {
    const Tile& front = *row.tiles[k];
    const Tile& behind = cell > 0 ? front : *row.tiles[k - 1];
    const int back = cell > 0 ? cell - 1 : TileCells - 1;
    const auto ahead = [&](const Tile& tile, size_t i) {
        const uint8_t way = tile.flowTo[i];
        return way > 7 ? 0 : row.alongZ ? NeighborZ[way] : NeighborX[way];
    };
    double carried = 0;
    for (int across = 0; across < TileCells; ++across) {
        const size_t from = row.alongZ ? cellOf(across, back) : cellOf(back, across), to = row.alongZ ? cellOf(across, cell) : cellOf(cell, across);
        if (ahead(behind, from) > 0) carried += behind.discharge[from];
        if (ahead(front, to) < 0) carried -= front.discharge[to];
    }
    return carried;
}

// A seam must pass on the river of a made parent as a line through a tile does. Each row's river
// is measured across lines 128 cells apart: from 64 cells before each seam to 64 after it, and from
// line 64 to line 192 inside each tile. What a stretch loses, as a share of the river entering the
// row, is held against the stretches inside the tiles, which see the same valley and gain the same
// flank water: the seams' mean may exceed the interior's by two standard errors of the difference
// of the two means (a heuristic for four deterministic rows), and no seam may lose more than two
// interior standard deviations of the river, which a lost river would. The rows run both ways and
// at both parities: a row shifted by one tile has each seam finished from the other side, so every
// seam is crossed once from the tile solved first and once from the later one.
void checkSeamRows(const Setting& setting) {
    const TileKey focus = tileAt(Level::Local, testFocus());
    struct Case {
        int dx, dz;
        bool alongZ;
    };
    // West of the focus: soluble rock east of it takes a river underground between the lines.
    const Case cases[] = {{-4, 0, false}, {-3, 0, false}, {-2, 0, true}, {-2, 1, true}};
    std::vector<std::future<SeamRow>> running;
    for (const Case& c : cases)
        running.push_back(std::async(std::launch::async, [&setting, focus, c] { return buildSeamRow(setting, {Level::Local, focus.x + c.dx, focus.z + c.dz}, c.alongZ); }));
    std::vector<double> seams, interior;
    double lostFirst = 0, lostLater = 0;
    for (auto& result : running) {
        const SeamRow row = result.get();
        const double entered = carriedAcross(row, 0, 64);
        double sunk = 0;
        for (const auto& tile : row.tiles)
            for (const float s : tile->sunk) sunk += s;
        std::printf("level consistency: made river %s from (%d,%d), entering %.3g m3/yr, across the lines:", row.alongZ ? "south" : "east", row.first.x, row.first.z, row.entering);
        for (int k = 0; k < 3; ++k)
            for (const int cell : {0, 64, 128, 192})
                if (k || cell) std::printf(" %s%.3g", cell ? "" : "| ", carriedAcross(row, k, cell));
        std::printf("\n");
        expectLandscape(entered >= 0.5 * row.entering, "the made parent's river does not reach its row: the check reads nothing", entered / row.entering);
        expectLandscape(sunk < 0.01 * entered, "the made river sinks into soluble rock under its row: the lines do not see all of it", sunk / entered);
        for (int k = 0; k < 3; ++k) interior.push_back((carriedAcross(row, k, 64) - carriedAcross(row, k, 192)) / entered);
        for (int k = 1; k < 3; ++k) {
            const double lost = (carriedAcross(row, k - 1, 192) - carriedAcross(row, k, 64)) / entered;
            const bool fromFirst = passOf(row.tiles[k - 1]->key) < passOf(row.tiles[k]->key);
            seams.push_back(lost);
            (fromFirst ? lostFirst : lostLater) += lost / 4;
            std::printf("level consistency:   seam into (%d,%d) from the %s tile: %.1f %% of the river lost over 128 cells\n", row.tiles[k]->key.x, row.tiles[k]->key.z,
                        fromFirst ? "finished" : "later", 100 * lost);
        }
    }
    const auto mean = [](const std::vector<double>& v) { double sum = 0; for (const double x : v) sum += x; return sum / double(v.size()); };
    const double inside = mean(interior), across = mean(seams);
    double spread = 0;
    for (const double x : interior) spread += (x - inside) * (x - inside);
    spread = std::sqrt(spread / double(interior.size() - 1));
    const double worst = *std::max_element(seams.begin(), seams.end());
    const double bound = inside + 2 * spread * std::sqrt(1.0 / double(seams.size()) + 1.0 / double(interior.size()));
    std::printf("level consistency: lost over 128 cells, seams %.1f %% (from finished %.1f, from later %.1f, worst %.1f), inside the tiles %.1f %% (sd %.1f); bounds %.1f and %.1f %%\n",
                100 * across, 100 * lostFirst, 100 * lostLater, 100 * worst, 100 * inside, 100 * spread, 100 * bound, 100 * 2 * spread);
    expectLandscape(across <= bound, "the seams lose more of a made river than lines inside the tiles do", across - inside);
    expectLandscape(worst <= 2 * spread, "a seam loses a made river that lines inside the tiles carry", worst);
}

} // namespace

// Lakes, rivers and edges are the same features on both sides of a tile edge. A tile replayed
// after its neighbours takes their rivers where they leave them (Domain::settledWay). The tile of
// the last pass beside the focus has finished neighbours on all eight sides.
void testLevelConsistency() {
    World& world = testWorld();
    checkSeamRows(world.atlas.setting());
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
    // Where the real relief puts its rivers changes with every change to the relief; the made rows
    // above are what keeps this check from reading nothing.
    std::printf("level consistency: %d rivers handed from finished local tiles to later ones at the seed site, all carried on\n", crossings);
    checkSeamLakes(later);
}
