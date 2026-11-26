#include "atmosphere/fn/SphereMultigrid.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SphereStencil.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// A multigrid V-cycle (Brandt 1977) on the quadtree's levels; the coarsest is solved by plain CG.
constexpr int CoarsestLevel = 1;

// One Gauss–Seidel sweep over the rows, forward or backward.
void sweep(const SphereMatrix& k, const std::vector<double>& b, std::vector<double>& x, bool forward) {
    const int rows = int(k.diagonal.size());
    for (int step = 0; step < rows; ++step) {
        const int r = forward ? step : rows - 1 - step;
        double sum = b[size_t(r)];
        for (int e = k.start[size_t(r)]; e < k.start[size_t(r) + 1]; ++e)
            if (k.column[size_t(e)] != r) sum -= k.value[size_t(e)] * x[size_t(k.column[size_t(e)])];
        x[size_t(r)] = sum / k.diagonal[size_t(r)];
    }
}

// The coarsest level is solved by plain CG to rounding; its 24 cells need at most a few dozen steps.
void solveCoarsest(PoissonLevel& level) {
    const size_t n = level.source.size();
    double mean = 0;
    for (double value : level.source) mean += value;
    mean /= double(n);
    std::vector<double> r(n), p(n), q(n);
    for (size_t c = 0; c < n; ++c) {
        level.solution[c] = 0;
        r[c] = level.source[c] - mean;
    }
    p = r;
    double rr = dotOf(r, r);
    const double floor = rr * 1e-28;
    for (int step = 0; step < int(4 * n) && rr > floor; ++step) {
        applyMatrix(level.stiffness, p, q);
        const double alpha = rr / dotOf(p, q);
        for (size_t c = 0; c < n; ++c) {
            level.solution[c] += alpha * p[c];
            r[c] -= alpha * q[c];
        }
        const double next = dotOf(r, r);
        for (size_t c = 0; c < n; ++c) p[c] = r[c] + next / rr * p[c];
        rr = next;
    }
}

int parentIndex(const PoissonLevel& fine, int cell) {
    const int perFace = fine.side * fine.side, face = cell / perFace, rest = cell % perFace;
    const int i = rest % fine.side, j = rest / fine.side, half = fine.side / 2;
    return (face * half + j / 2) * half + i / 2;
}

void cycle(std::vector<PoissonLevel>& levels, size_t at, int smoothing) {
    PoissonLevel& level = levels[at];
    if (at + 1 == levels.size()) {
        solveCoarsest(level);
        return;
    }
    std::fill(level.solution.begin(), level.solution.end(), 0.0);
    for (int s = 0; s < smoothing; ++s) sweep(level.stiffness, level.source, level.solution, true);
    applyMatrix(level.stiffness, level.solution, level.residual);
    PoissonLevel& coarse = levels[at + 1];
    std::fill(coarse.source.begin(), coarse.source.end(), 0.0);
    for (size_t c = 0; c < level.residual.size(); ++c)
        coarse.source[size_t(parentIndex(level, int(c)))] += level.source[c] - level.residual[c];
    cycle(levels, at + 1, smoothing);
    for (size_t c = 0; c < level.solution.size(); ++c) level.solution[c] += coarse.solution[size_t(parentIndex(level, int(c)))];
    for (int s = 0; s < smoothing; ++s) sweep(level.stiffness, level.source, level.solution, false);
}

} // namespace

std::vector<PoissonLevel> coarserLevels(int finest) {
    std::vector<PoissonLevel> levels;
    for (int l = finest - 1; l >= CoarsestLevel; --l) {
        const SphereGrid grid = makeSphereGrid(l);
        PoissonLevel level;
        level.level = l;
        level.side = grid.side;
        level.stiffness = makeLaplacian(grid, makeSphereStencil(grid));
        level.solution.assign(size_t(grid.cells), 0.0);
        level.source = level.solution;
        level.residual = level.solution;
        levels.push_back(std::move(level));
    }
    return levels;
}

void vCycle(std::vector<PoissonLevel>& levels, int smoothing, const std::vector<double>& r, std::vector<double>& z) {
    levels[0].source = r;
    cycle(levels, 0, smoothing);
    z = levels[0].solution;
}

} // namespace derivean::sim
