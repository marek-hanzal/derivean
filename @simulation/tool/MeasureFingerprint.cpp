#include "Measure.hpp"

#include "landscape/fn/Build.hpp"
#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Dependencies.hpp"
#include "measure/fn/Digest.hpp"
#include "measure/fn/Fingerprint.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/resource.h>
#endif
#if defined(__APPLE__)
#include <libproc.h>
#include <unistd.h>
#endif

using namespace derivean::sim;

namespace {

// The first-pass Site tile under the highest first-pass Local tile, as test/LandscapeFixture.cpp
// testFocus picks it: a mountain with rivers.
Vec2 highestFocus(const Planet& planet) {
    const int perRegion = int(tileSpan(Level::Region) / tileSpan(Level::Local));
    Vec2 best{};
    double highest = -1e30;
    for (int rz = 0; rz < int(PlanetSpan / tileSpan(Level::Region)); rz += 2)
        for (int rx = 0; rx < int(PlanetSpan / tileSpan(Level::Region)); rx += 2)
            for (int lz = 4; lz <= perRegion - 6; lz += 2)
                for (int lx = 4; lx <= perRegion - 6; lx += 2) {
                    const Vec2 centre{(rx * perRegion + lx + 0.5) * tileSpan(Level::Local), (rz * perRegion + lz + 0.5) * tileSpan(Level::Local)};
                    const double height = samplePlanet(planet.elevation, planetPoint(planet.seed, centre));
                    if (height > highest) highest = height, best = centre;
                }
    const TileKey site = tileAt(Level::Site, best);
    return Vec2{((site.x & ~1) + 0.5) * tileSpan(Level::Site), ((site.z & ~1) + 0.5) * tileSpan(Level::Site)};
}

// The mouth of the largest river that reaches the sea on a tile: the wettest cell whose water
// stands within a metre of sea level.
Vec2 riverMouth(const Tile& tile) {
    size_t best = 0;
    float most = -1;
    for (size_t i = 0; i < tile.water.size(); ++i) {
        const float ground = tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i];
        if (tile.water[i] <= ground || std::abs(tile.water[i] - float(tile.seaLevel)) > 1.0f) continue;
        if (tile.discharge[i] > most) most = tile.discharge[i], best = i;
    }
    const double cell = specOf(tile.key.level).cell;
    return {(tile.key.x * TileCells + double(best % TileCells) + 0.5) * cell, (tile.key.z * TileCells + double(best / TileCells) + 0.5) * cell};
}

// What the process has used so far: processor seconds, and the instructions and cycles it retired
// where the system counts them (instructions on macOS, cycles on macOS and Windows; zero
// elsewhere). Unlike times, the counts hardly move with the machine's load.
struct Usage {
    double user{}, sys{}, instructions{}, cycles{};
};
Usage usageNow() {
    Usage usage;
#if defined(_WIN32)
    FILETIME created, exited, kernel, user;
    const auto ticks = [](FILETIME time) { return double((uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime); };
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) usage.user = 1e-7 * ticks(user), usage.sys = 1e-7 * ticks(kernel);
    ULONG64 cycles = 0;
    if (QueryProcessCycleTime(GetCurrentProcess(), &cycles)) usage.cycles = double(cycles);
#else
    rusage self{};
    getrusage(RUSAGE_SELF, &self);
    const auto seconds = [](const timeval& t) { return double(t.tv_sec) + 1e-6 * double(t.tv_usec); };
    usage.user = seconds(self.ru_utime), usage.sys = seconds(self.ru_stime);
#endif
#if defined(__APPLE__)
    rusage_info_v4 info{};
    if (proc_pid_rusage(getpid(), RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t*>(&info)) == 0) usage.instructions = double(info.ri_instructions), usage.cycles = double(info.ri_cycles);
#endif
    return usage;
}

} // namespace

