#include "core/fn/Noise.hpp"
#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

int64_t lattice(int64_t index, int period) {
    if (period <= 0) return index;
    const int64_t rest = index % period;
    return rest < 0 ? rest + period : rest;
}

double corner(uint64_t seed, int64_t ix, int64_t iz, double dx, double dz, int period) {
    const uint64_t bits = hash(seed, lattice(ix, period), lattice(iz, period));
    // Sixteen evenly spaced gradient directions from a table-free integer hash.
    static constexpr double table[16][2] = {
        {1, 0}, {0.9239, 0.3827}, {0.7071, 0.7071}, {0.3827, 0.9239}, {0, 1}, {-0.3827, 0.9239},
        {-0.7071, 0.7071}, {-0.9239, 0.3827}, {-1, 0}, {-0.9239, -0.3827}, {-0.7071, -0.7071},
        {-0.3827, -0.9239}, {0, -1}, {0.3827, -0.9239}, {0.7071, -0.7071}, {0.9239, -0.3827}};
    const auto& gradient = table[bits & 15];
    return gradient[0] * dx + gradient[1] * dz;
}

} // namespace

double gradientNoise(uint64_t seed, double x, double z, int period) {
    const double fx = std::floor(x), fz = std::floor(z);
    const int64_t ix = int64_t(fx), iz = int64_t(fz);
    const double dx = x - fx, dz = z - fz;
    const double u = fade(dx), v = fade(dz);
    const double a = corner(seed, ix, iz, dx, dz, period);
    const double b = corner(seed, ix + 1, iz, dx - 1, dz, period);
    const double c = corner(seed, ix, iz + 1, dx, dz - 1, period);
    const double d = corner(seed, ix + 1, iz + 1, dx - 1, dz - 1, period);
    return 1.41421356 * mix(mix(a, b, u), mix(c, d, u), v);
}

double fractalNoise(uint64_t seed, double x, double z, int octaves, double gain, int period) {
    double sum = 0, amplitude = 1, total = 0;
    for (int octave = 0; octave < octaves; ++octave) {
        sum += amplitude * gradientNoise(seed + uint64_t(octave) * 0x51ed27, x, z, period);
        total += amplitude;
        amplitude *= gain;
        x *= 2;
        z *= 2;
        period *= 2;
    }
    return sum / total;
}

double ridgedNoise(uint64_t seed, double x, double z, int octaves, double gain, int period) {
    double sum = 0, amplitude = 1, total = 0;
    for (int octave = 0; octave < octaves; ++octave) {
        const double ridge = 1 - std::abs(gradientNoise(seed + uint64_t(octave) * 0x7f4a7c, x, z, period));
        sum += amplitude * ridge * ridge;
        total += amplitude;
        amplitude *= gain;
        x *= 2;
        z *= 2;
        period *= 2;
    }
    return sum / total;
}

double valueNoise3(uint64_t seed, double x, double y, double z) {
    const double fx = std::floor(x), fy = std::floor(y), fz = std::floor(z);
    const int64_t ix = int64_t(fx), iy = int64_t(fy), iz = int64_t(fz);
    const double u = fade(x - fx), v = fade(y - fy), w = fade(z - fz);
    const auto at = [&](int ox, int oy, int oz) {
        return unit(hash(seed, ix + ox, iy + oy, iz + oz)) * 2 - 1;
    };
    const double low = mix(mix(at(0, 0, 0), at(1, 0, 0), u), mix(at(0, 0, 1), at(1, 0, 1), u), w);
    const double high = mix(mix(at(0, 1, 0), at(1, 1, 0), u), mix(at(0, 1, 1), at(1, 1, 1), u), w);
    return mix(low, high, v);
}

CellularSite cellularNoise(uint64_t seed, double x, double z, double jitter, int period) {
    const int64_t ix = int64_t(std::floor(x)), iz = int64_t(std::floor(z));
    CellularSite result;
    result.nearest = result.second = 1e30;
    for (int64_t cz = iz - 2; cz <= iz + 2; ++cz) {
        for (int64_t cx = ix - 2; cx <= ix + 2; ++cx) {
            const int64_t keyX = lattice(cx, period), keyZ = lattice(cz, period);
            const Vec2 site{double(cx) + 0.5 + jitter * (unit(hash(seed, keyX, keyZ, 1)) - 0.5),
                            double(cz) + 0.5 + jitter * (unit(hash(seed, keyX, keyZ, 2)) - 0.5)};
            const double distance = length(site - Vec2{x, z});
            if (distance < result.nearest) {
                result.second = result.nearest;
                result.otherX = result.cellX;
                result.otherZ = result.cellZ;
                result.other = result.site;
                result.nearest = distance;
                result.cellX = keyX;
                result.cellZ = keyZ;
                result.site = site;
            } else if (distance < result.second) {
                result.second = distance;
                result.otherX = keyX;
                result.otherZ = keyZ;
                result.other = site;
            }
        }
    }
    return result;
}

} // namespace derivean::sim
