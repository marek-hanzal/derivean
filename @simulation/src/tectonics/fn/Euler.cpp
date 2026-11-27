#include "tectonics/fn/Euler.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Rotation.hpp"
#include "tectonics/type/Plates.hpp"

#include <cmath>
#include <numbers>

namespace derivean::sim {
namespace {

constexpr double Pi = std::numbers::pi;
constexpr double PacificSpeed = 0.0655;   // m/yr, RMS of the Pacific plate in NNR-MORVEL56 (Argus et al. 2011)
constexpr double EurasiaSpeed = 0.0228;   // m/yr, RMS of Eurasia, the slowest large continental plate

Vec3 unitOf(uint64_t seed, int a, int b) {
    const double z = 2 * unit(seed, a, b, 1) - 1, angle = 2 * Pi * unit(seed, a, b, 2), r = std::sqrt(std::max(0.0, 1 - z * z));
    return {r * std::cos(angle), r * std::sin(angle), z};
}

// Angle between neighbouring sites of n evenly spread on the sphere; plates split within the epoch are
// no sites.
double spacingOf(const EulerEpoch& epoch) { return std::sqrt(4 * Pi / double(epoch.sites ? epoch.sites : epoch.plates.size())); }

// Bends the great-circle contacts; the same warp serves every query of the epoch. Distances in units
// of the site spacing, as the flat warp's lattice units.
Vec3 warped(const EulerEpoch& epoch, Vec3 x) {
    const double s = spacingOf(epoch);
    const Vec3 p = x * (1 / s), fine = p * 6;
    const Vec3 broad{fractalNoise3(epoch.seed + 11, p.x, p.y, p.z, 3), fractalNoise3(epoch.seed + 12, p.x, p.y, p.z, 3),
                     fractalNoise3(epoch.seed + 13, p.x, p.y, p.z, 3)};
    const Vec3 detail{fractalNoise3(epoch.seed + 14, fine.x, fine.y, fine.z, 3), fractalNoise3(epoch.seed + 15, fine.x, fine.y, fine.z, 3),
                      fractalNoise3(epoch.seed + 16, fine.x, fine.y, fine.z, 3)};
    return normalized(x + broad * (0.42 * s) + detail * (0.06 * s));
}

} // namespace

EulerEpoch eulerEpochAt(uint64_t worldSeed, double time) {
    const Epoch flat = epochAt(worldSeed, time);
    EulerEpoch epoch;
    epoch.index = flat.index;
    epoch.start = flat.start;
    epoch.end = flat.end;
    epoch.seed = flat.seed;
    const int n = flat.lattice * flat.lattice;
    epoch.plates.resize(size_t(n));
    epoch.sites = size_t(n);
    // A Fibonacci lattice (González 2010) spreads n sites evenly; the epoch turns it at random and
    // jitters each site by up to 0.4 of the spacing, as the flat lattice is jittered by 0.4 of a cell.
    const Rotation turn = rotationAbout(unitOf(epoch.seed, 0, -1), 2 * Pi * unit(epoch.seed, 0, -1, 3));
    const double golden = Pi * (3 - std::sqrt(5.0)), s = spacingOf(epoch);
    for (int k = 0; k < n; ++k) {
        EulerPlate& plate = epoch.plates[size_t(k)];
        const double z = 1 - (2.0 * k + 1) / n, r = std::sqrt(1 - z * z), angle = golden * k;
        const Vec3 base = rotate(turn, Vec3{r * std::cos(angle), r * std::sin(angle), z});
        const Vec3 nudge = unitOf(epoch.seed, k, 4);
        const Vec3 tangent = normalized(nudge - base * dot(nudge, base));
        plate.site = normalized(base + tangent * (0.4 * s * unit(epoch.seed, k, 5, 0)));
        plate.reach = ranged(hash(epoch.seed, k, 6), 0.62, 1.55);
        plate.pole = unitOf(epoch.seed, k, 7);
        plate.draw = ranged(hash(epoch.seed, k, 10), 0.7, 1.3);
    }
    return epoch;
}

int plateAt(const EulerEpoch& epoch, Vec3 direction) {
    const Vec3 x = warped(epoch, direction);
    double best = 1e30;
    int plate = 0;
    const size_t sites = epoch.sites ? epoch.sites : epoch.plates.size();
    for (size_t k = 0; k < sites; ++k) {
        const double distance = centralAngle(x, epoch.plates[k].site) / epoch.plates[k].reach;
        if (distance < best) best = distance, plate = int(k);
    }
    return plate;
}

double swellAt(const EulerEpoch& epoch, Vec3 direction) {
    const Vec3 p = direction * (1.5 / spacingOf(epoch));
    return 380.0 * fractalNoise3(epoch.seed + 21, p.x, p.y, p.z, 2);
}

double targetSpeed(const EulerEpoch& epoch, int plate, double continentalShare, PlateSpeeds speeds) {
    if (speeds == PlateSpeeds::Today) return ranged(hash(epoch.seed, plate, 8), 0.004, 0.030);
    const double share = std::clamp(continentalShare, 0.0, 1.0);
    return (PacificSpeed + (EurasiaSpeed - PacificSpeed) * share) * epoch.plates[size_t(plate)].draw;
}

void setEuler(EulerEpoch& epoch, const std::vector<double>& meanSin2, const std::vector<double>& continentalShare, PlateSpeeds speeds) {
    for (size_t k = 0; k < epoch.plates.size(); ++k) {
        EulerPlate& plate = epoch.plates[k];
        plate.speed = targetSpeed(epoch, int(k), continentalShare[k], speeds);
        // A plate with no crust left keeps a finite rate (its <sin² γ> is that of the whole sphere).
        const double spread = meanSin2[k] > 1e-6 ? meanSin2[k] : 2.0 / 3.0;
        plate.euler = plate.pole * (plate.speed / (PlanetRadius * std::sqrt(spread)));
    }
}

Vec3 velocityAt(Vec3 euler, Vec3 direction) { return cross(euler, direction) * PlanetRadius; }

Closing closingAt(Vec3 own, Vec3 other, Vec3 direction, Vec3 normal) {
    const Vec3 relative = velocityAt(own - other, direction);
    return {dot(relative, normal), dot(relative, cross(direction, normal))};
}

void addInertia(Inertia& inertia, Vec3 x, double area) {
    inertia.xx += area * (1 - x.x * x.x);
    inertia.yy += area * (1 - x.y * x.y);
    inertia.zz += area * (1 - x.z * x.z);
    inertia.xy -= area * x.x * x.y;
    inertia.xz -= area * x.x * x.z;
    inertia.yz -= area * x.y * x.z;
}

Vec3 apply(const Inertia& m, Vec3 v) {
    return {m.xx * v.x + m.xy * v.y + m.xz * v.z, m.xy * v.x + m.yy * v.y + m.yz * v.z, m.xz * v.x + m.yz * v.y + m.zz * v.z};
}

std::vector<Vec3> withoutNetRotation(const std::vector<Vec3>& damped, const std::vector<Inertia>& inertia, Vec3* net) {
    Inertia m;
    Vec3 torque;
    for (size_t p = 0; p < damped.size(); ++p) {
        const Inertia& i = inertia[p];
        m.xx += i.xx, m.xy += i.xy, m.xz += i.xz, m.yy += i.yy, m.yz += i.yz, m.zz += i.zz;
        torque = torque + apply(i, damped[p]);
    }
    // M is symmetric positive definite (close to (8π/3) R² I for a full sphere): Cramer's rule.
    const Vec3 c0{m.xx, m.xy, m.xz}, c1{m.xy, m.yy, m.yz}, c2{m.xz, m.yz, m.zz};
    const double det = dot(c0, cross(c1, c2));
    Vec3 omega;
    if (std::abs(det) > 0) omega = Vec3{dot(torque, cross(c1, c2)), dot(c0, cross(torque, c2)), dot(c0, cross(c1, torque))} * (1 / det);
    if (net) *net = omega;
    std::vector<Vec3> applied(damped.size());
    for (size_t p = 0; p < damped.size(); ++p) applied[p] = damped[p] - omega;
    return applied;
}

} // namespace derivean::sim
