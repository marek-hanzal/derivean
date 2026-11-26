#pragma once

#include "particles/type/Particles.hpp"
#include "water/type/Water.hpp"

namespace derivean {

struct SandGrain {
    Position position;
    Position velocity;
    Material material{Material::Sand};   // what it settles back into
};

struct SandState {
    std::vector<SandGrain> grains;
    uint64_t deposited{}, escaped{};
};

void releaseSand(SandState& sand, VoxelWorld& world, std::vector<Cell>& changed);
void scatterSand(SandState& sand, std::span<const Cell> cells, Material material = Material::Sand);
void stepSand(SandState& sand, VoxelWorld& world, WaterState& water, double dt, std::vector<Cell>& landed,
              const JoltRuntime* physics = nullptr, double lowestMoving = 1e100);
void projectSand(ParticleProjection& output, const SandState& sand, Bounds visible, Position origin);

} // namespace derivean
