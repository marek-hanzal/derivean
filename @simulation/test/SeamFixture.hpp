#pragma once

#include "landscape/type/Setting.hpp"
#include "landscape/type/Terrain.hpp"

#include <memory>

// Three Local tiles in a row under a made parent whose one river runs down the row: it crosses
// both seams, and the two seams are finished from opposite sides, so one hands the river from the
// tile solved first to the later one and the other hands it back. The river's course is the
// parent's, not the luck of where the relief puts one. Tiles outside the row are not built: their
// halo cells evolve as a lone replay's do.
struct SeamRow {
    derivean::sim::TileKey first;   // upstream tile; the others follow down the river
    bool alongZ{};                  // the river runs towards +z, otherwise towards +x
    std::shared_ptr<const derivean::sim::Tile> tiles[3];
    double entering{};              // m³/yr of the parent's river where it enters the made parent
};

SeamRow buildSeamRow(const derivean::sim::Setting& setting, derivean::sim::TileKey first, bool alongZ);
