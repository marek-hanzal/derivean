#include "granular/type/Sand.hpp"
#include "particles/type/VoxelContact.hpp"
#include "physics/constant/Gravity.hpp"
#include "physics/service/JoltRuntime.hpp"

#include <cmath>
#include <numbers>
#include <optional>

namespace derivean {
namespace {

constexpr float GrainSize = VoxelSize;

std::optional<Cell> fallDirection(const VoxelWorld& world, Cell voxel) {
    if (materialAt(world, voxel + Cell{0, -1, 0}) == Material::Air) return Cell{};
    static const float minimum = std::tan(materialInfo(Material::Sand).angleOfRepose * (std::numbers::pi_v<float> / 180.0f));
    const size_t first = CellHash{}(voxel);
    for (size_t i = 0; i < Neighbors.size(); ++i) {
        const Cell side = Neighbors[(i + first) % Neighbors.size()];
        if (side.y || materialAt(world, voxel + side) != Material::Air) continue;
        const Cell support = voxel + side + Cell{0, -1, 0};
        if (materialAt(world, support) != Material::Air) continue;
        for (int run = 1; run <= 3; ++run) {
            const Cell edge = voxel + side * run;
            if (materialAt(world, edge) != Material::Air) break;
            int drop = 0;
            while (drop < 4 && materialAt(world, edge + Cell{0, -1 - drop, 0}) == Material::Air) ++drop;
            if (drop > minimum * run) return side;
        }
    }
    return std::nullopt;
}

void addGrain(SandState& sand, Cell point, Cell direction, Material material) {
    sand.grains.push_back({
        {(point.x + 0.5 + direction.x) * VoxelSize,
         (point.y + 0.5) * VoxelSize,
         (point.z + 0.5 + direction.z) * VoxelSize},
        {direction.x * 0.35, 0, direction.z * 0.35},
        material,
    });
}

void neighborsAbove(std::vector<Cell>& candidates, Cell hole) {
    const Cell above = hole + Cell{0, 1, 0};
    candidates.push_back(above);
    for (Cell side : Neighbors) {
        if (!side.y) candidates.push_back(above + side);
    }
}

} // namespace

void releaseSand(SandState& sand, VoxelWorld& world, std::vector<Cell>& changed) {
    std::vector<Cell> candidates;
    for (Cell hole : changed) neighborsAbove(candidates, hole);
    for (size_t i = 0; i < candidates.size(); ++i) {
        const Cell point = candidates[i];
        if (materialAt(world, point) != Material::Sand) continue;
        const auto direction = fallDirection(world, point);
        if (!direction) continue;
        removeVoxel(world, point);
        changed.push_back(point);
        neighborsAbove(candidates, point);

        addGrain(sand, point, *direction, Material::Sand);
    }
}

void scatterSand(SandState& sand, std::span<const Cell> cells, Material material) {
    for (Cell point : cells) addGrain(sand, point, {}, material);
}

void stepSand(SandState& sand, VoxelWorld& world, WaterState& water, double dt, std::vector<Cell>& landed,
              const JoltRuntime* physics, double lowestMoving) {
    std::erase_if(sand.grains, [&](SandGrain& grain) {
        // A grain whose own ground is not loaded waits where it is; working it free of that
        // unknown would move it by what happens to be loaded.
        if (reachesUnloaded(world, grain.position, GrainSize * 0.5)) return false;
        const auto clear = recoverFromTerrain(world, grain.position, GrainSize * 0.5);
        if (!clear) return false;
        const Position from = *clear;
        Position to = from;
        for (int axis = 0; axis < 3; ++axis) to[axis] += grain.velocity[axis] * dt;
        to[1] -= 0.5 * Gravity * dt * dt;
        grain.velocity[1] -= Gravity * dt;

        const auto moving = physics ? physics->sweepMoving(from, to, GrainSize * 0.5) : std::nullopt;
        Position lastFree = from;
        const int samples = sweepSamples(from, to);
        for (int i = 1; i <= samples; ++i) {
            const double fraction = i / double(samples);
            if (moving && moving->fraction <= fraction) {
                grain.position = interpolate(from, to, std::max(0.0, moving->fraction - 0.005));
                grain.velocity = rebound(grain.velocity, *moving, 0.36);
                if (std::abs(moving->normal[1]) > 0.55)
                    grain.velocity[0] += ((cellAt(from).x ^ cellAt(from).z) & 1) ? 0.46 : -0.46;
                for (int axis = 0; axis < 3; ++axis) grain.position[axis] += moving->normal[axis] * 0.003;
                return false;
            }
            const Position sample = interpolate(from, to, fraction);
            if (const Contact contact = probeTerrain(world, sample, GrainSize * 0.5); contact != Contact::Clear) {
                // Ground that is not loaded offers no surface yet. The grain waits as it is, so
                // where it ends up does not depend on what was loaded when it got here.
                if (contact == Contact::Unloaded) {
                    grain.velocity[1] += Gravity * dt;
                    return false;
                }
                const Cell site = cellAt(lastFree);
                if (materialAt(world, site) == Material::Air &&
                    (!physics || !physics->overlapsMovingVoxel(site)) && displaceWater(water, world, site)) {
                    setVoxel(world, site, grain.material);
                    landed.push_back(site);
                    ++sand.deposited;
                    return true;
                }
                grain.position = lastFree;
                grain.velocity[1] = std::max(0.2, -grain.velocity[1] * 0.28);
                return false;
            }
            lastFree = sample;
        }

        grain.position = to;
        const double upper = to[1] + GrainSize * 0.5;
        if (grain.velocity[1] >= 0 || !inVoid(world, upper) || upper >= lowestMoving) return false;
        ++sand.escaped;
        return true;
    });
}

void projectSand(ParticleProjection& output, const SandState& sand, Bounds visible, Position origin) {
    for (const auto& grain : sand.grains) {
        if (!visible.contains(cellAt(grain.position))) continue;
        appendParticle(output, float(grain.position[0] - origin[0]), float(grain.position[1] - origin[1]),
                       float(grain.position[2] - origin[2]), GrainSize, ParticleKind::Sand);
    }
}

} // namespace derivean
