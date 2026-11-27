#include "atmosphere/fn/PoissonLevel.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SphereStencil.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// The V-cycle's coarsest level (24 cells), solved by plain CG.
constexpr int CoarsestLevel = 1;
constexpr int PowerSteps = 12;

// λmax of D⁻¹K by power iteration from the grid-scale checkerboard, which lies close to the top of
// the spectrum; the Rayleigh quotient xᵀKx / xᵀDx approaches λmax from below.
double largestEigenvalue(PoissonLevel& level) {
    const size_t n = level.inverseDiagonal.size();
    std::vector<double>& x = level.direction;
    std::vector<double>& kx = level.spare;
    for (size_t c = 0; c < n; ++c) {
        const int rest = int(c) % (level.side * level.side);
        x[c] = ((rest % level.side + rest / level.side) % 2 == 0 ? 1.0 : -1.0) + 1e-3 * double(c % 7);
    }
    double estimate = 0;
    for (int step = 0; step < PowerSteps; ++step) {
        applyMatrix(level.stiffness, x, kx);
        double kxx = 0, dxx = 0, largest = 0;
        for (size_t c = 0; c < n; ++c) {
            const double energy = kx[c] * x[c];   // unfused on every platform, as in dotOf
            kxx += energy;
            dxx += x[c] * x[c] / level.inverseDiagonal[c];
        }
        estimate = kxx / dxx;
        for (size_t c = 0; c < n; ++c) {
            x[c] = level.inverseDiagonal[c] * kx[c];
            largest = std::max(largest, std::abs(x[c]));
        }
        for (double& value : x) value /= largest;
    }
    return estimate;
}

} // namespace

PoissonLevel makePoissonLevel(const SphereGrid& grid, const SphereStencil& stencil, LevelUse use) {
    PoissonLevel level;
    level.level = grid.level;
    level.side = grid.side;
    SphereMatrix laplacian = makeLaplacian(grid, stencil);
    shareFaceInteriors(laplacian, grid.side);
    level.stiffness = makeStencilMatrix(laplacian, grid.side);
    const size_t n = size_t(grid.cells);
    level.inverseDiagonal.resize(n);
    for (size_t c = 0; c < n; ++c) level.inverseDiagonal[c] = 1.0 / level.stiffness.diagonal[c];
    if (use == LevelUse::Diagonal) return level;
    level.residual.assign(n, 0.0);
    level.direction = level.spare = level.residual;
    if (use != LevelUse::Finest) level.solution = level.source = level.residual;
    if (use != LevelUse::Coarsest) level.largest = largestEigenvalue(level);
    return level;
}

std::vector<PoissonLevel> coarserLevels(int finest) {
    std::vector<PoissonLevel> levels;
    for (int l = finest - 1; l >= CoarsestLevel; --l) {
        const SphereGrid grid = makeSphereGrid(l);
        levels.push_back(makePoissonLevel(grid, makeSphereStencil(grid), l == CoarsestLevel ? LevelUse::Coarsest : LevelUse::Coarser));
    }
    return levels;
}

std::vector<PoissonLevelOf<float>> narrowLevels(const std::vector<PoissonLevel>& levels) {
    assert(levels.size() >= 2);
    std::vector<PoissonLevelOf<float>> narrow(levels.size());
    for (size_t at = 0; at < levels.size(); ++at) {
        const PoissonLevel& wide = levels[at];
        PoissonLevelOf<float>& level = narrow[at];
        level.level = wide.level;
        level.side = wide.side;
        const size_t n = wide.inverseDiagonal.size();
        level.solution.assign(n, 0.0f);
        level.source = level.solution;
        if (at + 1 == levels.size()) break;
        level.stiffness = narrowed(wide.stiffness);
        level.inverseDiagonal.assign(wide.inverseDiagonal.begin(), wide.inverseDiagonal.end());
        level.largest = wide.largest;
        level.residual = level.direction = level.spare = level.solution;
    }
    return narrow;
}

} // namespace derivean::sim
