#include "geometry/fn/Cell.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Projection.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>

namespace derivean::sim {

namespace {

int indexOf(double st, int level) {
    const int count = cellsPerSide(level);
    return std::clamp(int(std::floor(st * count)), 0, count - 1);
}

double uvAt(int index, int level) { return stToUv(double(index) / cellsPerSide(level)); }

// Spreads the low 30 bits of a value to the even bits of the result.
std::uint64_t spread(std::uint32_t value) {
    std::uint64_t x = value;
    x = (x | (x << 16)) & 0x0000FFFF0000FFFFull;
    x = (x | (x << 8)) & 0x00FF00FF00FF00FFull;
    x = (x | (x << 4)) & 0x0F0F0F0F0F0F0F0Full;
    x = (x | (x << 2)) & 0x3333333333333333ull;
    x = (x | (x << 1)) & 0x5555555555555555ull;
    return x;
}

std::uint32_t gather(std::uint64_t x) {
    x &= 0x5555555555555555ull;
    x = (x | (x >> 1)) & 0x3333333333333333ull;
    x = (x | (x >> 2)) & 0x0F0F0F0F0F0F0F0Full;
    x = (x | (x >> 4)) & 0x00FF00FF00FF00FFull;
    x = (x | (x >> 8)) & 0x0000FFFF0000FFFFull;
    x = (x | (x >> 16)) & 0x00000000FFFFFFFFull;
    return std::uint32_t(x);
}

} // namespace

bool isValid(const Cell& cell) {
    if (cell.face < 0 || cell.face >= FaceCount || cell.level < 0 || cell.level > MaxLevel) return false;
    const int count = cellsPerSide(cell.level);
    return cell.i >= 0 && cell.i < count && cell.j >= 0 && cell.j < count;
}

Cell cellOf(FacePoint point, int level) {
    return {point.face, level, indexOf(uvToSt(point.u), level), indexOf(uvToSt(point.v), level)};
}

Cell cellOf(Vec3 direction, int level) { return cellOf(facePointOf(direction), level); }

FacePoint centrePointOf(const Cell& cell) {
    const double count = cellsPerSide(cell.level);
    return {cell.face, stToUv((cell.i + 0.5) / count), stToUv((cell.j + 0.5) / count)};
}

Vec3 centreOf(const Cell& cell) { return directionOf(centrePointOf(cell)); }

Vec3 cornerOf(const Cell& cell, int corner) {
    const int di = corner == 1 || corner == 2, dj = corner >= 2;
    return directionOf({cell.face, uvAt(cell.i + di, cell.level), uvAt(cell.j + dj, cell.level)});
}

Cell parentOf(const Cell& cell, int level) {
    assert(level >= 0 && level <= cell.level);
    const int shift = cell.level - level;
    return {cell.face, level, cell.i >> shift, cell.j >> shift};
}

Cell childOf(const Cell& cell, int k) {
    assert(cell.level < MaxLevel);
    return {cell.face, cell.level + 1, 2 * cell.i + (k & 1), 2 * cell.j + ((k >> 1) & 1)};
}

std::uint64_t keyOf(const Cell& cell) {
    const std::uint64_t position = (spread(std::uint32_t(cell.i)) << 1) | spread(std::uint32_t(cell.j));
    const int low = 60 - 2 * cell.level;   // the sentinel's bit
    return (std::uint64_t(cell.face) << 61) | (position << (low + 1)) | (std::uint64_t(1) << low);
}

Cell cellOfKey(std::uint64_t key) {
    assert(key != 0);
    const int low = std::countr_zero(key);
    const int level = (60 - low) / 2;
    const std::uint64_t position = (key & ((std::uint64_t(1) << 61) - 1)) >> (low + 1);
    return {int(key >> 61), level, int(gather(position >> 1)), int(gather(position))};
}

} // namespace derivean::sim
