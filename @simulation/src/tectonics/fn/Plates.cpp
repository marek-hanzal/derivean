#include "tectonics/type/Plates.hpp"

#include "chronology/constant/Timeline.hpp"
#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "planet/constant/Scale.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t EpochStream = 0x45504f43;
constexpr double Transition = 12 * Myr;
constexpr double TwoPi = 6.283185307179586;

double spacing(const Epoch& epoch) { return PlanetSpan / epoch.lattice; }

// Bends the straight Voronoi contacts; the same warp serves every query of the epoch.
Vec2 warped(const Epoch& epoch, Vec2 position) {
    const double scale = 1.0 / spacing(epoch);
    const double u = position.x * scale, v = position.z * scale;
    const int period = epoch.lattice;
    const double broadX = fractalNoise(epoch.seed + 11, u, v, 3, 0.5, period);
    const double broadZ = fractalNoise(epoch.seed + 12, u, v, 3, 0.5, period);
    const double fineX = fractalNoise(epoch.seed + 13, u * 6, v * 6, 3, 0.5, period * 6);
    const double fineZ = fractalNoise(epoch.seed + 14, u * 6, v * 6, 3, 0.5, period * 6);
    return {u + 0.42 * broadX + 0.06 * fineX, v + 0.42 * broadZ + 0.06 * fineZ};
}

Vec2 plateVelocity(const Epoch& epoch, const Plate& plate, Vec2 lattice) {
    const double size = spacing(epoch);
    const Vec2 arm{wrapDelta(lattice.x - plate.site.x, epoch.lattice) * size,
                   wrapDelta(lattice.z - plate.site.z, epoch.lattice) * size};
    return plate.velocity + perpendicular(arm) * plate.spin;
}

} // namespace

Epoch epochAt(uint64_t worldSeed, double time) {
    const uint64_t seed = stream(worldSeed, EpochStream);
    Epoch epoch;
    epoch.start = TectonicsStart;
    for (int index = 0;; ++index) {
        const double length = ranged(hash(seed, index, 1), 140 * Myr, 360 * Myr);
        epoch.index = index;
        epoch.end = epoch.start + length;
        if (time < epoch.end || index > 64) break;
        epoch.start = epoch.end;
    }
    epoch.seed = hash(seed, epoch.index, 2);
    epoch.lattice = 3 + int(hash(epoch.seed, 3) % 3);
    epoch.plates.resize(size_t(epoch.lattice) * size_t(epoch.lattice));
    for (int z = 0; z < epoch.lattice; ++z) {
        for (int x = 0; x < epoch.lattice; ++x) {
            Plate& plate = epoch.plates[size_t(z) * size_t(epoch.lattice) + size_t(x)];
            plate.site = {x + 0.5 + 0.8 * (unit(epoch.seed, x, z, 4) - 0.5), z + 0.5 + 0.8 * (unit(epoch.seed, x, z, 5) - 0.5)};
            plate.reach = ranged(hash(epoch.seed, x, z, 6), 0.62, 1.55);
            const double heading = TwoPi * unit(epoch.seed, x, z, 7);
            // 4 to 30 km/Myr; the top speed keeps one step below one planet cell of travel.
            const double speed = ranged(hash(epoch.seed, x, z, 8), 0.004, 0.030);
            plate.velocity = Vec2{std::cos(heading), std::sin(heading)} * speed;
            plate.spin = ranged(hash(epoch.seed, x, z, 9), -3e-9, 3e-9);
        }
    }
    return epoch;
}

double epochActivity(const Epoch& epoch, double time) {
    return smoothstep(0, Transition, time - epoch.start) * smoothstep(0, Transition, epoch.end - time);
}

PlateSample samplePlate(const Epoch& epoch, Vec2 position) {
    const Vec2 lattice = warped(epoch, position);
    const int baseX = floorToInt(lattice.x), baseZ = floorToInt(lattice.z);
    double best = 1e30, next = 1e30;
    int bestId = 0, nextId = 0;
    Vec2 bestArm{}, nextArm{};
    for (int cz = baseZ - 2; cz <= baseZ + 2; ++cz) {
        for (int cx = baseX - 2; cx <= baseX + 2; ++cx) {
            const int id = wrapIndex(cz, epoch.lattice) * epoch.lattice + wrapIndex(cx, epoch.lattice);
            const Plate& plate = epoch.plates[size_t(id)];
            const Vec2 site{plate.site.x + (cx - wrapIndex(cx, epoch.lattice)), plate.site.z + (cz - wrapIndex(cz, epoch.lattice))};
            const Vec2 arm = lattice - site;
            const double distance = length(arm) / plate.reach;
            if (distance < best) {
                next = best;
                nextId = bestId;
                nextArm = bestArm;
                best = distance;
                bestId = id;
                bestArm = arm;
            } else if (distance < next) {
                next = distance;
                nextId = id;
                nextArm = arm;
            }
        }
    }
    const Plate& own = epoch.plates[size_t(bestId)];
    const Plate& other = epoch.plates[size_t(nextId)];
    // The weighted distances are equal on the contact; their gradient turns the difference into metres.
    const Vec2 gradient = normalized(nextArm) * (1.0 / other.reach) - normalized(bestArm) * (1.0 / own.reach);
    const double steepness = std::max(1e-6, length(gradient));

    PlateSample sample;
    sample.plate = bestId;
    sample.neighbor = nextId;
    sample.boundaryDistance = (next - best) / steepness * spacing(epoch);
    sample.normal = gradient * (-1.0 / steepness);
    sample.velocity = plateVelocity(epoch, own, lattice);
    sample.neighborVelocity = plateVelocity(epoch, other, lattice);
    const Vec2 relative = sample.velocity - sample.neighborVelocity;
    sample.convergence = dot(relative, sample.normal);
    sample.shear = dot(relative, perpendicular(sample.normal));
    return sample;
}

double dynamicTopography(const Epoch& epoch, Vec2 position) {
    const double scale = 1.0 / spacing(epoch);
    return 380.0 * fractalNoise(epoch.seed + 21, position.x * scale * 1.5, position.z * scale * 1.5, 2, 0.5, epoch.lattice * 3) ;
}

} // namespace derivean::sim
