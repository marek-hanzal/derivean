#include "Arguments.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "intervention/fn/Scenario.hpp"

#include <chrono>
#include <cstdio>

using namespace derivean::sim;

namespace {

template<typename Pick>
void row(const ScenarioReport& report, const char* name, Pick&& pick, const char* format = " %15.6g") {
    std::printf("  %-28s", name);
    for (const ScenarioStage& stage : report.stages) std::printf(format, pick(stage.budget));
    std::printf("\n");
}

// Every store and every flow of one quantity at every stage.
void printTally(const ScenarioReport& report, const char* unit, double Tally::*part) {
    std::printf("%s\n  %-28s", unit, "");
    for (const ScenarioStage& stage : report.stages) std::printf(" %15.15s", stage.label.c_str());
    std::printf("\n");
    row(report, "ocean", [&](const WorldBudget& b) { return b.ocean.*part; });
    row(report, "other bodies", [&](const WorldBudget& b) { return b.others.*part; });
    row(report, "  ocean and others", [&](const WorldBudget& b) { return b.ocean.*part + b.others.*part; }, " %15.12g");
    row(report, "moved between bodies", [&](const WorldBudget& b) { return b.moved.*part; });
    row(report, "  unbooked share", [&](const WorldBudget& b) { return b.unbooked.*part; });
    row(report, "on live tiles", [&](const WorldBudget& b) { return b.live.*part; }, " %15.9g");
    row(report, "  adopted with the tiles", [&](const WorldBudget& b) { return b.adopted.*part; }, " %15.9g");
    row(report, "  + rivers in", [&](const WorldBudget& b) { return b.flows.inflow.*part; }, " %15.9g");
    row(report, "  + rain and runoff", [&](const WorldBudget& b) { return b.flows.rain.*part; }, " %15.9g");
    row(report, "  + from rim water", [&](const WorldBudget& b) { return b.flows.fromBody.*part; }, " %15.9g");
    row(report, "  - evaporation", [&](const WorldBudget& b) { return b.flows.evaporation.*part; }, " %15.9g");
    row(report, "  - out across the rim", [&](const WorldBudget& b) { return b.flows.outflow.*part; }, " %15.9g");
}

void printReport(const ScenarioReport& report) {
    std::printf("\n== %s ==\n", report.title.c_str());
    if (!report.failure.empty()) std::printf("FAILED: %s\n", report.failure.c_str());
    if (report.stages.empty()) return;
    for (size_t stage = 0; stage < report.stages.size(); ++stage) std::printf("  stage %zu: %s\n", stage, report.stages[stage].label.c_str());
    printTally(report, "water, m3", &Tally::water);
    row(report, "  unaccounted share", waterImbalance, " %15.2e");
    printTally(report, "salt, kg", &Tally::salt);
    row(report, "  unaccounted share", saltImbalance, " %15.2e");
    std::printf("solid, m3\n");
    row(report, "dug by the player", [](const WorldBudget& b) { return b.dug; });
    row(report, "built by the player", [](const WorldBudget& b) { return b.filled; });
    row(report, "carried in by rivers", [](const WorldBudget& b) { return b.solid.carriedIn; });
    row(report, "cut by running water", [](const WorldBudget& b) { return b.solid.eroded; });
    row(report, "laid in standing water", [](const WorldBudget& b) { return b.solid.deposited; });
    row(report, "carried out across the rim", [](const WorldBudget& b) { return b.solid.exported; });
    row(report, "columns gained in all", [](const WorldBudget& b) { return b.groundChange; }, " %15.9g");
    row(report, "  unaccounted share", solidImbalance, " %15.2e");
    std::printf("measured\n");
    for (size_t i = 0; i < report.measures.size(); ++i) {
        std::printf("  %-28s", report.measures[i].c_str());
        for (const ScenarioStage& stage : report.stages) std::printf(" %15.6g", stage.values[i]);
        std::printf("\n");
    }
}

// Ground and standing water of the first live tile.
void saveMap(World& world, const std::string& path) {
    if (world.interventions.overlay.tiles.empty()) return;
    const EditedTile& tile = *world.interventions.overlay.tiles.begin()->second;
    Field<float> ground(TileCells, TileCells), water(TileCells, TileCells);
    for (size_t cell = 0; cell < ground.size(); ++cell) {
        ground.cells[cell] = tile.now.rock[cell] + tile.now.sediment[0][cell] + tile.now.sediment[1][cell] + tile.now.sediment[2][cell];
        water.cells[cell] = tile.water[cell];
    }
    writePng(path, TileCells, TileCells, reliefImage(ground, world.ocean.pristineLevel, specOf(Level::Site).cell, 4.0, &water));
    std::printf("%s\n", path.c_str());
}

} // namespace

// terra intervention ocean|cascade|diversion|all [--seed n] [--age years] [--years n] [--out directory]
int interventionCommand(const std::string& scenario, const Arguments& arguments) {
    if (scenario != "ocean" && scenario != "cascade" && scenario != "diversion" && scenario != "all") {
        std::fprintf(stderr, "usage: terra intervention <ocean|cascade|diversion|all> [--seed n] [--age years] [--years n] [--out directory]\n");
        return 2;
    }
    World world({arguments.seed(), arguments.number("age", 1.2e9), unsigned(arguments.number("threads", 0))});
    const std::string out = arguments.text("out", "");
    auto clock = std::chrono::steady_clock::now();
    const auto lap = [&] {
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - clock).count();
        clock = std::chrono::steady_clock::now();
        return seconds;
    };
    if (scenario == "ocean" || scenario == "all") {
        printReport(runStrait(world));
        std::printf("ran in %.3f s\n", lap());
    }
    if (scenario == "ocean") return 0;
    const StreamSite site = findStream(world);
    if (!site.found) {
        std::fprintf(stderr, "no stream site found\n");
        return 1;
    }
    world.atlas.require(site.tile);
    std::printf("\nstream site: tile %d %d at %.0f %.0f (found and built in %.1f s)\n", site.tile.x, site.tile.z, site.at.x, site.at.z, lap());
    if (scenario == "cascade" || scenario == "all") {
        printReport(runLakeCascade(world, site));
        std::printf("ran in %.3f s\n", lap());
        if (!out.empty()) saveMap(world, out + "/cascade.png");
    }
    if (scenario == "diversion" || scenario == "all") {
        const double years = arguments.number("years", 100);
        const ScenarioReport report = runDiversion(world, site, years);
        printReport(report);
        std::printf("ran in %.3f s\n", lap());
        if (!out.empty()) saveMap(world, out + "/diversion.png");
        // The same history in one jump from the untouched world.
        const double now = world.interventions.calendar.seconds;
        jumpTo(world, 0);
        lap();
        jumpTo(world, now);
        std::printf("replayed %.0f yr of history in one jump: %.3f s\n", now / YearSeconds, lap());
    }
    return 0;
}
