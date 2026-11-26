#include "LandscapeFixture.hpp"

#include "coast/type/SeaVoid.hpp"
#include "coast/type/Shore.hpp"
#include "core/fn/Hash.hpp"

using namespace derivean::sim;

namespace {

constexpr int Shoreline = 128;       // first column of water in the hand-built tile
constexpr float CliffTop = 30.0f;    // m
constexpr int Places = 5;

// A site tile with a straight cliff of rock facing east into the sea, struck by three-metre
// waves that have cut its foot two metres back.
Tile cliffTile(TileKey key) {
    Tile tile;
    tile.key = key;
    const size_t cells = size_t(TileCells) * TileCells;
    tile.now.resize(cells);
    tile.water.assign(cells, 0.0f);
    auto shore = std::make_shared<ShoreRecord>();
    shore->height.assign(cells * WaveDirections, 0);
    shore->notch.assign(cells, 0.0f), shore->cut.assign(cells, 0.0f), shore->laid.assign(cells, 0.0f);
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = size_t(z) * TileCells + size_t(x);
            const bool land = x < Shoreline;
            tile.now.rock[i] = land ? CliffTop : -2.0f;
            tile.now.datum[i] = CliffTop;
            tile.water[i] = land ? CliffTop - 1.0f : 0.0f;
            if (x != Shoreline - 1) continue;
            shore->height[i * WaveDirections + 4] = uint8_t(3.0f / WaveQuantum);   // the train that travels west
            shore->notch[i] = 2.0f;
        }
    tile.shore = shore;
    return tile;
}

} // namespace

// The voids of the waterline are properties of points in space: the same through any rectangle
// that reads them and through any lattice of voxels cut from them.
void testCoastVoid() {
    const Setting& setting = testWorld().atlas.setting();
    const double cell = specOf(Level::Site).cell, span = tileSpan(Level::Site);
    int caves = 0, inCave = 0;
    // Structural domains are some twenty kilometres across; a few of them hold joints a cave can follow.
    for (int place = 0; place < Places; ++place) {
        const TileKey key{Level::Site, place * 40, place * 23};
        const Tile tile = cliffTile(key);
        const SiteTiles tiles = [&](int32_t x, int32_t z) { return x == key.x && z == key.z ? &tile : nullptr; };
        const double face = key.x * span + Shoreline * cell, north = key.z * span;
        const SeaVoids whole = seaVoidsOver(tiles, setting, face - 60, north + 200, face + 20, north + 800);
        int notches = 0;
        for (const SeaVoid& v : whole.voids) {
            (v.kind == SeaVoidKind::Notch ? notches : caves) += 1;
            expectLandscape(v.wide > 0.2f && v.high >= 0.4f && std::abs(v.y) < 12.0f, "a sea void has no size or hangs away from the waterline", v.y);
            if (v.kind == SeaVoidKind::Cave) expectLandscape(std::hypot(v.bx - v.ax, v.bz - v.az) <= 32.0 && std::min(v.ax, v.bx) < face, "a sea cave does not run from the face into the rock", v.bx - v.ax);
        }
        expectLandscape(notches >= 140, "the foot of a wave-struck cliff has no notch", notches);
        // Half a metre inside the face at the waterline is in the notch; rock far behind it is not.
        expectLandscape(inSeaVoid(whole, face - 0.5, 0.2, north + 402.0) && !inSeaVoid(whole, face - 50.0, 0.2, north + 402.0), "the notch is not where the waves cut it", face);

        int differing = 0, open = 0;
        for (int n = 0; n < 160; ++n) {
            // Half of the points anywhere in the rock behind the face, half at its foot.
            const bool foot = n % 2;
            const double x = face - (foot ? 3.5 : 34) * unit(hash(71, n, place)), z = north + 240 + 520 * unit(hash(72, n, place)), y = foot ? -1.5 + 3 * unit(hash(73, n, place)) : -3 + 16 * unit(hash(73, n, place));
            const bool inside = inSeaVoid(whole, x, y, z);
            open += inside;
            // Through a rectangle of another size and place.
            const double reach = 1 + 30 * unit(hash(74, n, place));
            differing += inside != inSeaVoid(seaVoidsOver(tiles, setting, x - reach, z - 0.5 * reach, x + 0.3 * reach, z + reach), x, y, z);
            // Through a column of voxels whose lattice puts one of its centres on the point.
            const double step = n % 4 < 2 ? 0.25 : 0.5;
            const int count = 24, at = int(hash(75, n, place) % uint64_t(count));
            Material column[24];
            for (Material& voxel : column) voxel = Material::Granite;
            cutSeaVoids(whole, x, z, y - (at + 0.5) * step, step, count, column, 1);
            differing += (column[at] == Material::Air) != inside;
        }
        expectLandscape(differing == 0, "a sea void depends on the rectangle or the lattice it is read through", differing);
        expectLandscape(open > 15, "the points of the void check reached no void", open);

        // Points in and around every cave, each as one cube of a block whose size, edge and alignment
        // are drawn anew, read as simulation/fn/Block.cpp reads them: the voids over the block's own
        // rectangle, cut into its columns after the form of the ground.
        for (const SeaVoid& v : whole.voids) {
            if (v.kind != SeaVoidKind::Cave) continue;
            for (int n = 0; n < 24; ++n) {
                const uint64_t draw = hash(81, n, place);
                const double along = 0.2 + 0.75 * unit(hash(82, n, place)), lift = (n % 3 == 2 ? 1.3 : 0.8) * (unit(hash(83, n, place)) * 2 - 1);
                const double x = v.ax + (v.bx - v.ax) * along, z = v.az + (v.bz - v.az) * along, y = v.y + lift * v.high;
                const double step = 0.125 * double(1 << (draw % 3));
                const int count = 4 + int((draw >> 4) % 5);
                const int ix = int((draw >> 8) % uint64_t(count)), iy = int((draw >> 16) % uint64_t(count)), iz = int((draw >> 24) % uint64_t(count));
                const double originX = x - (ix + 0.5) * step, originY = y - (iy + 0.5) * step, originZ = z - (iz + 0.5) * step;
                const SeaVoids block = seaVoidsOver(tiles, setting, originX, originZ, originX + count * step, originZ + count * step);
                std::vector<Material> cubes(size_t(count) * size_t(count) * size_t(count), Material::Granite);
                const size_t layer = size_t(count) * size_t(count);
                for (int cz = 0; cz < count; ++cz)
                    for (int cx = 0; cx < count; ++cx)
                        cutSeaVoids(block, originX + (cx + 0.5) * step, originZ + (cz + 0.5) * step, originY, step, count, cubes.data() + size_t(cz) * size_t(count) + size_t(cx), layer);
                const bool inside = inSeaVoid(whole, x, y, z);
                inCave += inside;
                differing += (cubes[size_t(iy) * layer + size_t(iz) * size_t(count) + size_t(ix)] == Material::Air) != inside;
            }
        }
        expectLandscape(differing == 0, "a point in or by a sea cave reads differently through another block", differing);
    }
    expectLandscape(caves > 0, "no joint of five cliffs was opened into a cave", caves);
    expectLandscape(inCave > 10, "the block check reached no point inside a sea cave", inCave);
}
