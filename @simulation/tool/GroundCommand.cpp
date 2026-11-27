#include "Ground.hpp"

#include "ground/type/Detail.hpp"
#include "landscape/fn/Build.hpp"
#include "simulation/fn/Query.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <map>

#if defined(_WIN32)
#include <windows.h>
#endif

using namespace derivean::sim;

namespace {

double seconds(std::chrono::steady_clock::time_point since) { return std::chrono::duration<double>(std::chrono::steady_clock::now() - since).count(); }

// Processor time the calling thread has used: what a piece of work costs however busy the machine is.
double threadSeconds() {
#if defined(_WIN32)
    FILETIME created, exited, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user)) return 0.0;
    // Kernel and user time together, as CLOCK_THREAD_CPUTIME_ID counts them; in 100 ns units but
    // advanced only at the scheduler's tick (about 15.6 ms), so only long work is timed truly.
    const auto ticks = [](FILETIME time) { return double((uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime); };
    return 1e-7 * (ticks(kernel) + ticks(user));
#else
    timespec used{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &used);
    return double(used.tv_sec) + 1e-9 * double(used.tv_nsec);
#endif
}

// Mean and scatter (standard deviation over mean) of a list of distances.
void printSpacing(const char* name, std::vector<float> gaps) {
    if (gaps.size() < 3) {
        std::printf("  %s: %zu, too few for a spacing\n", name, gaps.size());
        return;
    }
    std::sort(gaps.begin(), gaps.end());
    double sum = 0, squares = 0;
    for (float gap : gaps) sum += gap, squares += double(gap) * gap;
    const double mean = sum / gaps.size(), scatter = std::sqrt(std::max(0.0, squares / gaps.size() - mean * mean)) / mean;
    std::printf("  %s: %zu gaps  mean %.2f m  p10 %.2f  median %.2f  p90 %.2f  scatter %.2f (0 is even spacing, 1 is chance)\n", name, gaps.size(), mean,
                gaps[gaps.size() / 10], gaps[gaps.size() / 2], gaps[gaps.size() * 9 / 10], scatter);
}

// Distances between the troughs a rill or gully makes on the lines across a tile: a cell that lies
// at least `depth` under both shoulders four cells away.
void rillGaps(const Tile& tile, std::vector<float>& gaps) {
    constexpr int Shoulder = 4;
    constexpr float Depth = 0.06f;
    const auto top = [&](int x, int z) {
        const size_t i = size_t(z) * TileCells + size_t(x);
        return tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i];
    };
    for (const bool alongX : {true, false})
        for (int line = 8; line < TileCells; line += 16) {
            int last = -1;
            for (int k = Shoulder; k < TileCells - Shoulder; ++k) {
                const auto at = [&](int j) { return alongX ? top(j, line) : top(line, j); };
                if (at(k) > at(k - Shoulder) - Depth || at(k) > at(k + Shoulder) - Depth || at(k) > at(k - 1) || at(k) >= at(k + 1)) continue;
                if (last >= 0) gaps.push_back(0.25f * float(k - last));
                last = k;
            }
        }
}

} // namespace

// terra ground --x --z [--span m]: what the ground level made of a place — stones by size, rock
// and origin, what covers the surface, how its rills and steps are spaced, where the coarse layer
// went, and what it cost to make and to read.
int groundReport(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    reportGround(world, {arguments.number("x", 0), arguments.number("z", 0)}, arguments.number("span", 100));
    return 0;
}

