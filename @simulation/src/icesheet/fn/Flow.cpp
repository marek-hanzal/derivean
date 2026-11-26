#include "icesheet/fn/Flow.hpp"

#include "core/service/Workers.hpp"
#include "core/type/Vec.hpp"
#include "icesheet/fn/Balance.hpp"

#include <functional>

namespace derivean::sim {
namespace {

constexpr int LeastRows = 16;             // rows below which a range is not worth another thread
constexpr float LongestStep = 2000.0f;    // yr a cell may take in one round
constexpr float LargestChange = 60.0f;    // m a cell may gain or lose in one round
// Explicit diffusion holds below cell² / (4 · D · step). The flux grows with the third power of the
// slope, which makes the surface three times stiffer than D alone says.
constexpr float Stiffness = 15.0f;
constexpr float SteepestSurface = 0.3f;   // slope beyond which the thin-slab flow law is not asked
constexpr int Reweigh = 4;                // rounds between readings of the climate
constexpr float Reread = 4.0f;            // m a surface must move before its climate is read again
constexpr float SinkPace = 0.25f;         // share of the way to its resting depth the crust goes per round

void forRows(Workers* workers, int count, const std::function<void(int, int)>& body) {
    if (workers) workers->parallelFor(count, body, LeastRows);
    else body(0, count);
}

int beside(const IceGrid& grid, int index, int count) { return grid.wraps ? wrapIndex(index, count) : std::clamp(index, 0, count - 1); }

float waterOver(const IceGrid& grid, size_t i) { return grid.seaLevel - (grid.bed[i] - grid.sink[i]); }

float fallOf(const IceGrid& grid, int x, int z, float& fallX, float& fallZ) {
    fallX = (grid.surface(grid.index(beside(grid, x + 1, grid.width), z)) - grid.surface(grid.index(beside(grid, x - 1, grid.width), z))) / float(2 * grid.cell);
    fallZ = (grid.surface(grid.index(x, beside(grid, z + 1, grid.height))) - grid.surface(grid.index(x, beside(grid, z - 1, grid.height)))) / float(2 * grid.cell);
    return std::min(SteepestSurface, std::sqrt(fallX * fallX + fallZ * fallZ));
}

// `seen` holds the surface each cell's climate was last read for; a cell is read again only once
// its surface has moved. Null reads every cell.
void weigh(IceGrid& grid, Workers* workers, std::vector<float>* seen) {
    forRows(workers, grid.height, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < grid.width; ++x) {
                const size_t i = grid.index(x, z);
                const float top = grid.surface(i);
                if (seen) {
                    if (std::abs(top - (*seen)[i]) < Reread) continue;
                    (*seen)[i] = top;
                }
                const double sea = grid.seaLevel, above = top - sea;
                grid.balance[i] = float(iceBalance(grid.warmth[i], grid.cooling, grid.halfRange[i], grid.rain[i], above, grid.bed[i] - sea));
                float fallX, fallZ;
                grid.thaw[i] = grid.thickness[i] <= 0 ? 0.0f
                    : float(bedThaw(iceSurfaceWarmth(grid.warmth[i], grid.cooling, above), grid.thickness[i], grid.balance[i], grid.heat[i], fallOf(grid, x, z, fallX, fallZ)));
            }
        }
    });
}

