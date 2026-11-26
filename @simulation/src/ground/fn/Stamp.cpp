#include "ground/fn/Stamp.hpp"

#include "ground/type/Detail.hpp"

namespace derivean::sim {
namespace {

// Whether a point, given from the stone's centre, lies in the stone.
struct Shape {
    float cy, sy, cp, sp, cr, sr, ia, ib, ic;
    int exponent;

    explicit Shape(const Clast& clast)
        : cy(std::cos(clast.yaw)), sy(std::sin(clast.yaw)), cp(std::cos(clast.pitch)), sp(std::sin(clast.pitch)), cr(std::cos(clast.roll)), sr(std::sin(clast.roll)),
          ia(1.0f / clast.a), ib(1.0f / clast.b), ic(1.0f / clast.c), exponent(clast.exponent()) {}

    bool holds(float dx, float dy, float dz) const {
        // Into the stone's frame: heading, then the dip of the long axis, then the lean about it.
        const float along = cy * dx + sy * dz, across = -sy * dx + cy * dz;
        const float up = -sp * along + cp * dy;
        const float u = (cp * along + sp * dy) * ia, v = (cr * across + sr * up) * ib, w = (-sr * across + cr * up) * ic;
        float s = u * u, t = w * w, q = v * v;
        if (exponent == 4) s *= s, t *= t, q *= q;
        else if (exponent == 6) s *= s * s, t *= t * t, q *= q * q;
        return s + t + q <= 1.0f;
    }
};

} // namespace

const Clast* clastAt(const Tile& tile, double x, double y, double z, const std::function<float(double, double)>* lift) {
    if (!tile.detail || tile.detail->clasts.empty()) return nullptr;
    const GroundDetail& detail = *tile.detail;
    const double span = tileSpan(Level::Ground), cornerX = tile.key.x * span, cornerZ = tile.key.z * span;
    const float localX = float(x - cornerX), localZ = float(z - cornerZ), reach = float(ClastReach);
    const auto binOf = [](float position) { return std::clamp(int(std::floor(position / float(ClastBinSpan))), 0, ClastBins - 1); };
    // The last stone stamped over a point is the one it reads as: the same order here.
    const Clast* found = nullptr;
    for (int bz = binOf(localZ - reach); bz <= binOf(localZ + reach); ++bz)
        for (int bx = binOf(localX - reach); bx <= binOf(localX + reach); ++bx) {
            const size_t bin = size_t(bz) * ClastBins + size_t(bx);
            for (uint32_t at = detail.binStart[bin]; at < detail.binStart[bin + 1]; ++at) {
                const Clast& clast = detail.clasts[at];
                const double centreY = double(clast.y) + (lift ? double((*lift)(cornerX + double(clast.x), cornerZ + double(clast.z))) : 0.0);
                if (Shape(clast).holds(float(x - (cornerX + double(clast.x))), float(y - centreY), float(z - (cornerZ + double(clast.z))))) found = &clast;
            }
        }
    return found;
}

void stampClasts(const Tile& tile, const CubeLattice& lattice, Material* materials, const std::function<float(double, double)>* lift) {
    if (!tile.detail || tile.detail->clasts.empty()) return;
    const GroundDetail& detail = *tile.detail;
    const double span = tileSpan(Level::Ground);
    // Bounds of the lattice in the tile's own frame, for choosing stones; the points themselves are
    // taken in double, so that a point reads the same through any lattice that has it for a centre.
    const double cornerX = tile.key.x * span, cornerZ = tile.key.z * span;
    const float step = float(lattice.step);
    const float lowX = float(lattice.originX - cornerX), lowZ = float(lattice.originZ - cornerZ), lowY = float(lattice.originY);
    const float highX = lowX + lattice.countX * step, highZ = lowZ + lattice.countZ * step, highY = lowY + lattice.countY * step;
    const auto binOf = [](float position) { return std::clamp(int(std::floor(position / float(ClastBinSpan))), 0, ClastBins - 1); };
    const float reach = float(ClastReach) + step;
    if (highX + reach < 0 || highZ + reach < 0 || lowX - reach > span || lowZ - reach > span) return;

    std::vector<float> dxs(size_t(lattice.countX)), dys(size_t(lattice.countY)), dzs(size_t(lattice.countZ));
    for (int bz = binOf(lowZ - reach); bz <= binOf(highZ + reach); ++bz) {
        for (int bx = binOf(lowX - reach); bx <= binOf(highX + reach); ++bx) {
            const size_t bin = size_t(bz) * ClastBins + size_t(bx);
            for (uint32_t at = detail.binStart[bin]; at < detail.binStart[bin + 1]; ++at) {
                Clast clast = detail.clasts[at];
                if (lift) clast.y += (*lift)(cornerX + double(clast.x), cornerZ + double(clast.z));
                const float r = clast.reach() + step;
                if (clast.x + r < lowX || clast.x - r > highX || clast.z + r < lowZ || clast.z - r > highZ || clast.y + r < lowY || clast.y - r > highY) continue;
                // Cubes whose centres can lie inside, a cube to spare on every side; centre k is at origin + (k + 0.5) * step.
                const auto first = [&](float low, float centre) { return std::max(0, int(std::floor((centre - r - low) / step))); };
                const auto last = [&](float low, float centre, int count) { return std::min(count - 1, int(std::ceil((centre + r - low) / step))); };
                const int x0 = first(lowX, clast.x), x1 = last(lowX, clast.x, lattice.countX), y0 = first(lowY, clast.y), y1 = last(lowY, clast.y, lattice.countY);
                const int z0 = first(lowZ, clast.z), z1 = last(lowZ, clast.z, lattice.countZ);
                for (int ix = x0; ix <= x1; ++ix) dxs[size_t(ix)] = float(lattice.originX + (ix + 0.5) * lattice.step - (cornerX + double(clast.x)));
                for (int iy = y0; iy <= y1; ++iy) dys[size_t(iy)] = float(lattice.originY + (iy + 0.5) * lattice.step - double(clast.y));
                for (int iz = z0; iz <= z1; ++iz) dzs[size_t(iz)] = float(lattice.originZ + (iz + 0.5) * lattice.step - (cornerZ + double(clast.z)));
                const Shape shape(clast);
                for (int iy = y0; iy <= y1; ++iy) {
                    const float dy = dys[size_t(iy)];
                    for (int iz = z0; iz <= z1; ++iz) {
                        const float dz = dzs[size_t(iz)];
                        Material* row = materials + (size_t(iy) * size_t(lattice.countZ) + size_t(iz)) * size_t(lattice.countX);
                        for (int ix = x0; ix <= x1; ++ix) {
                            if (shape.holds(dxs[size_t(ix)], dy, dz)) row[ix] = clast.rock;
                        }
                    }
                }
            }
        }
    }
}

} // namespace derivean::sim
