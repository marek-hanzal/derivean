#pragma once

namespace derivean::sim {

// Centres of a regular lattice of cubes: `count` cubes of edge `step` along each axis from the
// corner `origin`. Materials are indexed (y * countZ + z) * countX + x.
struct CubeLattice {
    double originX{}, originY{}, originZ{};
    double step{};
    int countX{}, countY{}, countZ{};
};

} // namespace derivean::sim
