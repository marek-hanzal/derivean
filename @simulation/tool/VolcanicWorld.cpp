#include "Volcanic.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "simulation/type/World.hpp"

#include <chrono>
#include <cstdio>
#include <map>

using namespace derivean::sim;

namespace {

constexpr double Km3 = 1e9;

// Finished tiles of one level side by side, with what the report needs of each cell.
struct Sheet {
    HeightGrid grid;
    std::vector<float> water, laid;
    std::vector<uint8_t> lava;
    std::vector<uint64_t> vent;   // id of the vent that laid the top, 0 for none
    double seaLevel{};
};

Sheet gather(World& world, Level level, Vec2 centre, int tiles, std::map<uint64_t, Vent>& vents, VolcanicLedger& ledger, double& seconds) {
    const TileKey middle = tileAt(level, centre);
    const TileKey first{level, middle.x - tiles / 2, middle.z - tiles / 2};
    const int side = tiles * TileCells;
    Sheet sheet;
    sheet.grid = {side, side, specOf(level).cell, {first.x * tileSpan(level), first.z * tileSpan(level)}, std::vector<float>(size_t(side) * side), std::vector<float>(size_t(side) * side, 0.0f)};
    sheet.water.assign(sheet.grid.top.size(), -1e9f);
    sheet.laid.assign(sheet.grid.top.size(), 0.0f);
    sheet.lava.assign(sheet.grid.top.size(), 0);
    sheet.vent.assign(sheet.grid.top.size(), 0);
    for (int tz = 0; tz < tiles; ++tz)
        for (int tx = 0; tx < tiles; ++tx) world.atlas.request({level, first.x + tx, first.z + tz});
    const auto started = std::chrono::steady_clock::now();
    for (int tz = 0; tz < tiles; ++tz) {
        for (int tx = 0; tx < tiles; ++tx) {
            const auto tile = world.atlas.require({level, first.x + tx, first.z + tz});
            sheet.seaLevel = tile->seaLevel;
            for (int z = 0; z < TileCells; ++z) {
                for (int x = 0; x < TileCells; ++x) {
                    const size_t from = size_t(z) * TileCells + size_t(x), to = size_t(tz * TileCells + z) * size_t(side) + size_t(tx * TileCells + x);
                    sheet.grid.top[to] = tile->now.rock[from] + tile->now.sediment[0][from] + tile->now.sediment[1][from] + tile->now.sediment[2][from];
                    sheet.water[to] = tile->water[from];
                    const PileColumn column = pileColumn(*tile, from);
                    sheet.grid.pile[to] = column.thickness, sheet.laid[to] = column.laid, sheet.lava[to] = column.lava;
                    if (tile->volcanic && column.vent < tile->volcanic->vents.size()) sheet.vent[to] = tile->volcanic->vents[column.vent].id;
                }
            }
            if (!tile->volcanic) continue;
            for (const Vent& vent : tile->volcanic->vents) vents.emplace(vent.id, vent);
            const VolcanicLedger& own = tile->volcanic->ledger;
            ledger.erupted += own.erupted, ledger.lava += own.lava, ledger.cone += own.cone, ledger.ash += own.ash, ledger.left += own.left, ledger.drifted += own.drifted;
            ledger.sealed += own.sealed, ledger.foundered += own.foundered, ledger.built += own.built;
        }
    }
    seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return sheet;
}

void writeMap(const Sheet& sheet, const std::string& path) {
    const int side = sheet.grid.width;
    Field<float> ground(side, side), water(side, side);
    ground.cells = sheet.grid.top, water.cells = sheet.water;
    std::vector<uint8_t> image = reliefImage(ground, sheet.seaLevel, sheet.grid.cell, 2.0, &water);
    for (size_t i = 0; i < sheet.grid.top.size(); ++i) {
        if (sheet.grid.pile[i] <= 0 || sheet.water[i] > sheet.grid.top[i]) continue;
        const float lava = float(sheet.lava[i]) / 255.0f;
        image[i * 3] = uint8_t(image[i * 3] * (0.55f + 0.45f * (1 - lava)));
        image[i * 3 + 1] = uint8_t(image[i * 3 + 1] * (0.5f + 0.3f * (1 - lava)));
        image[i * 3 + 2] = uint8_t(image[i * 3 + 2] * 0.5f);
    }
    writePng(path, side, side, image);
}

// Tiles are built apart, so a pile or a flow that crosses their edge must come out whole: the step
// in pile thickness and in lava share between two cells across an edge is set against the same step
// between two cells beside each other within a tile, and the vent that laid the top must be the same
// on both sides about as often as it is inside.
void printSeams(const Sheet& sheet, int tiles) {
    if (tiles < 2) return;
    const int side = sheet.grid.width;
    struct Steps {
        double pile{}, lava{}, ground{}, worst{};
        long pairs{}, sameVent{}, vented{}, wet{};
        size_t at{};
    } across, inside;
    const auto compare = [&](size_t a, size_t b, Steps& steps) {
        if (sheet.grid.pile[a] <= 0 && sheet.grid.pile[b] <= 0) return;
        const double step = std::abs(sheet.grid.pile[a] - sheet.grid.pile[b]);
        steps.pile += step;
        steps.ground += std::abs(sheet.grid.top[a] - sheet.grid.top[b]);
        steps.wet += sheet.grid.top[a] < sheet.seaLevel;
        if (step > steps.worst) steps.worst = step, steps.at = a;
        steps.lava += std::abs(int(sheet.lava[a]) - int(sheet.lava[b])) / 255.0;
        ++steps.pairs;
        if (sheet.vent[a] && sheet.vent[b]) ++steps.vented, steps.sameVent += sheet.vent[a] == sheet.vent[b];
    };
    // A pair whose second cell starts a tile is across an edge; one just past the band a seam is
    // blended over (Apron cells) on either side of it is inside.
    const auto kind = [&](int next) -> Steps* {
        const int within = next % TileCells;
        return within == 0 ? &across : within == Apron + 2 || within == TileCells - Apron - 2 ? &inside : nullptr;
    };
    for (int a = 0; a < side; ++a) {
        for (int b = 1; b < side; ++b) {
            if (Steps* into = kind(b)) {
                compare(size_t(a) * side + size_t(b - 1), size_t(a) * side + size_t(b), *into);
                compare(size_t(b - 1) * side + size_t(a), size_t(b) * side + size_t(a), *into);
            }
        }
    }
    const auto mean = [](double sum, long n) { return n ? sum / double(n) : 0.0; };
    std::printf("  seams (1 = invisible): pile step %.2f (%.1f m across, %.1f m inside), lava share step %.2f (%.3f, %.3f), same top vent %.0f %% across, %.0f %% inside (%ld, %ld pairs)\n",
                mean(across.pile, across.pairs) / std::max(1e-6, mean(inside.pile, inside.pairs)), mean(across.pile, across.pairs), mean(inside.pile, inside.pairs),
                mean(across.lava, across.pairs) / std::max(1e-6, mean(inside.lava, inside.pairs)), mean(across.lava, across.pairs), mean(inside.lava, inside.pairs),
                100.0 * mean(double(across.sameVent), across.vented), 100.0 * mean(double(inside.sameVent), inside.vented), across.pairs, inside.pairs);
    std::printf("    the ground surface over those pairs steps %.1f m across, %.1f m inside; %.0f %% of them under the sea; worst pile step across %.0f m at X %.0f Z %.0f\n",
                mean(across.ground, across.pairs), mean(inside.ground, inside.pairs), 100.0 * mean(double(across.wet), across.pairs), across.worst,
                sheet.grid.origin.x + (double(across.at % size_t(side)) + 0.5) * sheet.grid.cell, sheet.grid.origin.z + (double(across.at / size_t(side)) + 0.5) * sheet.grid.cell);
}

} // namespace

