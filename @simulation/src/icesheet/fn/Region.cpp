#include "icesheet/fn/Region.hpp"

#include "core/service/Workers.hpp"
#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Balance.hpp"
#include "icesheet/fn/Flow.hpp"

#include <functional>

namespace derivean::sim {
namespace {

constexpr int Fold = SheetCoarsening;   // domain cells per side of a coarse cell
constexpr int FirstRounds = 480;          // most a stage is relaxed for the first time it is solved
constexpr int NextRounds = 48;            // and each later time, from where it was left
constexpr float Steady = 2e-4f;          // share of the ice volume forty rounds may still move
constexpr int SmoothRadius = 8;           // coarse cells over which the ground under the source's sheet is averaged
constexpr float SteepestSurface = 0.3f;
constexpr float SmallIce = 0.25f * LeastSheet;   // m of blended coarse ice below which a cell is beyond the sheet
constexpr int LeastRows = 16;

Vec2 nodeCentre(const Domain& domain, int x, int z) {
    return {domain.origin.x + (x + 0.5) * Fold * domain.cell, domain.origin.z + (z + 0.5) * Fold * domain.cell};
}

float groundOf(const Domain& domain, size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); }

// m of ice a finished neighbour recorded on a domain cell; negative where no finished tile owns it.
float recordedIce(const Domain& domain, int x, int z, int stage) {
    const int tx = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, tz = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
    const Tile* tile = tx || tz ? domain.finished[(tz + 1) * 3 + tx + 1] : nullptr;
    if (!tile) return -1;
    const size_t from = size_t(z - Halo - tz * TileCells) * TileCells + size_t(x - Halo - tx * TileCells);
    return stage == IceNow ? tile->ice[from] : tile->iceAge[from];
}

} // namespace

void boxMean(const std::vector<float>& field, int n, int radius, std::vector<float>& out, std::vector<float>& along) {
    along.resize(field.size());
    for (int pass = 0; pass < 2; ++pass) {
        if (pass) out.resize(field.size());   // after the first pass has read `field`, which `out` may be
        const std::vector<float>& from = pass ? along : field;
        std::vector<float>& to = pass ? out : along;
        for (int z = 0; z < n; ++z)
            for (int x = 0; x < n; ++x) {
                float sum = 0;
                for (int d = -radius; d <= radius; ++d)
                    sum += pass ? from[size_t(std::clamp(z + d, 0, n - 1)) * size_t(n) + size_t(x)] : from[size_t(z) * size_t(n) + size_t(std::clamp(x + d, 0, n - 1))];
                to[size_t(z) * size_t(n) + size_t(x)] = sum / float(2 * radius + 1);
            }
    }
}

bool iceReaches(const Domain& domain, const SheetSource& source) {
    const int n = domain.size / Fold, coldest = source.stages() - 1;
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const Vec2 at = nodeCentre(domain, x, z);
            if (source.thickness(coldest, at) > 0) return true;
            const size_t i = domain.index(x * Fold + Fold / 2, z * Fold + Fold / 2);
            const double above = groundOf(domain, i) - domain.seaLevel;
            if (above > 0 && iceBalance(domain.warmth[i], source.cooling[size_t(coldest)], source.halfRange(at), domain.rain[i], above, above) > 0) return true;
        }
    }
    return false;
}

