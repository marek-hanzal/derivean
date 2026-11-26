#pragma once

#include "ground/type/Detail.hpp"
#include "simulation/fn/Reader.hpp"

#include <vector>

namespace derivean::sim {

// The surface at one horizontal position, with its fall line.
struct GroundPoint {
    float rock{}, datum{}, ground{};   // `ground` is the top of the loose cover, under any spring deposit
    float cover[SedimentClasses]{};
    float slope{};   // rise over run of the rock surface, taken over a metre or more
    Vec2 fall;       // unit vector pointing downhill
    float crust{};   // m of spring deposit on the ground
    Crust crustKind{Crust::None};
    Material coarse{Material::Gravel}, fine{Material::Silt};   // what the coarse and the fine layer read as
    bool bare{};     // the fine layer carries no soil
    PilePoint pile;  // volcanic rock under the rock surface: it fills (rock − pile.thickness, rock]; the layering is read below it
};

// The finest finished surface on a world-anchored lattice over a rectangle: the ground level's own
// cell centres. Voxels read it by bilinear interpolation. Because the nodes are fixed in world
// space, a point gets the same answer whatever block asks for it.
struct GroundLattice {
    double spacing{0.25};           // m between nodes
    double shift{0.125};            // m from a multiple of `spacing` to a node
    int64_t originX{}, originZ{};   // lattice index of node (0, 0)
    int countX{}, countZ{};
    std::vector<float> rock, datum, cover[SedimentClasses], crust;
    std::vector<float> lie;         // m, the rock surface as a slope has it: its mean over SmoothReach to every side
    std::vector<uint8_t> crustKind, coarseKind, fineKind, bare;
    std::vector<PilePoint> pile;    // empty where no tile under the lattice holds volcanic rock

    GroundPoint at(double x, double z) const;
    // The slope's own surface: what the fall line and the faces of beds are read from. A ledge or a
    // rill half a metre wide is on the ground, not in the lie of the slope.
    float lieAt(double x, double z) const;
    // m of loose cover at a point.
    float looseAt(double x, double z) const;
};

inline constexpr double SmoothReach = 2.0;   // m to every side over which the lie of a slope is taken
inline constexpr double SlopeReach = 1.0;    // m to either side over which its fall line is taken
// m of finished ground tiles a reader needs around the rectangle it passes to readGround.
inline constexpr double LatticeMargin = SmoothReach + SlopeReach + 0.5;

// `reader` must be of the ground level and every tile within LatticeMargin of the rectangle finished.
// Every point of the rectangle then reads the same values whatever rectangle it is asked through.
GroundLattice readGround(TileReader& reader, double minX, double minZ, double maxX, double maxZ);

// Works out `lie` from `rock`. readGround has done it; a caller that moves the rock afterwards does it again.
void smoothLie(GroundLattice& lattice);

} // namespace derivean::sim