// terra measure fingerprint [--age 1.2e9|4.6e9] [--threads n] [--time 0|1] [--repeat n] [--blocks 0|1]
// Builds a fixed set of tiles of seed 42 (river, coast and glaciated country at every level) and
// prints one hash over every field of each of them and of every tile they were built from. Two
// builds that print the same lines made the same world to the bit. With --time 1 each target is
// then replayed on the calling thread alone, --repeat times, and the least wall and user time are
// printed: on a loaded machine one replay's time says more about the scheduler than the code.
// Instructions and cycles (macOS) are the least over the repeats too.
int measureFingerprint(const Arguments& arguments) {
    const double age = arguments.number("age", 4.6e9);
    Lap lap;
    World world({arguments.seed(), age, unsigned(arguments.number("threads", 0))});
    std::printf("fingerprint  seed %llu  age %.3g yr  planet %.1f s\n", static_cast<unsigned long long>(arguments.seed()), age, lap());

    std::vector<std::pair<std::string, TileKey>> targets;
    const auto add = [&](const char* what, Level level, Vec2 at) { targets.emplace_back(what, tileAt(level, at)); };
    if (age < 3e9) {
        const Vec2 focus = highestFocus(world.planet);
        add("range", Level::Local, focus), add("range", Level::Site, focus), add("range", Level::Ground, focus);
        TileKey second = tileAt(Level::Ground, focus);
        second.x |= 1, second.z &= ~1;   // a second-pass ground tile, as testGroundTile takes
        targets.emplace_back("range pass 2", second);
        const Vec2 shore = findShore(world.planet, {0, 0});
        add("shore", Level::Local, shore), add("shore", Level::Site, shore), add("shore", Level::Ground, shore);
    } else {
        // testReliefGlaciatedSite's trough wall and testReliefLedger's coast tile.
        const Vec2 trough{5605504, 10105024};
        add("glaciated", Level::Site, trough), add("glaciated", Level::Ground, trough);
        targets.emplace_back("coast", TileKey{Level::Local, 705, 261});
        const Vec2 mouth = riverMouth(*world.atlas.require({Level::Local, 705, 261}));
        add("river mouth", Level::Site, mouth), add("river mouth", Level::Ground, mouth);
    }

    std::map<TileKey, uint64_t, bool (*)(const TileKey&, const TileKey&)> prints([](const TileKey& a, const TileKey& b) {
        return std::tuple(int(a.level), a.x, a.z) < std::tuple(int(b.level), b.x, b.z);
    });
    std::vector<TileKey> pending;
    for (const auto& [what, key] : targets) {
        world.atlas.require(key);
        std::printf("target %-13s level %d  %lld %lld\n", what.c_str(), int(key.level), static_cast<long long>(key.x), static_cast<long long>(key.z));
        pending.push_back(key);
    }
    std::printf("built %zu tiles in %.1f s\n", world.atlas.finished(), lap());
    // Every tile the targets were built from, down to the regions; one the atlas does not hold is
    // counted, not hashed.
    size_t missing = 0;
    while (!pending.empty()) {
        const TileKey key = pending.back();
        pending.pop_back();
        if (prints.count(key)) continue;
        const auto tile = world.atlas.find(key);
        if (!tile) {
            ++missing;
            continue;
        }
        prints[key] = fingerprintOf(*tile);
        for (const TileKey& other : dependencies(key)) pending.push_back(other);
    }
    uint64_t all = 0;
    int perLevel[LevelCount]{};
    for (const auto& [key, print] : prints) {
        // Beside the print, the ledger and the cover, to say by how much a tile moved.
        const auto held = world.atlas.find(key);
        const Tile& tile = *held;
        double cover[SedimentClasses]{};
        for (int c = 0; c < SedimentClasses; ++c)
            for (const float m : tile.now.sediment[c]) cover[c] += m;
        std::printf("tile %d %lld %lld %016llx  out %.9g  in %.9g  laid %.9g  cut %.9g  cover %.9g %.9g %.9g\n", int(key.level), static_cast<long long>(key.x),
                    static_cast<long long>(key.z), static_cast<unsigned long long>(print), tile.ledger.sedimentOut, tile.ledger.sedimentIn, tile.ledger.deposited,
                    tile.ledger.eroded, cover[0], cover[1], cover[2]);
        all = digestOf(&print, sizeof(print), all);
        ++perLevel[size_t(key.level)];
    }
    std::printf("prints %zu (region %d local %d site %d ground %d)  all %016llx\n", prints.size(), perLevel[1], perLevel[2], perLevel[3], perLevel[4],
                static_cast<unsigned long long>(all));
    if (missing) std::printf("missing %zu dependencies the atlas does not hold\n", missing);

    // --blocks 1: a block of voxels over the middle of each ground target, cut once to build the
    // tiles it reads and then timed again; its materials hashed like a tile.
    if (arguments.number("blocks", 0) != 0)
        for (const auto& [what, key] : targets) {
            if (key.level != Level::Ground) continue;
            const auto held = world.atlas.find(key);
            if (!held) continue;
            const Tile& tile = *held;
            const size_t middle = size_t(TileCells / 2) * TileCells + TileCells / 2;
            const double top = tile.now.rock[middle] + tile.now.sediment[0][middle] + tile.now.sediment[1][middle] + tile.now.sediment[2][middle];
            const double span = tileSpan(Level::Ground);
            const Block block{(key.x + 0.5) * span - 16, std::floor(top) - 48, (key.z + 0.5) * span - 16, 1.0, 32, 64, 32};
            std::vector<Material> materials(size_t(block.countX) * size_t(block.countY) * size_t(block.countZ));
            sampleBlock(world, block, materials.data());
            constexpr int Repeats = 20;
            lap();
            for (int repeat = 0; repeat < Repeats; ++repeat) sampleBlock(world, block, materials.data());
            const double each = lap() / Repeats;
            std::printf("block %-13s %016llx  %.2f ms each\n", what.c_str(), static_cast<unsigned long long>(digestOf(materials.data(), materials.size() * sizeof(Material))), 1000 * each);
        }

    if (arguments.number("time", 0) == 0) return 0;
    // One thread: the replay alone, as the atlas makes it, without the pool a ground tile may use.
    Setting alone = world.atlas.setting();
    alone.workers = nullptr;
    const int repeats = int(std::max(1.0, std::min(100.0, arguments.number("repeat", 1))));   // each of wall, user, sys is its own least
    for (const auto& [what, key] : targets) {
        const Dependencies needed = requireDependencies(world, key);
        double wall = 1e30, user = 1e30, sys = 1e30, instructions = 1e30, cycles = 1e30;
        bool same = true;
        for (int repeat = 0; repeat < repeats; ++repeat) {
            const Usage before = usageNow();
            lap();
            const auto again = replayTile(alone, key, needed.lookup());
            wall = std::min(wall, lap());
            const Usage after = usageNow();
            user = std::min(user, after.user - before.user), sys = std::min(sys, after.sys - before.sys);
            instructions = std::min(instructions, after.instructions - before.instructions), cycles = std::min(cycles, after.cycles - before.cycles);
            same = same && fingerprintOf(*again) == prints.at(key);
        }
        std::printf("time %-13s level %d  wall %.2f s  user %.2f s  sys %.2f s  instructions %.4g  cycles %.4g  %s\n", what.c_str(), int(key.level), wall, user, sys,
                    instructions, cycles, same ? "same" : "DIFFERS");
        std::fflush(stdout);
    }
    return 0;
}
