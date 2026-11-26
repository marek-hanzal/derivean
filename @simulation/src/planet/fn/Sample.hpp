#pragma once

#include "planet/type/Planet.hpp"

namespace derivean::sim {

// Planet fields read at a world position. Positions wrap; cell values sit at cell centres.
inline double planetGridX(double x) { return x / PlanetCell - 0.5; }

inline double samplePlanet(const Field<float>& field, Vec2 position, Kernel kernel = Kernel::Spline) {
    return sample(field, planetGridX(position.x), planetGridX(position.z), kernel, Edge::Wrap);
}

inline const CrustColumn& planetColumn(const Planet& planet, Vec2 position) {
    return planet.columns.at(wrapIndex(floorToInt(position.x / PlanetCell), PlanetCells),
                             wrapIndex(floorToInt(position.z / PlanetCell), PlanetCells));
}

// Bilinear weights of the four columns around a position, for blending per-column records.
struct ColumnBlend {
    const CrustColumn* column[4];
    double weight[4];
};

inline ColumnBlend blendColumns(const Planet& planet, Vec2 position) {
    const double gx = planetGridX(position.x), gz = planetGridX(position.z);
    const int ix = floorToInt(gx), iz = floorToInt(gz);
    const double tx = gx - ix, tz = gz - iz;
    ColumnBlend blend;
    for (int corner = 0; corner < 4; ++corner) {
        const int ox = corner & 1, oz = corner >> 1;
        blend.column[corner] = &planet.columns.at(wrapIndex(ix + ox, PlanetCells), wrapIndex(iz + oz, PlanetCells));
        blend.weight[corner] = (ox ? tx : 1 - tx) * (oz ? tz : 1 - tz);
    }
    return blend;
}

// Plate contact kinematics at a position, blended from the four cells around it so nothing steps
// at a cell edge. Interiors beyond ContactReach answer with twice that distance and no motion.
struct ContactSample {
    double distance{};      // m to the nearest plate contact
    double convergence{};   // m/yr, positive where the plates close
    double shear{};         // m/yr, magnitude
};

inline ContactSample contactAt(const Planet& planet, Vec2 position) {
    const double gx = planetGridX(position.x), gz = planetGridX(position.z);
    const int ix = floorToInt(gx), iz = floorToInt(gz);
    const double tx = gx - ix, tz = gz - iz;
    ContactSample sample;
    for (int corner = 0; corner < 4; ++corner) {
        const int ox = corner & 1, oz = corner >> 1;
        const ContactCell& cell = planet.contact.at(wrapIndex(ix + ox, PlanetCells), wrapIndex(iz + oz, PlanetCells));
        const double weight = (ox ? tx : 1 - tx) * (oz ? tz : 1 - tz);
        sample.distance += weight * std::min(double(cell.distance), 2.0 * ContactReach);
        sample.convergence += weight * cell.convergence;
        sample.shear += weight * std::abs(cell.shear);
    }
    return sample;
}

} // namespace derivean::sim
