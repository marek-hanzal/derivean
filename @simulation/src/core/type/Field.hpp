#pragma once

#include "core/type/Vec.hpp"

#include <cstddef>
#include <vector>

namespace derivean::sim {

// Dense row-major grid. Index (x, z) addresses the cell whose centre lies half a cell in.
template<typename T>
struct Field {
    int width{}, height{};
    std::vector<T> cells;

    Field() = default;
    Field(int w, int h, T fill = T{}) : width(w), height(h), cells(size_t(w) * size_t(h), fill) {}

    size_t index(int x, int z) const { return size_t(z) * size_t(width) + size_t(x); }
    T& at(int x, int z) { return cells[index(x, z)]; }
    const T& at(int x, int z) const { return cells[index(x, z)]; }
    size_t size() const { return cells.size(); }
    bool contains(int x, int z) const { return x >= 0 && z >= 0 && x < width && z < height; }
};

// Catmull-Rom weights; the interpolant passes through its samples and is C1 between them.
inline void cubicWeights(double t, double weights[4]) {
    const double t2 = t * t, t3 = t2 * t;
    weights[0] = -0.5 * t3 + t2 - 0.5 * t;
    weights[1] = 1.5 * t3 - 2.5 * t2 + 1;
    weights[2] = -1.5 * t3 + 2 * t2 + 0.5 * t;
    weights[3] = 0.5 * t3 - 0.5 * t2;
}

// Uniform cubic B-spline weights; smooth (C2) and never overshoots, at the cost of some blur.
inline void splineWeights(double t, double weights[4]) {
    const double t2 = t * t, t3 = t2 * t;
    weights[0] = (1 - 3 * t + 3 * t2 - t3) / 6;
    weights[1] = (4 - 6 * t2 + 3 * t3) / 6;
    weights[2] = (1 + 3 * t + 3 * t2 - 3 * t3) / 6;
    weights[3] = t3 / 6;
}

enum class Edge { Clamp, Wrap };
enum class Kernel { Linear, Cubic, Spline };

// Samples at continuous cell-centre coordinates: (0, 0) is the centre of cell (0, 0).
template<typename T>
double sample(const Field<T>& field, double x, double z, Kernel kernel = Kernel::Cubic, Edge edge = Edge::Clamp) {
    const auto fetch = [&](int ix, int iz) -> double {
        if (edge == Edge::Wrap) return double(field.at(wrapIndex(ix, field.width), wrapIndex(iz, field.height)));
        return double(field.at(std::clamp(ix, 0, field.width - 1), std::clamp(iz, 0, field.height - 1)));
    };
    const double fx = std::floor(x), fz = std::floor(z);
    const int ix = int(fx), iz = int(fz);
    const double tx = x - fx, tz = z - fz;
    if (kernel == Kernel::Linear) {
        return mix(mix(fetch(ix, iz), fetch(ix + 1, iz), tx), mix(fetch(ix, iz + 1), fetch(ix + 1, iz + 1), tx), tz);
    }
    double wx[4], wz[4];
    if (kernel == Kernel::Cubic) {
        cubicWeights(tx, wx);
        cubicWeights(tz, wz);
    } else {
        splineWeights(tx, wx);
        splineWeights(tz, wz);
    }
    double sum = 0;
    for (int j = 0; j < 4; ++j) {
        double row = 0;
        for (int i = 0; i < 4; ++i) row += wx[i] * fetch(ix - 1 + i, iz - 1 + j);
        sum += wz[j] * row;
    }
    return sum;
}

} // namespace derivean::sim
