#include "Coast.hpp"

#include "Measure.hpp"

#include "coast/fn/Scenario.hpp"
#include "measure/constant/Angle.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

Material rockNamed(const std::string& name, Material fallback) {
    for (int i = 0; i < MaterialCount; ++i) {
        std::string known(definitionOf(Material(i)).name);
        for (char& letter : known) letter = char(std::tolower(letter));
        if (known == name) return Material(i);
    }
    return fallback;
}

void printKinds(const char* water, const double length[]) {
    double total = 0;
    for (int k = 1; k < int(ShoreKind::Count); ++k) total += length[k];
    if (total <= 0) return;
    std::printf("  %s shore %.1f km:", water, total / 1000);
    for (int k = 1; k < int(ShoreKind::Count); ++k) std::printf("  %s %.1f km (%.0f %%)", ShoreKindNames[k], length[k] / 1000, 100 * length[k] / total);
    std::printf("\n");
}

// terra coast --scene straight|bay|lake [--level 2|3] [--rock name] [--other name] [--wind-from deg] [--wind m/s] [--swell m]
//             [--sand m] [--gravel m] [--mud m] [--joints 0..1] [--river m3/yr] [--upland m] [--shelf m] [--steps n] [--waves-only 1]
// A hand-built shore (coast/fn/Scenario.hpp) replayed by the level's whole step loop, or with
// --waves-only 1 by the coast alone, with its statistics, a map and a view from the sea.
int sceneCommand(const Arguments& arguments) {
    CoastSpec spec;
    const std::string scene = arguments.text("scene", "straight");
    spec.scene = scene == "bay" ? CoastScene::Bay : scene == "lake" ? CoastScene::Lake : CoastScene::Straight;
    spec.level = Level(int(arguments.number("level", 2)));
    spec.rock = rockNamed(arguments.text("rock", "sandstone"), Material::Sandstone);
    spec.other = rockNamed(arguments.text("other", arguments.text("rock", "sandstone")), spec.rock);
    spec.windFrom = arguments.number("wind-from", 0), spec.wind = arguments.number("wind", 7), spec.swell = arguments.number("swell", 3);
    spec.sand = arguments.number("sand", 0), spec.gravel = arguments.number("gravel", 0), spec.mud = arguments.number("mud", 0);
    spec.joints = arguments.number("joints", 0), spec.river = arguments.number("river", 0), spec.upland = arguments.number("upland", 60);
    spec.shelf = arguments.number("shelf", 15);
    spec.steps = int(arguments.number("steps", 0));
    spec.seed = arguments.seed();
    Lap lap;
    Domain domain;
    if (arguments.number("waves-only", 0) > 0) {
        domain = coastDomain(spec);
        workCoast(domain, int(std::llround((domain.endTime - domain.startTime) / domain.step)));
    } else {
        domain = replayCoast(spec);
    }
    const double seconds = lap();
    const std::string name = arguments.text("name", scene);
    std::printf("coast scene %s  level %d  %.0f m cells  %d steps in %.1f s  rock %s / %s  wind %.0f m/s from %.0f deg  swell %.1f m\n", scene.c_str(), int(spec.level),
                domain.cell, domain.taken, seconds, std::string(definitionOf(spec.rock).name).c_str(), std::string(definitionOf(spec.other).name).c_str(), spec.wind, spec.windFrom,
                spec.swell);
    const ShoreGrid grid = shoreGridOf(domain);
    const ShoreSurvey survey = surveyShore(grid);
    printShore(grid, survey);
    if (domain.shore) printCoastLedger(domain.shore->ledger, grid.cell * grid.cell * double(grid.rock.size()));
    const std::string out = arguments.text("out", ".");
    writeShoreMap(grid, survey, out + "/coast-" + name + ".png", int(arguments.number("zoom", 3)));
    std::printf("  %s\n", (out + "/coast-" + name + ".png").c_str());
    if (arguments.number("view", 1) > 0) {
        const bool lake = spec.scene == CoastScene::Lake;
        const double span = domain.size * domain.cell, middle = domain.origin.x + 0.5 * span;
        const std::string path = out + "/coast-" + name + "-view.png";
        writeShoreView(domain, {middle + arguments.number("eye-x", lake ? 0.0 : -0.12) * span, middle + arguments.number("eye-z", lake ? 0.3 : -0.2) * span},
                       {middle + arguments.number("to-x", 0.03) * span, middle + arguments.number("to-z", lake ? 0.21 : 0.02) * span}, arguments.number("above", lake ? 12 : 0.05 * span), path);
        std::printf("  %s\n", path.c_str());
    }
    return 0;
}

} // namespace

