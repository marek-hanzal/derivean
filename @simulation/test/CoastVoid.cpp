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
    int caves = 0;
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
    }
    expectLandscape(caves > 0, "no joint of five cliffs was opened into a cave", caves);
}
