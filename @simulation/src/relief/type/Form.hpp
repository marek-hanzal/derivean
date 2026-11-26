#pragma once

#include "groundwater/type/Cave.hpp"
#include "ore/type/Ore.hpp"
#include "relief/type/Ground.hpp"
#include "rockmass/type/RockMass.hpp"
#include "strata/type/Column.hpp"

namespace derivean::sim {

// What a column of voxels is made of under the simulated ground surface, and the shapes rock takes
// that a height field cannot hold. They are closed-form consequences of what the levels simulated
// (layering, slope, cover) and of seeded fracture geometry; no time stepping.
//  - Beds weather back from a steep face by an amount set by their strength, so resistant beds
//    stand out as ledges and roofs over the recesses of the weak ones.
//  - Massive, widely jointed rock near a cliff band opens along the planes of its rock mass into
//    slots and free-standing towers; a fin between two slots can be cut through into an arch
//    (relief/type/City.hpp).
//  - Seams of weak rock between the layers of a bed, and veins and pipes that cut the beds.
//  - Voids the groundwater dissolved: air where a conduit passes through rock; where one reaches
//    the surface the cover over it has fallen in and lies on its floor.
// A bed a layering node has met, with its rock mass: asking the mass anew for every column is too slow.
struct BedMass {
    Material rock{Material::Bedrock};
    float thickness{};
    double laidDown{};
    RockMass mass;
};

struct FormSetting {
    uint64_t seed{};
    StrataColumn strata;     // layering of the lattice cell the column lies in
    Vec2 strataCentre;       // world position that `strata.material` belongs to
    OreField ore;            // bodies that cut the beds around that lattice cell
    // The node's own column and the beds its columns have met so far; null for a column on its own.
    // A mass is asked with the node's column, so it does not depend on which column met the bed first.
    const StrataColumn* node{};
    std::vector<BedMass>* masses{};
    const VolcanicTile* volcanic{};   // vents whose plugs may cut the column (volcanic/type/Pile.hpp); null where there are none
    float runoff{};          // m/yr the ground sheds where the column stands: bare soluble rock opens by it into grikes (groundwater/fn/Pavement.hpp)
};

// Materials of one vertical run of `count` points, `step` apart upwards from `bottom`, written to
// `out[k * stride]`. `exposure` (0..1) says how close the column is to steep ground. `caves` is
// the field of the local tile the column stands in, or null.
void formColumn(const FormSetting& setting, const GroundLattice& ground, float exposure, const CaveField* caves, double x, double z,
                double bottom, double step, int count, Material* out, size_t stride);

// How close a point is to a cliff band: from the relief of the site surface within ExposureReach.
float exposureAt(TileReader& reader, Vec2 position);

inline constexpr double StrataLattice = 16.0;     // m, spacing of the layering nodes
inline constexpr double ExposureLattice = 8.0;    // m, spacing at which exposure is evaluated
inline constexpr double ExposureReach = 24.0;     // m, distance at which a cliff band is felt
inline constexpr double FormMargin = 4.0;         // m of ground surface a column needs around itself
// m around a block of voxels within which tiles must be finished before its columns can be cut: site
// tiles for the exposure, ground tiles for the surface, the lie of the slope and the stones that
// reach in. sampleBlock builds what is missing; a caller that must not wait asks readyAround first.
// The exposure is read on its own lattice around the block, a reach to every side of each node, and
// each of those reads interpolates over two site cells more.
inline constexpr double BlockSiteReach = ExposureLattice + ExposureReach + 2 * 4.0 + 0.5;
inline constexpr double BlockGroundReach = FormMargin + LatticeMargin > ClastReach ? FormMargin + LatticeMargin : ClastReach;

} // namespace derivean::sim