// One round for the rows [begin, end): every cell from the surface `top` of the round before.
float advanceRows(const IceGrid& grid, const std::vector<float>& top, std::vector<float>& next, int begin, int end) {
    const float inverse = float(1.0 / grid.cell), area = float(grid.cell * grid.cell);
    float most = 0;
    for (int z = begin; z < end; ++z) {
        const int rows[3] = {beside(grid, z - 1, grid.height), z, beside(grid, z + 1, grid.height)};
        for (int x = 0; x < grid.width; ++x) {
            const size_t i = grid.index(x, z);
            const float here = grid.thickness[i];
            next[i] = here;
            if (grid.held[i]) continue;
            const int columns[3] = {beside(grid, x - 1, grid.width), x, beside(grid, x + 1, grid.width)};
            // East, south, west, north.
            const size_t side[4] = {grid.index(columns[2], z), grid.index(x, rows[2]), grid.index(columns[0], z), grid.index(x, rows[0])};
            if (here <= 0 && grid.balance[i] <= 0 && grid.thickness[side[0]] <= 0 && grid.thickness[side[1]] <= 0 && grid.thickness[side[2]] <= 0 &&
                grid.thickness[side[3]] <= 0)
                continue;
            float t[3][3];
            for (int dz = 0; dz < 3; ++dz)
                for (int dx = 0; dx < 3; ++dx) t[dz][dx] = top[grid.index(columns[dx], rows[dz])];
            // Fall of the surface towards each side, and its fall along that side from the four cells that flank it.
            const float normal[4] = {t[1][2] - t[1][1], t[2][1] - t[1][1], t[1][0] - t[1][1], t[0][1] - t[1][1]};
            const float across[4] = {t[2][1] + t[2][2] - t[0][1] - t[0][2], t[1][2] + t[2][2] - t[1][0] - t[2][0], t[2][1] + t[2][0] - t[0][1] - t[0][0],
                                     t[1][2] + t[0][2] - t[1][0] - t[0][0]};
            const float water = waterOver(grid, i);
            float gain = 0, stiffest = 0;
            int open = 0;
            for (int k = 0; k < 4; ++k) {
                const size_t j = side[k];
                const float other = grid.thickness[j];
                if (water > 0 && other <= 0 && waterOver(grid, j) > 0) ++open;
                // Ice leaves only a cell that has some: bare rock above a glacier sheds none onto it.
                if ((normal[k] < 0 ? here : other) <= 0) continue;
                const float middle = 0.5f * (here + other);
                const float slope = std::min(SteepestSurface, std::sqrt(normal[k] * normal[k] + 0.0625f * across[k] * across[k]) * inverse);
                const float mobility = middle * middle * middle *
                                       (float(CreepFlow) * middle * middle * slope * slope + float(SlideFlow) * 0.5f * (grid.thaw[i] + grid.thaw[j]) * slope);
                gain += mobility * normal[k];
                stiffest = std::max(stiffest, mobility);
            }
            // Snow that falls into open water builds nothing.
            float rate = (here <= 0 && water > 0 ? std::min(grid.balance[i], 0.0f) : grid.balance[i]) + gain / area;
            float step = stiffest > 0 ? std::min(LongestStep, area / (Stiffness * stiffest)) : LongestStep;
            if (std::abs(rate) * step > LargestChange) step = LargestChange / std::abs(rate);
            // A front in water breaks off along every face that looks onto open water; a slab that
            // floats, along all four. The loss is taken at the thickness the round ends with, so a
            // cell the sea keeps empty settles there instead of filling and emptying in turn.
            const float breaking = water > 0 ? float(CalvingRate) * water * inverse * float(float(IceToWater) * here < water ? 4 : open) : 0.0f;
            next[i] = std::max(0.0f, (here + step * rate) / (1.0f + step * breaking));
            most = std::max(most, std::abs(next[i] - here));
        }
    }
    return most;
}

} // namespace

void shapeIce(IceGrid& grid, int width, int height, double cell, bool wraps) {
    grid.width = width, grid.height = height, grid.cell = cell, grid.wraps = wraps;
    const size_t cells = grid.cells();
    for (auto* field : {&grid.bed, &grid.warmth, &grid.halfRange, &grid.rain, &grid.heat, &grid.thickness, &grid.sink, &grid.balance, &grid.thaw}) field->assign(cells, 0.0f);
    grid.held.assign(cells, 0);
}

void weighIce(IceGrid& grid, Workers* workers) { weigh(grid, workers, nullptr); }

float relaxIce(IceGrid& grid, Workers* workers, int rounds, float settled, bool crustSinks) {
    const size_t cells = grid.cells();
    std::vector<float> top(cells), next(cells), seen(cells, -1e30f), most(size_t(grid.height));
    float change = 0;
    for (int round = 0; round < rounds; ++round) {
        if (round % Reweigh == 0) weigh(grid, workers, &seen);
        forRows(workers, grid.height, [&](int begin, int end) {
            for (size_t i = grid.index(0, begin); i < grid.index(0, end); ++i) top[i] = grid.surface(i);
        });
        // The largest change is gathered row by row, so it is the same however the rows are cut.
        forRows(workers, grid.height, [&](int begin, int end) {
            for (int z = begin; z < end; ++z) most[size_t(z)] = advanceRows(grid, top, next, z, z + 1);
        });
        grid.thickness.swap(next);
        if (crustSinks) {
            forRows(workers, grid.height, [&](int begin, int end) {
                for (size_t i = grid.index(0, begin); i < grid.index(0, end); ++i)
                    if (!grid.held[i]) grid.sink[i] += SinkPace * (float(IceToMantle) * grid.thickness[i] - grid.sink[i]);
            });
        }
        change = 0;
        for (const float row : most) change = std::max(change, row);
        if (change < settled && round % Reweigh == Reweigh - 1) break;
    }
    weigh(grid, workers, nullptr);
    return change;
}

IceSlide slideAt(const IceGrid& grid, int x, int z) {
    const size_t i = grid.index(x, z);
    if (grid.thickness[i] <= 0 || grid.thaw[i] <= 0 || afloat(grid, i)) return {};
    float fallX, fallZ;
    const float slope = fallOf(grid, x, z, fallX, fallZ), steep = std::sqrt(fallX * fallX + fallZ * fallZ);
    if (steep <= 0) return {};
    const float stress = float(IceWeight) * grid.thickness[i] * slope;
    const float speed = grid.thaw[i] * float(SlideRate) * stress * stress;
    return {-speed * fallX / steep, -speed * fallZ / steep};
}

bool afloat(const IceGrid& grid, size_t i) {
    const float water = waterOver(grid, i);
    return water > 0 && float(IceToWater) * grid.thickness[i] < water;
}

} // namespace derivean::sim
