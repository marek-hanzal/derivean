#include "structure/type/Collapse.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>

namespace derivean {
namespace {

constexpr int SearchPadding = 4 * VoxelsPerMeter;

struct Island {
    std::vector<Cell> cells;
    std::unordered_map<int, float> links;
    float mass{};
    float direct{};
    bool stable{};
};

bool horizon(Bounds area, Cell point) {
    return point.x == area.min.x || point.y == area.min.y || point.z == area.min.z
        || point.x == area.max.x - 1 || point.y == area.max.y - 1 || point.z == area.max.z - 1;
}

void discover(const Voxels& voxels, Volume<int>& owners,
              std::vector<Island>& islands, std::vector<Cell>& seeds) {
    for (size_t nextSeed = 0; nextSeed < seeds.size(); ++nextSeed) {
        const Cell seed = seeds[nextSeed];
        if (!voxels.bounds.contains(seed) || voxels[seed] == Material::Air || owners[seed] || horizon(voxels.bounds, seed)) continue;
        const bool weak = voxels[seed] == Material::Leaf;
        islands.emplace_back();
        Island& island = islands.back();
        const int id = int(islands.size());
        island.cells.push_back(seed);
        owners[seed] = id;
        for (size_t i = 0; i < island.cells.size(); ++i) {
            const Cell point = island.cells[i];
            const auto& material = materialInfo(voxels[point]);
            island.mass += material.density * VoxelVolume;
            for (Cell delta : Neighbors) {
                const Cell neighbor = point + delta;
                if (!voxels.bounds.contains(neighbor)) {
                    // Search includes a one-cell halo; only actual solid contacts can support an edge.
                    continue;
                }
                const Material other = voxels[neighbor];
                if (other == Material::Air) continue;
                if (horizon(voxels.bounds, neighbor)) {
                    island.direct += std::min(material.support, materialInfo(other).support);
                } else if (!owners[neighbor]) {
                    if ((other == Material::Leaf) == weak) {
                        owners[neighbor] = id;
                        island.cells.push_back(neighbor);
                    } else {
                        seeds.push_back(neighbor);
                    }
                }
            }
        }
    }

    // Weak material is an explicit load path between islands. A single leaf face cannot
    // donate the strength of the solid block behind it.
    for (size_t i = 0; i < islands.size(); ++i) {
        Island& island = islands[i];
        if (voxels[island.cells.front()] != Material::Leaf) continue;
        for (Cell point : island.cells) {
            for (Cell delta : Neighbors) {
                const Cell neighbor = point + delta;
                if (!voxels.bounds.contains(neighbor)) continue;
                const int other = owners[neighbor] - 1;
                if (other < 0 || other == int(i)) continue;
                const float capacity = materialInfo(Material::Leaf).support;
                island.links[other] += capacity;
                islands[other].links[int(i)] += capacity;
            }
        }
    }
}

void carryLoads(std::vector<Island>& islands) {
    for (Island& island : islands) island.stable = island.direct >= island.mass;
    for (Island& island : islands) {
        if (island.stable) continue;
        float support = island.direct;
        for (const auto [id, capacity] : island.links) {
            const Island& other = islands[id];
            support += std::min(capacity, std::max(0.0f, other.direct - other.mass));
        }
        island.stable = support >= island.mass;
    }
}

void crushLeaves(VoxelWorld& world, Voxels& voxels, Volume<int>& owners,
                 const std::vector<Island>& islands, std::vector<Cell>& changed) {
    for (const Island& island : islands) {
        if (island.stable || voxels[island.cells.front()] == Material::Leaf) continue;
        for (Cell point : island.cells) {
            // Clear the weak column under its failed bearing surface so the piece can drop.
            Cell below = point + Cell{0, -1, 0};
            while (voxels.bounds.contains(below) && voxels[below] == Material::Leaf) {
                removeVoxel(world, below);
                voxels[below] = Material::Air;
                owners[below] = 0;
                changed.push_back(below);
                --below.y;
            }
        }
    }
}

void extract(VoxelWorld& world, const Voxels& original, std::vector<Cell>& changed,
             const std::vector<Cell>& cells, CollapseResult& detached) {
    Voxels voxels(boundsOf(cells));
    float mass = 0;
    float weightedFriction = 0;
    for (Cell point : cells) {
        const Material material = original[point];
        const auto& info = materialInfo(material);
        removeVoxel(world, point);
        changed.push_back(point);
        if (info.matter == Matter::Granular) {
            detached.looseSand.push_back(point);
            continue;
        }
        const float weight = info.density * VoxelVolume;
        mass += weight;
        weightedFriction += weight * info.friction;
        voxels[point] = material;
    }
    if (mass > 0) detached.pieces.push_back({std::move(voxels), mass, weightedFriction / mass});
}

} // namespace

CollapseResult collapse(VoxelWorld& world, std::vector<Cell>& changed) {
    if (changed.empty()) return {};
    const Bounds area = boundsOf(changed).padded(SearchPadding + 1);
    Voxels voxels = sampleWorld(world, area);
    Volume<int> owners(area);
    std::vector<Cell> seeds;
    for (Cell hole : changed) {
        for (Cell direction : Neighbors) seeds.push_back(hole + direction);
    }
    std::vector<Island> islands;
    discover(voxels, owners, islands, seeds);
    carryLoads(islands);
    crushLeaves(world, voxels, owners, islands, changed);

    CollapseResult detached;
    for (const Island& island : islands) {
        if (island.stable) continue;
        for (Cell seed : island.cells) {
            if (owners[seed] == 0) continue;
            std::vector<Cell> cells{seed};
            owners[seed] = 0;
            for (size_t i = 0; i < cells.size(); ++i) {
                for (Cell delta : Neighbors) {
                    const Cell point = cells[i] + delta;
                    if (!area.contains(point)) continue;
                    const int id = owners[point] - 1;
                    if (id < 0 || islands[id].stable) continue;
                    owners[point] = 0;
                    cells.push_back(point);
                }
            }
            extract(world, voxels, changed, cells, detached);
        }
    }
    return detached;
}

} // namespace derivean
