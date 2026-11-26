#include "volcanic/fn/Emplace.hpp"

#include <cmath>

namespace derivean::sim {

double calderaRadius(double volume, Magma magma) { return std::sqrt(volume / (3.14159265358979323846 * CalderaDepth[size_t(magma)])); }

std::vector<float> founder(const Emplacement& field, Vec2 centre, double volume, Magma magma) {
    std::vector<float> drop(field.cells(), 0.0f);
    const double radius = calderaRadius(volume, magma);
    // The roof goes down as one block inside the ring fault; the wall between block and rim is as
    // narrow as the grid can draw it, and failure and weather take it from there.
    const double wall = std::max(0.5 * field.cell, 0.06 * radius);
    const auto share = [&](Vec2 at) { return 1.0 - smoothstep(radius - wall, radius + wall, length(at - centre)); };
    // Normalised over the world's lattice of cells, so the pit holds the volume that left whatever
    // part of it this grid covers.
    const int reach = int(std::ceil((radius + wall) / field.cell)) + 1;
    const int64_t midX = int64_t(std::floor(centre.x / field.cell)), midZ = int64_t(std::floor(centre.z / field.cell));
    const int64_t firstX = int64_t(std::llround(field.origin.x / field.cell)), firstZ = int64_t(std::llround(field.origin.z / field.cell));
    double total = 0;
    for (int dz = -reach; dz <= reach; ++dz)
        for (int dx = -reach; dx <= reach; ++dx) total += share({(double(midX + dx) + 0.5) * field.cell, (double(midZ + dz) + 0.5) * field.cell});
    if (total <= 0) return drop;
    const double depth = volume / (total * field.cell * field.cell);
    for (int dz = -reach; dz <= reach; ++dz) {
        for (int dx = -reach; dx <= reach; ++dx) {
            const int x = int(midX + dx - firstX), z = int(midZ + dz - firstZ);
            if (!field.contains(x, z)) continue;
            drop[field.index(x, z)] = float(depth * share({(double(midX + dx) + 0.5) * field.cell, (double(midZ + dz) + 0.5) * field.cell}));
        }
    }
    return drop;
}

} // namespace derivean::sim
