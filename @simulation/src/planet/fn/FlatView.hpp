#pragma once

#include "geometry/constant/Sphere.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/type/Globe.hpp"

namespace derivean::sim {

// The bridge between the globe's history and today's flat world (S3-design.md §8). G is the only
// owner of the conversion: the flat row z is one parallel at latitudeAt(z), and x is longitude,
// periodic over PlanetSpan, so the climate bands and `northwardZ` of the flat world hold unchanged.
//     φ(z) = latitudeAt(z),  λ(x) = 2π x / PlanetSpan,  G(x, z) = (cos φ cos λ, cos φ sin λ, sin φ).
// Every place of the sphere is twice in the flat world, on z and on its mirror across a pole.

// Metres of the sphere per flat metre along the equator and every meridian: 2πR / PlanetSpan (2.386).
inline constexpr double BridgeScale = 6.283185307179586 * PlanetRadius / PlanetSpan;

Vec3 globeDirectionOf(Vec2 flat);
// The flat position of a direction on sheet A (the half-period of z after the north pole).
Vec2 flatPositionOf(Vec3 direction);

// J = dG in the bases (east, north) of the sphere and (+x, +z) of the flat world: diag(k cos φ,
// k · northwardZ(z)), sphere metres per flat metre.
struct BridgeJacobian {
    double east{}, north{};
};
BridgeJacobian bridgeJacobianAt(double z);

// Quantities go by meaning (Codex V7). A velocity or displacement v (tangent at `direction`, sphere
// units) goes to the flat world as J⁻¹ v; a boundary normal or gradient n as Jᵀ n, normalised. Then
// (J⁻¹ v) · (Jᵀ n) = v · n and the sign of a convergence survives. Both read at the flat position `at`,
// a cell centre, where cos φ ≥ 0.009 (§8: no cell centre lies on a pole).
Vec2 flatVector(Vec2 at, Vec3 v);
Vec2 flatNormal(Vec2 at, Vec3 n);

// The flat history's strike of a contact with normal n at `direction`, atan2(n_x, −n_z) of the normal in
// the flat world (Contacts.cpp), for a column's `fabric`. It is the strike when it is written; a piece
// that turns later keeps it, as a flat piece does (the material frame is S3m's).
float flatStrikeOf(Vec3 direction, Vec3 n);

// Today's flat Planet filled from the globe: each flat cell takes the column and record of the piece
// with the largest weight in the mantle cell under G(its centre), and that cell's views of elevation
// (on which the sea was solved), swell, melt heat, erosion, deposition, temperature and rain. Plate
// velocities are the mean of J⁻¹ v over the plate's flat cells weighted by cos φ, contact normals are
// converted as above, contact distances along the normal by J⁻¹. Derived: nobody writes mass into it.
Planet flatViewOf(const Globe& globe);

// The flat climate read back onto every cell of the globe at G⁻¹ of its centre on sheet A.
void readClimateBack(Globe& globe, const Planet& view);

} // namespace derivean::sim
