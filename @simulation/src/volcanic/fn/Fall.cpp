#include "volcanic/fn/Emplace.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr double Pi = 3.14159265358979323846;
constexpr double ConeReach = 9.0;    // e-folding distances beyond which the fall of a cone is not followed
constexpr int Finest = 6;            // samples per cell side where a cell is wider than the fall it holds

// Fall that slides: it holds any slope up to its angle of rest, whatever its thickness above a
// hand's breadth, and it does not set by cooling.
Rheology looseAt(float reposeTan) { return {reposeTan * 0.3f, reposeTan * 0.3f, 0.3f, 1e30f}; }

} // namespace

FallLaw fallLaw(const Fall& fall) {
    const double lean = dot(fall.stretch, fall.stretch);
    FallLaw law;
    law.peak = std::pow(1.0 - lean, 1.5) / (2.0 * Pi * fall.length * fall.length);
    if (fall.hole > 0) {
        const double inner = fall.length * fall.hole / (fall.length + fall.hole);
        law.whole = fall.length * fall.length - inner * inner;
    }
    return law;
}

double fallThickness(const Fall& fall, const FallLaw& law, Vec2 at) {
    const Vec2 offset = at - fall.vent;
    const double distance = length(offset);
    // An ellipse of equal thickness has the vent at one focus; without wind it is a circle.
    const double blanket = law.peak * std::exp(-(distance - dot(fall.stretch, offset)) / fall.length);
    if (fall.hole <= 0) return fall.volume * blanket;
    // Around the jet the same law with a hollow over the vent, normalised to the same volume.
    return fall.volume * blanket * (1.0 - std::exp(-distance / fall.hole)) * fall.length * fall.length / law.whole;
}

double fallThickness(const Fall& fall, Vec2 at) { return fallThickness(fall, fallLaw(fall), at); }

Fall coneFall(Vec2 vent, double volume) {
    Fall fall;
    fall.vent = vent;
    fall.volume = volume;
    fall.length = ConeLength * std::cbrt(volume);
    fall.hole = VentShare * fall.length;
    return fall;
}

Fall ashFall(Vec2 vent, double volume, Vec2 wind) {
    Fall fall;
    fall.vent = vent;
    fall.volume = volume;
    fall.length = AshLength * std::cbrt(volume / 1e9);
    const double speed = length(wind);
    fall.stretch = normalized(wind) * (speed / (speed + WindStretch));
    return fall;
}

void buildCone(Emplacement& field, const Fall& fall, float reposeTan) {
    if (fall.volume <= 0) return;
    // Summed over the world's own lattice of cells, also where the grid does not reach: every grid
    // that holds a cell gives it the same share, and the shares of all cells are the whole volume.
    const int reach = int(std::ceil(ConeReach * fall.length / field.cell)) + 1;
    const int samples = std::clamp(int(std::ceil(3.0 * field.cell / fall.length)), 1, Finest);
    const int64_t ventX = int64_t(std::floor(fall.vent.x / field.cell)), ventZ = int64_t(std::floor(fall.vent.z / field.cell));
    const int64_t firstX = int64_t(std::llround(field.origin.x / field.cell)), firstZ = int64_t(std::llround(field.origin.z / field.cell));
    const int side = 2 * reach + 1;
    std::vector<double> share(size_t(side) * size_t(side));
    const FallLaw law = fallLaw(fall);
    double total = 0;
    for (int dz = -reach; dz <= reach; ++dz) {
        for (int dx = -reach; dx <= reach; ++dx) {
            double sum = 0;
            for (int sz = 0; sz < samples; ++sz)
                for (int sx = 0; sx < samples; ++sx)
                    sum += fallThickness(fall, law, {(double(ventX + dx) + (sx + 0.5) / samples) * field.cell, (double(ventZ + dz) + (sz + 0.5) / samples) * field.cell});
            share[size_t(dz + reach) * size_t(side) + size_t(dx + reach)] = sum;
            total += sum;
        }
    }
    if (total <= 0) return;
    const double scale = fall.volume / (total * field.cell * field.cell);
    Worklist work;
    for (int dz = -reach; dz <= reach; ++dz) {
        for (int dx = -reach; dx <= reach; ++dx) {
            const int x = int(ventX + dx - firstX), z = int(ventZ + dz - firstZ);
            const double thickness = share[size_t(dz + reach) * size_t(side) + size_t(dx + reach)] * scale;
            if (thickness <= 0) continue;
            if (!field.contains(x, z) || field.rest[field.index(x, z)] == Rest::Closed) field.lost += thickness * field.cell * field.cell;
            else feed(field, work, field.index(x, z), float(thickness));
        }
    }
    relax(field, looseAt(reposeTan), work);
    set(field, work);
}

} // namespace derivean::sim
