#pragma once

#include "strata/type/Column.hpp"

namespace derivean::sim {

// How a body of rock is broken. Every level and every volumetric form reads the same planes, so a
// corridor a 64 m cell erodes along is the street a 4 m cell cuts and the slot a voxel column opens:
// structure is one field, seen at different widths. It is attached to the crust and travels with it.
//
// Rock is cut by two steep sets of joints and by its bedding. Each set repeats at three orders:
// the joints themselves, corridors where several run together, and zones — each order OrderStep
// times wider apart than the one below and broken through more completely. Planes are exact planes
// inside a structural domain; domains are a few tens of kilometres across and end at the faults
// between them, where the rock is crushed at every scale.
struct JointSet {
    double strike{};       // rad, trend of the planes
    float lean{};          // horizontal shift per metre of depth; 0 is vertical
    float spacing{};       // m between neighbouring joints
    float persistence{};   // 0..1 share of a joint plane that is broken through
};

inline constexpr int JointSets = 2;
inline constexpr int JointOrders = 3;
inline constexpr float OrderStep = 7.0f;

struct RockMass {
    JointSet sets[JointSets];
    uint64_t domain{};       // planes are continuous inside one domain
    float crush{};           // 0..1 closeness to the fault that bounds the domain
    float dip{};             // rad, of the layering
    double dipDirection{};   // rad, the way the layering descends
    float parting{};         // m between the bedding planes blocks part along
    float block{};           // m, edge of the block that joints and bedding cut out
    float cohesion{};        // kPa of the jointed mass for a face 100 m high: the material's massCohesion as jointed here
    float friction{};        // tangent of the angle a slope of any height holds: the material's massFriction, less in crushed rock
    float openDepth{};       // m below the surface to which weather has opened the joints
};

// `laidDown` is the unit's (StrataUnit::laidDown): strata younger than the province's last
// deformation carry only its gentle warping. `material` is the crust-attached position of the point.
RockMass rockMassAt(uint64_t seed, const StrataColumn& column, const Bed& bed, double laidDown, Vec2 material);

// Metres from a point to the nearest plane of one order of one set that is broken through there;
// beyond reach (1e9) where none lies within one spacing of that order. `depth` is metres below the datum.
float jointDistance(uint64_t seed, const RockMass& mass, int set, int order, Vec2 material, float depth);

// Share 0..1 of a cell `width` across taken up by broken rock along the planes that cell can tell
// apart (orders at least three cells apart) and along the bounding fault. Finer orders are not lines
// to such a cell; they are already in `cohesion` and `block`.
float fractureShare(uint64_t seed, const RockMass& mass, Vec2 material, float depth, float width);

// Cooling joints of a lava sheet or a plug: prisms about `width` across, a little irregular, that
// stand normal to the surface the rock cooled from. `across` is the point in the plane the columns
// stand normal to (the horizontal plane for upright columns; arc length and height for columns that
// lie radial in a plug), `body` tells one cooling unit from the next. Metres to the nearest joint.
float columnJointDistance(uint64_t seed, uint64_t body, Vec2 across, float width);

} // namespace derivean::sim