int volcanicWorld(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const Vec2 centre{arguments.number("x", 0), arguments.number("z", 0)};
    const Level level = Level(int(arguments.number("level", 1)));
    const int tiles = int(arguments.number("tiles", 1));
    std::map<uint64_t, Vent> vents;
    VolcanicLedger ledger;
    double seconds = 0;
    const Sheet sheet = gather(world, level, centre, tiles, vents, ledger, seconds);
    const HeightGrid& grid = sheet.grid;
    const double area = grid.cell * grid.cell, span = grid.width * grid.cell;
    long land = 0, volcanic = 0, lavaCells = 0, young = 0;
    double pile = 0;
    for (size_t i = 0; i < grid.top.size(); ++i) {
        const bool dry = grid.top[i] > sheet.seaLevel && sheet.water[i] <= grid.top[i];
        land += dry;
        pile += grid.pile[i] * area;
        if (!dry || grid.pile[i] < 1) continue;
        ++volcanic;
        lavaCells += sheet.lava[i] >= 128;
        young += sheet.lava[i] >= 128 && sheet.laid[i] <= 4e5;
    }
    std::printf("volcanic  seed %llu  level %d  %d x %d tiles at X %.0f Z %.0f built in %.1f s\n", static_cast<unsigned long long>(arguments.seed()), int(level), tiles, tiles, centre.x, centre.z, seconds);
    std::printf("  land %.0f km2, volcanic rock at the surface on %.1f %% of it (Earth 6 %%): lava %.0f km2, of it laid within 400 000 yr %.0f km2; pile now %.0f km3\n", land * area / 1e6,
                100.0 * volcanic / std::max(1L, land), lavaCells * area / 1e6, young * area / 1e6, pile / Km3);
    std::printf("  ledgers of these tiles, km3: erupted %.1f = lava %.1f + fall and flows %.1f + ash %.1f + left %.1f + drifted %.1f | on the tiles' own cells %.1f | sealed cover %.1f | foundered %.1f\n",
                ledger.erupted / Km3, ledger.lava / Km3, ledger.cone / Km3, ledger.ash / Km3, ledger.left / Km3, ledger.drifted / Km3, ledger.built / Km3, ledger.sealed / Km3, ledger.foundered / Km3);
    printSeams(sheet, tiles);
    std::printf("  vents whose conduits come up here (X, Z in m; ages before the world's age):\n");
    for (const auto& [id, vent] : vents) {
        const Feeder& feeder = vent.feeders.back();
        if (feeder.site.x < grid.origin.x || feeder.site.z < grid.origin.z || feeder.site.x >= grid.origin.x + span || feeder.site.z >= grid.origin.z + span) continue;
        const EdificeShape shape = measureEdifice(grid, feeder.site, level == Level::Region ? 25e3 : 4e3);
        double largest = 0;
        for (const Episode& episode : vent.episodes) largest = std::max(largest, episode.paroxysm);
        std::printf("    X %.0f Z %.0f %-8s %6.0f km3 in %2zu steps over %4.1f Myr, last %4.1f Myr ago, conduit used %4.1f to %4.1f Myr ago (%5.0f km3), largest paroxysm of a step %5.1f km3, now %.2f km3/kyr | "
                    "%4.0f m over its foot, summit %5.0f m, radius %4.1f km, slopes %4.1f / %4.1f / %4.1f deg, pit %4.0f m\n",
                    feeder.site.x, feeder.site.z, vent.magma == Magma::Basalt ? "basalt" : "andesite", vent.erupted / Km3, vent.episodes.size(), (vent.until - vent.episodes.front().time) / 1e6,
                    (vent.until - vent.episodes.back().time - RegionStep) / 1e6, (vent.until - feeder.first) / 1e6, (vent.until - feeder.last) / 1e6, feeder.volume / Km3, largest / Km3,
                    vent.supply * 1000 / Km3, shape.height, shape.summit, shape.radius / 1000, shape.topSlope, shape.midSlope, shape.footSlope, shape.pit);
    }
    const std::string path = arguments.text("out", ".") + "/volcanic-" + arguments.text("name", "place") + "-map.png";
    writeMap(sheet, path);
    std::printf("%s\n", path.c_str());
    return 0;
}
