#pragma once

#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// The site level around one ground tile, copied out of the parent tiles once: the tile's own site
// cells and FallReachCells more on every side. Everything a ground tile takes from farther than its
// own domain is read here, and it is the same for every ground tile that reads the same place.
struct SiteView {
    int64_t firstX{}, firstZ{};     // global site cell of entry (0, 0)
    int side{};
    double cell{};
    std::vector<float> ground;      // m, rock plus cover
    std::vector<float> cut;         // m the rock surface lies below the datum of the layering
    std::vector<float> coarse, fine;   // m of coarse cover; of sand and fines together
    std::vector<float> discharge;   // m³/yr
    std::vector<float> shed;        // m/yr
    std::vector<uint8_t> flowTo, rock, wet, known;   // `known` is 0 where the site tile is missing

    size_t index(int x, int z) const { return size_t(z) * size_t(side) + size_t(x); }
    bool inside(int x, int z) const { return x >= 0 && z >= 0 && x < side && z < side; }
    Vec2 centre(int x, int z) const { return {(double(firstX + x) + 0.5) * cell, (double(firstZ + z) + 0.5) * cell}; }
    // Smooth surface between the cell centres, as the levels read their parents.
    float groundAt(Vec2 position) const;
};

// Where the bed material of a stream comes from: `distance` metres up the channel from a position,
// or as far as the view follows it.
struct Upstream {
    Vec2 at;
    Material rock{Material::Bedrock};
    float followed{};   // m
};

} // namespace derivean::sim