void printShore(const ShoreGrid& grid, const ShoreSurvey& s) {
    printKinds("sea", s.sea);
    printKinds("lake", s.lake);
    std::printf("  waves at the shore: p50 %.2f m  p95 %.2f m  highest %.2f m\n", s.struck.p50, s.struck.p95, s.struck.high);
    std::printf("  cliffs (%zu shore cells): height p50 %.0f m  p95 %.0f m  highest %.0f m | platform before %.1f km of rock shore: width p50 %.0f m  p95 %.0f m  widest %.0f m\n",
                s.cliffHeight.count, s.cliffHeight.p50, s.cliffHeight.p95, s.cliffHeight.high, s.benched / 1000, s.platformWidth.p50, s.platformWidth.p95, s.platformWidth.high);
    std::printf("  beach width: sea p50 %.0f m  p95 %.0f m (%zu) | lake p50 %.0f m  p95 %.0f m (%zu)\n", s.beachWidth[0].p50, s.beachWidth[0].p95, s.beachWidth[0].count,
                s.beachWidth[1].p50, s.beachWidth[1].p95, s.beachWidth[1].count);
    std::printf("  slope at the waterline, p50: gravel %.1f deg (%zu)  sand %.1f deg (%zu)  mud %.1f deg (%zu)  [a cell of %.0f m cannot show a slope steeper over less than its width]\n",
                std::atan(s.beachSlope[Coarse].p50) * DegreesPerRadian, s.beachSlope[Coarse].count, std::atan(s.beachSlope[Sand].p50) * DegreesPerRadian, s.beachSlope[Sand].count,
                std::atan(s.beachSlope[Fines].p50) * DegreesPerRadian, s.beachSlope[Fines].count, grid.cell);
    std::printf("  spits and barriers: %.1f km of shore on strips with water behind | lagoon %.2f km2 with %.1f km of shore | stacks %d\n", s.strip / 1000, s.lagoon / 1e6,
                s.lagoonShore / 1000, s.stacks);
}

void printCoastLedger(const CoastLedger& ledger, double area) {
    std::printf("  waves, m of ground over the square: cut from rock %.3f  of it dissolved %.3f  sand and gravel handed in at the edge %.3f  carried out %.3f\n", ledger.cut / area,
                ledger.dissolved / area, ledger.entered / area, ledger.left / area);
}

// terra coast --x m --z m [--level 2|3] [--tiles n] [--out directory] [--name tag], or --scene (above), or --find (CoastFind.cpp).
// What the shores of n × n tiles around a place are made of: length by kind, cliff heights, platform
// and beach widths, spits and lagoons; with --out a map of them.
int coastCommand(const Arguments& arguments) {
    if (arguments.values.count("scene")) return sceneCommand(arguments);
    if (arguments.values.count("find")) return coastFind(arguments);
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const Level level = Level(int(arguments.number("level", 2)));
    const int count = int(arguments.number("tiles", 2));
    const TileKey middle = tileAt(level, {arguments.number("x", 0), arguments.number("z", 0)});
    const TileKey first{level, middle.x - count / 2, middle.z - count / 2};
    std::vector<std::shared_ptr<const Tile>> held;
    std::vector<const Tile*> tiles;
    for (int dz = 0; dz < count; ++dz)
        for (int dx = 0; dx < count; ++dx) world.atlas.request({level, first.x + dx, first.z + dz});
    CoastLedger ledger;
    for (int dz = 0; dz < count; ++dz)
        for (int dx = 0; dx < count; ++dx) {
            held.push_back(world.atlas.require({level, first.x + dx, first.z + dz}));
            tiles.push_back(held.back().get());
            if (const ShoreRecord* shore = held.back()->shore.get())
                ledger.cut += shore->ledger.cut, ledger.dissolved += shore->ledger.dissolved, ledger.entered += shore->ledger.entered, ledger.left += shore->ledger.left;
        }
    const ShoreGrid grid = shoreGridOf(tiles, count);
    const ShoreSurvey survey = surveyShore(grid);
    std::printf("coast  seed %llu  level %d  %d x %d tiles from X %.0f Z %.0f  %.1f km across  %.0f m cells  sea level %.1f m  (%zu tiles in %.0f s)\n",
                static_cast<unsigned long long>(arguments.seed()), int(level), count, count, first.x * tileSpan(level), first.z * tileSpan(level), grid.side * grid.cell / 1000, grid.cell,
                grid.seaLevel, world.atlas.finished(), lap());
    printShore(grid, survey);
    printCoastLedger(ledger, grid.cell * grid.cell * double(grid.rock.size()));
    if (const std::string out = arguments.text("out", ""); !out.empty()) {
        const std::string path = out + "/coast-" + arguments.text("name", "place") + "-L" + std::to_string(int(level)) + ".png";
        writeShoreMap(grid, survey, path, int(arguments.number("zoom", 1)));
        std::printf("  %s\n", path.c_str());
    }
    return 0;
}
