#include "water/type/WaterTransfer.hpp"
#include "particles/type/VoxelContact.hpp"
#include "physics/constant/Gravity.hpp"
#include "simulation/type/JoltRuntime.hpp"

#include <cmath>

namespace derivean {
namespace {

double parcelWidth(int amount) {
    return VoxelSize * std::cbrt(amount / double(WaterUnits));
}

bool poolContact(const WaterState& state, Position at, double half) {
    at[1] -= half;
    const Cell cell = cellAt(at);
    const double surface = (cell.y + waterAt(state, cell) / double(WaterUnits)) * VoxelSize;
    return waterAt(state, cell) && at[1] <= surface;
}

bool receive(WaterState& state, const VoxelWorld& world, WaterParcel& parcel, Position lastFree) {
    Cell target = cellAt(lastFree);
    if (materialAt(world, target) != Material::Air) return false;
    const Cell lower = target + Cell{0, -1, 0};
    if (materialAt(world, lower) == Material::Air && waterAt(state, lower) != WaterUnits)
        target = lower;
    for (int row = 0; row < 3 && parcel.amount && materialAt(world, target) == Material::Air; ++row, ++target.y) {
        const int amount = std::min(parcel.amount, WaterUnits - waterAt(state, target));
        addWater(state, target, amount);
        state.budget.airborne -= uint64_t(amount);
        parcel.amount -= amount;
    }
    return parcel.amount == 0;
}

bool advanceParcel(WaterState& state, const VoxelWorld& world, WaterParcel& parcel,
                   double step, const JoltRuntime* physics, double lowestMoving) {
    const double half = parcelWidth(parcel.amount) / 2;
    const auto clear = recoverFromTerrain(world, parcel.position, half);
    if (!clear) return false;
    const Position from = *clear;
    Position to = from;
    for (int axis = 0; axis < 3; ++axis) to[axis] += parcel.velocity[axis] * step;
    to[1] -= 0.5 * Gravity * step * step;
    parcel.velocity[1] -= Gravity * step;

    const auto moving = physics ? physics->sweepMoving(from, to, half) : std::nullopt;
    const int samples = sweepSamples(from, to);
    Position lastFree = from;
    for (int i = 1; i <= samples; ++i) {
        const double fraction = i / double(samples);
        if (moving && moving->fraction <= fraction) {
            parcel.position = interpolate(from, to, std::max(0.0, moving->fraction - 0.005));
            parcel.velocity = rebound(parcel.velocity, *moving, 0.17);
            if (std::abs(moving->normal[1]) > 0.55) {
                const double side = ((cellAt(from).x ^ cellAt(from).z) & 1) ? 1 : -1;
                parcel.velocity[0] += 0.6 * side;
            }
            for (int axis = 0; axis < 3; ++axis) parcel.position[axis] += moving->normal[axis] * 0.003;
            return false;
        }
        const Position sample = interpolate(from, to, fraction);
        if (poolContact(state, sample, half) || touchesTerrain(world, sample, half)) {
            if (receive(state, world, parcel, lastFree)) {
                if (parcel.velocity[1] < -1.4 && state.foam.size() < 768)
                    state.foam.push_back({lastFree, 0.27});
                return true;
            }
            parcel.position = lastFree;
            parcel.velocity = {0, std::max(0.14, -parcel.velocity[1] * 0.12), 0};
            return false;
        }
        lastFree = sample;
    }
    parcel.position = to;
    const double upper = to[1] + half;
    if (!belowTerrain(world, upper) || upper >= lowestMoving || parcel.velocity[1] >= 0) return false;
    state.budget.airborne -= uint64_t(parcel.amount);
    state.budget.escaped += uint64_t(parcel.amount);
    return true;
}

} // namespace

void moveWaterParcels(WaterState& state, const VoxelWorld& world, double step,
                      const JoltRuntime* physics, double lowestMoving) {
    for (auto& foam : state.foam) foam.life -= step;
    std::erase_if(state.foam, [](const WaterFoam& foam) { return foam.life <= 0; });
    std::erase_if(state.parcels, [&](WaterParcel& parcel) {
        return advanceParcel(state, world, parcel, step, physics, lowestMoving);
    });
}

void projectWaterParticles(ParticleProjection& output, const WaterState& state, Bounds visible) {
    for (const auto& parcel : state.parcels) {
        if (!visible.contains(cellAt(parcel.position))) continue;
        appendParticle(output, float(parcel.position[0]), float(parcel.position[1]), float(parcel.position[2]),
                       float(parcelWidth(parcel.amount)), ParticleKind::Water);
    }
    for (const auto& foam : state.foam) {
        if (!visible.contains(cellAt(foam.position))) continue;
        const float lift = float(0.10 * (0.27 - foam.life));
        appendParticle(output, float(foam.position[0]), float(foam.position[1]) + lift, float(foam.position[2]),
                       float(0.034 + foam.life * 0.21), ParticleKind::Foam);
    }
}

} // namespace derivean