void solveStage(const Domain& domain, SheetWork& work, int stage, double towards, int recorded, Workers* workers) {
    const SheetSource& source = work.source;
    const int n = domain.size / Fold, stages = source.stages();
    if (work.coarse.size() != size_t(stages)) {
        // What the source says of a place does not change while the domain is replayed.
        work.coarse.resize(size_t(stages)), work.beyond.resize(size_t(stages)), work.sunk.resize(size_t(stages));
        for (int k = 0; k < stages; ++k) {
            work.beyond[size_t(k)].resize(size_t(n) * size_t(n)), work.sunk[size_t(k)].resize(size_t(n) * size_t(n));
            for (int z = 0; z < n; ++z)
                for (int x = 0; x < n; ++x) {
                    const Vec2 at = nodeCentre(domain, x, z);
                    work.beyond[size_t(k)][size_t(z) * size_t(n) + size_t(x)] = source.thickness(k, at);
                    work.sunk[size_t(k)][size_t(z) * size_t(n) + size_t(x)] = source.sink(k, at);
                }
        }
    }
    IceGrid& grid = work.coarse[size_t(stage)];
    const bool first = grid.width == 0;
    if (first) {
        shapeIce(grid, n, n, Fold * domain.cell, false);
        for (int z = 0; z < n; ++z)
            for (int x = 0; x < n; ++x) {
                const Vec2 at = nodeCentre(domain, x, z);
                grid.heat[grid.index(x, z)] = source.heat(at), grid.halfRange[grid.index(x, z)] = source.halfRange(at);
            }
    }
    const size_t colder = size_t(std::min(stage + 1, stages - 1));
    const float blend = float(towards);
    grid.seaLevel = float(domain.seaLevel);
    grid.cooling = float(mix(source.cooling[size_t(stage)], source.cooling[colder], towards));
    std::vector<float> beyond(grid.cells());
    for (size_t i = 0; i < grid.cells(); ++i) {
        beyond[i] = work.beyond[size_t(stage)][i] + blend * (work.beyond[colder][i] - work.beyond[size_t(stage)][i]);
        grid.sink[i] = work.sunk[size_t(stage)][i] + blend * (work.sunk[colder][i] - work.sunk[size_t(stage)][i]);
    }

    std::vector<float> given(grid.cells(), -1.0f);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const size_t to = grid.index(x, z);
            float bed = 0, warmth = 0, rain = 0, ice = 0;
            bool owned = recorded >= 0;
            for (int k = 0; k < Fold * Fold; ++k) {
                const int cx = x * Fold + k % Fold, cz = z * Fold + k / Fold;
                const size_t from = domain.index(cx, cz);
                bed += groundOf(domain, from), warmth += domain.warmth[from], rain += domain.rain[from];
                const float theirs = owned ? recordedIce(domain, cx, cz, recorded) : -1.0f;
                owned = owned && theirs >= 0;
                ice += theirs;
            }
            const float share = 1.0f / float(Fold * Fold);
            grid.bed[to] = bed * share, grid.warmth[to] = warmth * share, grid.rain[to] = rain * share;
            if (owned) given[to] = ice * share;
        }
    }
    // The source's sheet lies on ground far smoother than a domain's; its thickness is taken over
    // the domain's ground averaged as widely, which puts its surface, not its thickness, on every cell.
    std::vector<float> level, along;
    boxMean(grid.bed, n, SmoothRadius, level, along);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const size_t i = grid.index(x, z);
            const float guess = beyond[i] > 0 ? std::max(0.0f, level[i] + beyond[i] - grid.bed[i]) : 0.0f;
            const bool rim = x == 0 || z == 0 || x == n - 1 || z == n - 1;
            grid.held[i] = given[i] >= 0 || (rim && beyond[i] > 0);
            if (given[i] >= 0) grid.thickness[i] = given[i];
            else if (grid.held[i] || first) grid.thickness[i] = guess;
        }
    }
    relaxIce(grid, workers, first ? FirstRounds : NextRounds, Steady, false);
}

