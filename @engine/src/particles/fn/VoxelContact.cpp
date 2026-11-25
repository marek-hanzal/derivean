#include "particles/type/VoxelContact.hpp"

#include <cmath>

namespace derivean {

Cell cellAt(Position position) {
    return {int(std::floor(position[0] / VoxelSize)), int(std::floor(position[1] / VoxelSize)),
            int(std::floor(position[2] / VoxelSize))};
}

Position cellCenter(Cell cell) {
    return {(cell.x + 0.5) * VoxelSize, (cell.y + 0.5) * VoxelSize, (cell.z + 0.5) * VoxelSize};
}

Position interpolate(Position from, Position to, double fraction) {
    for (int axis = 0; axis < 3; ++axis) from[axis] = std::lerp(from[axis], to[axis], fraction);
    return from;
}

bool touchesTerrain(const VoxelWorld& world, Position point, double halfSize) {
    const double radius = std::max(0.0, halfSize - 0.00001);
    const Cell min = cellAt({point[0] - radius, point[1] - radius, point[2] - radius});
    const Cell max = cellAt({point[0] + radius, point[1] + radius, point[2] + radius});
    for (int y = min.y; y <= max.y; ++y)
        for (int z = min.z; z <= max.z; ++z)
            for (int x = min.x; x <= max.x; ++x)
                if (materialAt(world, {x, y, z}) != Material::Air) return true;
    return false;
}

std::optional<Position> recoverFromTerrain(const VoxelWorld& world, Position point, double halfSize) {
    // Another grain can settle or a player can build between two sweeps.
    if (!touchesTerrain(world, point, halfSize)) return point;
    const Cell center = cellAt(point);
    for (int rise = 0; rise <= 2; ++rise) {
        const Cell level = center + Cell{0, rise, 0};
        if (materialAt(world, level) == Material::Air) return cellCenter(level);
        for (Cell side : Neighbors) {
            if (!side.y && materialAt(world, level + side) == Material::Air) return cellCenter(level + side);
        }
    }
    const Cell below = center + Cell{0, -1, 0};
    if (materialAt(world, below) == Material::Air) return cellCenter(below);
    return std::nullopt;
}

int sweepSamples(Position from, Position to) {
    double longest = 0;
    for (int axis = 0; axis < 3; ++axis) longest = std::max(longest, std::abs(to[axis] - from[axis]));
    return std::max(1, int(std::ceil(longest / (VoxelSize * 0.33))));
}

Position rebound(Position velocity, const MovingContact& contact, double restitution) {
    double entering = 0;
    for (int axis = 0; axis < 3; ++axis)
        entering += (velocity[axis] - contact.velocity[axis]) * contact.normal[axis];
    if (entering < 0) {
        for (int axis = 0; axis < 3; ++axis) {
            velocity[axis] -= (1 + restitution) * entering * contact.normal[axis];
        }
    }
    return velocity;
}

} // namespace derivean