void reportGround(World& world, Vec2 centre, double span) {
    const double half = 0.5 * span;
    surveyAt(world, centre);
    requestAround(world, centre, half + 140, Level::Site);
    while (!readyAround(world, centre, half + 140, Level::Site)) world.workers.helpUntil([&] { return readyAround(world, centre, half + 140, Level::Site); });

    auto start = std::chrono::steady_clock::now();
    requestAround(world, centre, half, Level::Ground);
    const TileKey low = tileAt(Level::Ground, {centre.x - half, centre.z - half}), high = tileAt(Level::Ground, {centre.x + half, centre.z + half});
    std::vector<std::shared_ptr<const Tile>> tiles;
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x) tiles.push_back(world.atlas.require({Level::Ground, x, z}));
    const double made = seconds(start);
    const double hectares = double(tiles.size()) * tileSpan(Level::Ground) * tileSpan(Level::Ground) / 1e4;
    std::printf("ground at X %.0f Z %.0f: %zu tiles (%.2f ha) under a square of %.0f m\n", centre.x, centre.z, tiles.size(), hectares, 2 * half);

    // One tile again on this thread alone, against the neighbours it had.
    const TileKey again = tileAt(Level::Ground, centre);
    const std::vector<TileKey> needed = dependencies(again);
    std::vector<std::shared_ptr<const Tile>> held;
    for (const TileKey& other : needed) held.push_back(world.atlas.require(other));
    const TileLookup lookup = [&](TileKey wanted) -> const Tile* {
        for (size_t i = 0; i < needed.size(); ++i)
            if (needed[i] == wanted) return held[i].get();
        return nullptr;
    };
    Setting alone = world.atlas.setting();
    alone.workers = nullptr;
    start = std::chrono::steady_clock::now();
    const double cpuBefore = threadSeconds();
    replayTile(alone, again, lookup);
    const double single = seconds(start), cpu = threadSeconds() - cpuBefore;
    start = std::chrono::steady_clock::now();
    replayTile(world.atlas.setting(), again, lookup);
    const double spread = seconds(start);

    // Reading: voxel chunks of 4 m at 0.125 m, at the height of the ground.
    std::vector<Material> voxels(32 * 32 * 32);
    int chunks = 0;
    start = std::chrono::steady_clock::now();
    for (double z = centre.z - 16; z < centre.z + 16; z += 4)
        for (double x = centre.x - 16; x < centre.x + 16; x += 4, ++chunks)
            sampleBlock(world, {x, std::floor(surfaceAt(world, {x + 2, z + 2}, Level::Ground).ground / 4) * 4, z, 0.125, 32, 32, 32}, voxels.data());
    const double read = seconds(start);
    std::printf("  made in %.1f s on %u threads (site tiles already there); one tile alone on one thread %.2f s (%.2f s of that thread's own time), spread over the pool %.2f s; a 4 m chunk of voxels %.2f ms\n",
                made, world.workers.threads(), single, cpu, spread, 1000 * read / chunks);

    ClastLedger ledger;
    std::map<std::string, int> byRock;
    int bySize[4] = {}, stones = 0;
    constexpr const char* Origins[ClastOrigins] = {"fallen", "lag", "stream bed", "till", "frost"};
    enum Cover { Water, Spring, Stone, Rock, Gravel, Scree, Till, Sand, Soil, Silt, Clay, Covers };
    constexpr const char* CoverNames[Covers] = {"water", "spring deposit", "stone", "bare rock", "gravel", "scree", "till", "sand", "soil", "bare silt", "altered clay"};
    double cover[Covers] = {}, cells = 0;
    std::vector<float> rills, steps;
    for (const auto& tile : tiles) {
        const GroundDetail& detail = *tile->detail;
        for (int o = 0; o < ClastOrigins; ++o) ledger.byOrigin[o] += detail.ledger.byOrigin[o];
        ledger.coarseBefore += detail.ledger.coarseBefore, ledger.dug += detail.ledger.dug, ledger.matrix += detail.ledger.matrix;
        ledger.stones += detail.ledger.stones, ledger.offered += detail.ledger.offered;
        std::vector<uint8_t> under(size_t(TileCells) * TileCells, 0);
        for (const Clast& stone : detail.clasts) {
            ++stones;
            ++bySize[stone.b < 0.25f ? 0 : stone.b < 0.5f ? 1 : stone.b < 1.0f ? 2 : 3];
            ++byRock[std::string(definitionOf(stone.rock).name)];
            const int reach = int(stone.b / 0.25f);
            for (int z = std::max(0, int(stone.z / 0.25f) - reach); z <= std::min(TileCells - 1, int(stone.z / 0.25f) + reach); ++z)
                for (int x = std::max(0, int(stone.x / 0.25f) - reach); x <= std::min(TileCells - 1, int(stone.x / 0.25f) + reach); ++x) under[size_t(z) * TileCells + size_t(x)] = 1;
        }
        for (size_t i = 0; i < under.size(); ++i, ++cells) {
            const float coarse = tile->now.sediment[Coarse][i], sand = tile->now.sediment[Sand][i], fines = tile->now.sediment[Fines][i];
            const float ground = tile->now.rock[i] + coarse + sand + fines;
            const Cover kind = tile->water[i] > ground + 0.01f ? Water : detail.crust[i] > 0.01f ? Spring : under[i] ? Stone : coarse + sand + fines < 0.03f ? Rock
                               : fines > 0.02f ? (Material(detail.fineKind[i]) == Material::Clay ? Clay : detail.bare[i] ? Silt : Soil) : sand > 0.02f ? Sand
                               : Material(detail.coarseKind[i]) == Material::Till ? Till : Material(detail.coarseKind[i]) == Material::Scree ? Scree : Gravel;
            cover[kind] += 1;
        }
        rillGaps(*tile, rills);
        steps.insert(steps.end(), detail.stepGaps.begin(), detail.stepGaps.end());
    }
    std::printf("  stones: %d (%.0f per hectare); by half-width under 0.25 m %.0f, to 0.5 m %.0f, to 1 m %.0f, larger %.0f per hectare\n", stones, stones / hectares,
                bySize[0] / hectares, bySize[1] / hectares, bySize[2] / hectares, bySize[3] / hectares);
    std::printf("  by origin (per hectare):");
    for (int o = 0; o < ClastOrigins; ++o) std::printf("  %s %.0f", Origins[o], ledger.byOrigin[o] / hectares);
    std::printf("\n  by rock (per hectare):");
    for (const auto& [name, count] : byRock) std::printf("  %s %.0f", name.c_str(), count / hectares);
    std::printf("\n  surface:");
    for (int kind = 0; kind < Covers; ++kind)
        if (cover[kind] > 0) std::printf("  %s %.1f %%", CoverNames[kind], 100 * cover[kind] / cells);
    std::printf("\n");
    printSpacing("rills and gullies across the tiles", rills);
    printSpacing("steps down the stream beds", steps);
    const double gap = ledger.coarseBefore + ledger.dug - ledger.matrix - ledger.stones;
    std::printf("  coarse layer %.1f m3 + dug from soft rock under pools %.1f = matrix %.1f + stones %.1f, residual %.4f m3 (%.4f %%); stones offered %.1f m3, laid %.0f %%\n",
                ledger.coarseBefore, ledger.dug, ledger.matrix, ledger.stones, gap, 100 * gap / std::max(1e-9, ledger.coarseBefore), ledger.offered,
                100 * ledger.stones / std::max(1e-9, ledger.offered));
}

// terra ground: `--find 1` lists places of each kind of ground, `--map dir` draws the tiles at --x --z;
// otherwise the report of the ground there.
int groundCommand(const Arguments& arguments) {
    if (arguments.number("find", 0) > 0) return groundFind(arguments);
    return arguments.values.count("map") ? groundMap(arguments) : groundReport(arguments);
}