SheetField readStage(const Domain& domain, const SheetWork& work, int stage, Workers* workers) {
    const IceGrid& grid = work.coarse[size_t(stage)];
    const int n = grid.width, size = domain.size;
    const size_t cells = domain.cells();
    SheetField field;
    for (auto* each : {&field.thickness, &field.top, &field.fallX, &field.fallZ, &field.slide, &field.melt}) each->assign(cells, 0.0f);

    // On the coarse cells: the surface with the crust's sink left out, its slope, and the ice a
    // thawed bed would pass by sliding (m² a year through a metre of width).
    std::vector<float> top(grid.cells()), fallX(grid.cells()), fallZ(grid.cells()), passing(grid.cells()), cubes(grid.cells(), 0.0f);
    for (size_t i = 0; i < grid.cells(); ++i) top[i] = grid.bed[i] + grid.thickness[i];
    for (int z = 0; z < n; ++z)
        for (int x = 0; x < n; ++x) {
            const size_t i = grid.index(x, z);
            const int west = std::max(0, x - 1), east = std::min(n - 1, x + 1), north = std::max(0, z - 1), south = std::min(n - 1, z + 1);
            fallX[i] = (top[grid.index(east, z)] - top[grid.index(west, z)]) / float((east - west) * grid.cell);
            fallZ[i] = (top[grid.index(x, south)] - top[grid.index(x, north)]) / float((south - north) * grid.cell);
            const float stress = std::min(YieldStress, float(IceWeight) * grid.thickness[i] * std::sqrt(fallX[i] * fallX[i] + fallZ[i] * fallZ[i]));
            passing[i] = float(SlideRate) * stress * stress * grid.thickness[i];
        }

    const float sea = float(domain.seaLevel);
    struct Blend {
        size_t corner[4];
        float weight[4];
        float of(const std::vector<float>& from) const {
            return weight[0] * from[corner[0]] + weight[1] * from[corner[1]] + weight[2] * from[corner[2]] + weight[3] * from[corner[3]];
        }
    };
    const auto blendAt = [&](int x, int z) {
        const float gx = (x + 0.5f) / Fold - 0.5f, gz = (z + 0.5f) / Fold - 0.5f;
        const int ix = std::clamp(int(std::floor(gx)), 0, n - 2), iz = std::clamp(int(std::floor(gz)), 0, n - 2);
        const float tx = std::clamp(gx - float(ix), 0.0f, 1.0f), tz = std::clamp(gz - float(iz), 0.0f, 1.0f);
        return Blend{{grid.index(ix, iz), grid.index(ix + 1, iz), grid.index(ix, iz + 1), grid.index(ix + 1, iz + 1)}, {(1 - tx) * (1 - tz), tx * (1 - tz), (1 - tx) * tz, tx * tz}};
    };
    const auto spread = [&](const std::function<void(int, int)>& rows) {
        if (workers) workers->parallelFor(size, rows, LeastRows);
        else rows(0, size);
    };
    spread([&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < size; ++x) {
                const size_t i = domain.index(x, z);
                const float ground = groundOf(domain, i);
                field.top[i] = std::max(ground, sea);
                const Blend blend = blendAt(x, z);
                // Ice stands on a cell only within reach of coarse cells that hold some: a valley
                // far outside the sheet is below its neighbours' ground, not under their ice.
                if (blend.of(grid.thickness) < SmallIce) continue;
                const float surface = blend.of(top), thickness = surface - ground;
                if (thickness < LeastSheet || (ground < sea && float(IceToWater) * thickness < sea - (ground - blend.of(grid.sink)))) continue;
                field.thickness[i] = thickness;
                field.top[i] = surface;
                field.fallX[i] = blend.of(fallX), field.fallZ[i] = blend.of(fallZ);
            }
        }
    });
    // Sliding grows with the square of the thickness, so of the ice a coarse cell passes, a valley
    // under it takes more than its width: its share is its thickness cubed against the mean of
    // the cubes. However deep a trough is cut, it carries no more ice than arrives, and slows as
    // it deepens.
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const float thickness = field.thickness[domain.index(x, z)];
            cubes[grid.index(x / Fold, z / Fold)] += thickness * thickness * thickness / float(Fold * Fold);
        }
    spread([&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < size; ++x) {
                const size_t i = domain.index(x, z);
                const float thickness = field.thickness[i];
                if (thickness <= 0) continue;
                const Blend blend = blendAt(x, z);
                const float slope = std::min(SteepestSurface, std::sqrt(field.fallX[i] * field.fallX[i] + field.fallZ[i] * field.fallZ[i]));
                const float balance = blend.of(grid.balance);
                const double thaw = bedThaw(iceSurfaceWarmth(domain.warmth[i], grid.cooling, field.top[i] - blend.of(grid.sink) - sea), thickness, balance, blend.of(grid.heat), slope);
                const float cube = blend.of(cubes);
                if (cube > 0) field.slide[i] = std::min(FastestSlide, float(thaw) * blend.of(passing) * thickness * thickness / cube);
                // Summer melt drains to the bed through the ice; a thawed bed adds its own.
                field.melt[i] = float(IceToWater) * std::max(0.0f, -balance) + (thaw > 0.5 ? BasalMelt : 0.0f);
            }
        }
    });
    return field;
}

} // namespace derivean::sim
