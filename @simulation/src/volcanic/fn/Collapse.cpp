#include "volcanic/fn/Emplace.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {

double calderaRadius(double volume, Magma magma) { return std::sqrt(volume / (3.14159265358979323846 * CalderaDepth[size_t(magma)])); }

std::vector<float> blockDrop(const Emplacement& field, Vec2 centre, double volume, Magma magma) {
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

// The ring fault cuts whatever stands over the chamber, a cone built in the same years included:
// the roof goes down as a piston by `block`, and what stood over the ring above the level it comes
// to breaks up and falls in, so the pit is floored level across the ring, as at Crater Lake or
// Santorini. The pit holds the volume that left the chamber, which is the block's: the void between
// the ground as it stood and that floor. On level ground the floor lies the block's depth down
// everywhere, and the drop is the block.
std::vector<float> founder(const Emplacement& field, const std::vector<float>& block) {
    std::vector<float> drop(block.size(), 0.0f);
    std::vector<size_t> ring;
    float most = 0;
    double covered = 0, low = 1e30, high = -1e30;
    for (size_t i = 0; i < block.size(); ++i) {
        if (block[i] <= 0) continue;
        ring.push_back(i);
        most = std::max(most, block[i]);
        covered += block[i];
        low = std::min(low, double(field.ground[i])), high = std::max(high, double(field.ground[i]));
    }
    if (ring.empty()) return drop;
    // The block's share of each cell, the wall's included, is what of it goes down to the floor. The
    // floor is solved over every cell of the ring, a finished neighbour's too, so that its floor
    // and this one are the same; only the caller's own cells go down.
    const auto above = [&](double floor) {
        double sum = 0;
        for (size_t i : ring) sum += block[i] / most * std::max(0.0, double(field.ground[i]) - floor);
        return sum;
    };
    // On ground no higher anywhere than its lowest cell the floor would lie the block's depth below that.
    double bottom = low - double(most), top = high;
    for (int k = 0; k < 60; ++k) {
        const double floor = 0.5 * (bottom + top);
        (above(floor) > covered ? bottom : top) = floor;
    }
    const double floor = 0.5 * (bottom + top);
    for (size_t i : ring) drop[i] = float(block[i] / most * std::max(0.0, double(field.ground[i]) - floor));
    return drop;
}

} // namespace derivean::sim
