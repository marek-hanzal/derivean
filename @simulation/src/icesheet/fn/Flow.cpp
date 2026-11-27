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
constexpr float ThawPace = 0.2f;          // share of the way to its reading the thaw of a bed goes each time the climate is read
constexpr int Watch = 40;                 // rounds over which the volume must hold still for the sheet to count as steady

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

// What the relaxation keeps between rounds.
struct Rounds {
    std::vector<float> next, nextSink;
    std::vector<float> seen;          // surface each cell's climate was last read for
    std::vector<float> aim;           // thaw that reading gave; the bed eases towards it
    std::vector<uint8_t> icy, snowy;  // per row: holds ice; holds a cell the climate feeds
    std::vector<double> volume;       // per row, m of ice summed over its cells
};

// Reads the climate of the cells whose surface has moved since their last reading (all of them
// without `rounds`) and lets every bed ease towards the thaw its reading gave.
void weigh(IceGrid& grid, Workers* workers, Rounds* rounds) {
    forRows(workers, grid.height, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            bool snowy = false;
            for (int x = 0; x < grid.width; ++x) {
                const size_t i = grid.index(x, z);
                const float top = grid.surface(i);
                if (!rounds || std::abs(top - rounds->seen[i]) >= Reread) {
                    const double sea = grid.seaLevel, above = top - sea;
                    grid.balance[i] = float(iceBalance(grid.warmth[i], grid.cooling, grid.halfRange[i], grid.rain[i], above, grid.bed[i] - sea));
                    float fallX, fallZ;
                    const float thaw = grid.thickness[i] <= 0 ? 0.0f
                        : float(bedThaw(iceSurfaceWarmth(grid.warmth[i], grid.cooling, above), grid.thickness[i], grid.balance[i], grid.heat[i], fallOf(grid, x, z, fallX, fallZ)));
                    if (rounds) rounds->seen[i] = top, rounds->aim[i] = thaw;
                    else grid.thaw[i] = thaw;
                }
                // A bed that thaws lets its ice run out and thin until it freezes again. Taking only
                // part of each reading lets such a cell come to rest between the two.
                if (rounds) grid.thaw[i] += ThawPace * (rounds->aim[i] - grid.thaw[i]);
                snowy = snowy || grid.balance[i] > 0;
            }
            if (rounds) rounds->snowy[size_t(z)] = snowy;
        }
    });
}

