// One command, cohesive: the print of every tile field, the targets, and their timing (about 260 lines).
#include "Measure.hpp"

#include "coast/type/Shore.hpp"
#include "ground/type/Detail.hpp"
#include "icesheet/type/Sheet.hpp"
#include "landscape/fn/Build.hpp"
#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Dependencies.hpp"
#include "measure/fn/Digest.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <sys/resource.h>
#ifdef __APPLE__
#include <libproc.h>
#include <unistd.h>
#endif

using namespace derivean::sim;

namespace {

// Feeds every field a finished tile keeps into one FNV state: a change of any bit of any of them
// changes the print. Structs with padding go in field by field.
struct Print {
    uint64_t state{0x9e3779b97f4a7c15ULL};
    template<typename T> void value(const T& v) { state = digestOf(&v, sizeof(T), state); }
    template<typename T> void array(const std::vector<T>& values) {
        value(values.size());
        if (!values.empty()) state = digestOf(values.data(), values.size() * sizeof(T), state);
    }
    void ground(const Ground& g) {
        array(g.rock), array(g.datum);
        for (const auto& layer : g.sediment) array(layer);
    }
    void labels(const WindLabels& l) { array(l.blown), array(l.loess), array(l.lag), array(l.salt); }
    void ledger(const TileLedger& l) { value(l.uplifted), value(l.eroded), value(l.deposited), value(l.weathered), value(l.dissolved), value(l.failed), value(l.sedimentIn), value(l.sedimentOut); }
    void coast(const CoastLedger& l) { value(l.cut), value(l.dissolved), value(l.entered), value(l.left); }
};

uint64_t printOf(const Tile& tile) {
    Print p;
    p.value(tile.key.x), p.value(tile.key.z), p.value(tile.time), p.value(tile.seaLevel);
    p.ground(tile.now), p.ground(tile.nowWide);
    for (const auto* field : {&tile.water, &tile.discharge, &tile.sunk, &tile.seeped, &tile.load, &tile.uplift, &tile.rebound, &tile.ice, &tile.iceAge, &tile.iceSpeed,
                              &tile.erosionRate, &tile.depositionRate, &tile.eroded, &tile.deposited, &tile.uplifted, &tile.rain, &tile.warmth, &tile.shed})
        p.array(*field);
    p.array(tile.flowTo), p.array(tile.rock);
    for (const IceSource& source : tile.upIce) p.value(source.source.x), p.value(source.source.z), p.value(source.rock);
    if (const GroundDetail* d = tile.detail.get()) {
        p.value(d->clasts.size());
        for (const Clast& c : d->clasts) {
            const float f[12] = {c.x, c.z, c.y, c.a, c.b, c.c, c.yaw, c.pitch, c.roll, c.roundness, c.travel, 0};
            p.value(f), p.value(c.source.x), p.value(c.source.z), p.value(c.rock), p.value(c.origin);
        }
        p.array(d->binStart), p.array(d->crust), p.array(d->crustKind), p.array(d->coarseKind), p.array(d->fineKind), p.array(d->bare), p.array(d->stepGaps);
        p.value(d->ledger.coarseBefore), p.value(d->ledger.dug), p.value(d->ledger.matrix), p.value(d->ledger.stones), p.value(d->ledger.offered), p.value(d->ledger.byOrigin);
    } else p.value(-1);
    if (const ShoreRecord* s = tile.shore.get()) p.array(s->height), p.array(s->notch), p.array(s->cut), p.array(s->laid), p.coast(s->ledger);
    else p.value(-2);
    if (const VolcanicTile* v = tile.volcanic.get()) {
        p.array(v->pile), p.array(v->laid), p.array(v->lava), p.array(v->flows), p.array(v->fall), p.array(v->wet), p.array(v->lavaUnit), p.array(v->vent);
        p.array(v->wide), p.array(v->wideLava), p.array(v->wideFlows), p.array(v->wideFall), p.array(v->wideWet), p.array(v->wideUnit);
        for (const Vent& vent : v->vents)
            p.value(vent.id), p.value(vent.centre.x), p.value(vent.centre.z), p.value(vent.vigor), p.value(vent.begin), p.value(vent.until), p.value(vent.supply),
                p.value(vent.erupted), p.value(vent.held), p.value(vent.chamber), p.value(vent.count), p.value(vent.recent.size()), p.value(vent.episodes.size());
        for (const Caldera& c : v->calderas) p.value(c.vent), p.value(c.site.x), p.value(c.site.z), p.value(c.volume), p.value(c.time), p.value(c.cut), p.value(c.kept), p.value(c.wall);
        const VolcanicLedger& l = v->ledger;
        p.value(l.erupted), p.value(l.lava), p.value(l.cone), p.value(l.ash), p.value(l.left), p.value(l.drifted), p.value(l.sealed), p.value(l.foundered), p.value(l.built);
    } else p.value(-3);
    if (const SheetRecord* s = tile.sheet.get()) {
        p.value(s->reached), p.array(s->slideX), p.array(s->slideZ), p.array(s->worn), p.array(s->till), p.array(s->moraine), p.array(s->outwash), p.array(s->esker);
        p.array(s->meltwater), p.array(s->meltTo), p.array(s->sheetThen);
        for (int c = 0; c < SedimentClasses; ++c) p.array(s->passing[c]), p.array(s->laidBack[c]);
        for (const auto* crossings : {&s->eskerArrived, &s->eskerSent})
            for (const EskerCrossing& e : *crossings) p.value(e.x), p.value(e.z), p.value(e.tx), p.value(e.tz), p.value(e.volume);
    } else p.value(-4);
    const WindRecord& w = tile.aeolian;
    p.labels(w.labels), p.labels(w.wide), p.array(w.ebb), p.value(w.span), p.value(w.rounds);
    const WindLedger& wl = w.ledger;
    for (const double v : {wl.sandIn, wl.sandOut, wl.sandLifted, wl.sandLaid, wl.sandDrowned, wl.dustRaised, wl.dustSettled, wl.dustDrowned, wl.dustIn, wl.dustOut, wl.salt}) p.value(v);
    p.ledger(tile.ledger);
    return p.state;
}

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

double cpuSeconds(const timeval& t) { return double(t.tv_sec) + 1e-6 * double(t.tv_usec); }

// Instructions and cycles the process has retired, where the system counts them (macOS): unlike
// times they hardly move with the machine's load. Zero elsewhere.
struct Counters {
    double instructions{}, cycles{};
};
Counters countersNow() {
#ifdef __APPLE__
    rusage_info_v4 info{};
    if (proc_pid_rusage(getpid(), RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t*>(&info)) == 0) return {double(info.ri_instructions), double(info.ri_cycles)};
#endif
    return {};
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
    // Every tile the targets were built from, down to the regions.
    while (!pending.empty()) {
        const TileKey key = pending.back();
        pending.pop_back();
        if (prints.count(key)) continue;
        const auto tile = world.atlas.find(key);
        if (!tile) continue;
        prints[key] = printOf(*tile);
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

    // --blocks 1: a block of voxels over the middle of each ground target, cut once to build the
    // tiles it reads and then timed again; its materials hashed like a tile.
    if (arguments.number("blocks", 0) != 0)
        for (const auto& [what, key] : targets) {
            if (key.level != Level::Ground) continue;
            const auto held = world.atlas.find(key);
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
            rusage before{}, after{};
            getrusage(RUSAGE_SELF, &before);
            const Counters start = countersNow();
            lap();
            const auto again = replayTile(alone, key, needed.lookup());
            wall = std::min(wall, lap());
            const Counters end = countersNow();
            getrusage(RUSAGE_SELF, &after);
            instructions = std::min(instructions, end.instructions - start.instructions), cycles = std::min(cycles, end.cycles - start.cycles);
            user = std::min(user, cpuSeconds(after.ru_utime) - cpuSeconds(before.ru_utime));
            sys = std::min(sys, cpuSeconds(after.ru_stime) - cpuSeconds(before.ru_stime));
            same = same && printOf(*again) == prints.at(key);
        }
        std::printf("time %-13s level %d  wall %.2f s  user %.2f s  sys %.2f s  instructions %.4g  cycles %.4g  %s\n", what.c_str(), int(key.level), wall, user, sys,
                    instructions, cycles, same ? "same" : "DIFFERS");
        std::fflush(stdout);
    }
    return 0;
}