// One round for row z: every cell from the state of the round before.
void advanceRow(const IceGrid& grid, Rounds& rounds, int z, bool crustSinks) {
    const float inverse = float(1.0 / grid.cell), area = float(grid.cell * grid.cell), perArea = 1.0f / area;
    const int rows[3] = {beside(grid, z - 1, grid.height), z, beside(grid, z + 1, grid.height)};
    std::vector<float>& next = rounds.next;
    const size_t first = grid.index(0, z);
    // A row with no ice in it or beside it and no snow that lasts cannot change.
    if (!rounds.snowy[size_t(z)] && !rounds.icy[size_t(rows[0])] && !rounds.icy[size_t(z)] && !rounds.icy[size_t(rows[2])]) {
        for (size_t i = first; i < first + size_t(grid.width); ++i) {
            next[i] = grid.thickness[i];
            if (crustSinks) rounds.nextSink[i] = grid.sink[i] * (1.0f - SinkPace);
        }
        rounds.volume[size_t(z)] = 0;
        return;
    }
    double volume = 0;
    for (int x = 0; x < grid.width; ++x) {
        const size_t i = first + size_t(x);
        const float here = grid.thickness[i];
        next[i] = here;
        if (crustSinks) rounds.nextSink[i] = grid.sink[i];
        if (grid.held[i]) {
            volume += here;
            continue;
        }
        const int columns[3] = {beside(grid, x - 1, grid.width), x, beside(grid, x + 1, grid.width)};
        // East, south, west, north.
        const size_t side[4] = {grid.index(columns[2], z), grid.index(x, rows[2]), grid.index(columns[0], z), grid.index(x, rows[0])};
        if (here <= 0 && grid.balance[i] <= 0 && grid.thickness[side[0]] <= 0 && grid.thickness[side[1]] <= 0 && grid.thickness[side[2]] <= 0 &&
            grid.thickness[side[3]] <= 0) {
            if (crustSinks) rounds.nextSink[i] = grid.sink[i] * (1.0f - SinkPace);
            continue;
        }
        float t[3][3];
        for (int dz = 0; dz < 3; ++dz)
            for (int dx = 0; dx < 3; ++dx) t[dz][dx] = grid.surface(grid.index(columns[dx], rows[dz]));
        // Fall of the surface towards each side, and its fall along that side from the four cells that flank it.
        const float normal[4] = {t[1][2] - t[1][1], t[2][1] - t[1][1], t[1][0] - t[1][1], t[0][1] - t[1][1]};
        const float across[4] = {t[2][1] + t[2][2] - t[0][1] - t[0][2], t[1][2] + t[2][2] - t[1][0] - t[2][0], t[2][1] + t[2][0] - t[0][1] - t[0][0],
                                 t[1][2] + t[0][2] - t[1][0] - t[0][0]};
        // Only ground that lies under the sea without any load is a sea floor: land its own ice
        // has pressed below the sea level is not reached by the sea.
        const float water = grid.bed[i] < grid.seaLevel ? waterOver(grid, i) : 0.0f;
        float gain = 0, stiffest = 0;
        int open = 0;
        for (int k = 0; k < 4; ++k) {
            const size_t j = side[k];
            const float other = grid.thickness[j];
            if (water > 0 && grid.bed[j] < grid.seaLevel && float(IceToWater) * other < waterOver(grid, j)) ++open;
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
        const float rate = (here <= 0 && water > 0 ? std::min(grid.balance[i], 0.0f) : grid.balance[i]) + gain * perArea;
        float step = stiffest > 0 ? std::min(LongestStep, area / (Stiffness * stiffest)) : LongestStep;
        if (std::abs(rate) * step > LargestChange) step = LargestChange / std::abs(rate);
        // A front in water breaks off along every face that looks onto open water; a slab that
        // floats, along all four. The loss is taken at the thickness the round ends with, so a
        // cell the sea keeps empty settles there instead of filling and emptying in turn.
        const float breaking = water > 0 ? float(CalvingRate) * water * inverse * float(float(IceToWater) * here < water ? 4 : open) : 0.0f;
        next[i] = std::max(0.0f, (here + step * rate) / (1.0f + step * breaking));
        if (crustSinks) rounds.nextSink[i] = grid.sink[i] + SinkPace * (float(IceToMantle) * next[i] - grid.sink[i]);
        volume += next[i];
    }
    rounds.volume[size_t(z)] = volume;
}

} // namespace

void shapeIce(IceGrid& grid, int width, int height, double cell, bool wraps) {
    grid.width = width, grid.height = height, grid.cell = cell, grid.wraps = wraps;
    const size_t cells = grid.cells();
    for (auto* field : {&grid.bed, &grid.warmth, &grid.halfRange, &grid.rain, &grid.heat, &grid.thickness, &grid.sink, &grid.balance, &grid.thaw}) field->assign(cells, 0.0f);
    grid.held.assign(cells, 0);
}

void weighIce(IceGrid& grid, Workers* workers) { weigh(grid, workers, nullptr); }

int relaxIce(IceGrid& grid, Workers* workers, int most, float settled, bool crustSinks) {
    const size_t cells = grid.cells(), rows = size_t(grid.height);
    Rounds rounds{std::vector<float>(cells), std::vector<float>(crustSinks ? cells : 0), std::vector<float>(cells, -1e30f), std::vector<float>(cells),
                  std::vector<uint8_t>(rows, 1), std::vector<uint8_t>(rows, 1), std::vector<double>(rows)};
    double watched = -1;
    int round = 0;
    for (; round < most; ++round) {
        if (round % Reweigh == 0) weigh(grid, workers, &rounds);
        forRows(workers, grid.height, [&](int begin, int end) {
            for (int z = begin; z < end; ++z) advanceRow(grid, rounds, z, crustSinks);
        });
        grid.thickness.swap(rounds.next);
        if (crustSinks) grid.sink.swap(rounds.nextSink);
        // Summed row by row in a fixed order, so the count of rounds is the same for any pool.
        double volume = 0;
        for (size_t z = 0; z < rows; ++z) volume += rounds.volume[z], rounds.icy[z] = rounds.volume[z] > 0;
        if (round % Watch != Watch - 1) continue;
        if (watched >= 0 && std::abs(volume - watched) <= double(settled) * std::max(volume, 1.0)) {
            ++round;
            break;
        }
        watched = volume;
    }
    return round;
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

bool afloat(const IceGrid& grid, size_t i) { return grid.bed[i] < grid.seaLevel && float(IceToWater) * grid.thickness[i] < waterOver(grid, i); }

} // namespace derivean::sim
