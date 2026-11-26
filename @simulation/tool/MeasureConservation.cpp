#include "Measure.hpp"

#include "landscape/fn/Process.hpp"
#include "measure/fn/Plane.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

// Rock and loose material of the whole domain, m³ per m² of cell (so metres summed over cells).
struct Stock {
    double rock{}, loose{};
};

Stock stockOf(const Domain& domain) {
    Stock stock;
    for (size_t i = 0; i < domain.cells(); ++i) stock.rock += domain.ground.rock[i], stock.loose += domain.sedimentAt(i);
    return stock;
}

// A level, dry, flat floor of quartzite, which neither dissolves nor leaves solutes: whatever the
// slope processes do on it, rock plus loose material must stay what it was.
Domain floorDomain(Level level, double slope = 0) {
    PlaneSpec spec;
    spec.level = level;
    spec.slope = slope;
    spec.roughness = 0;
    spec.rock = Material::Quartzite;
    Domain domain = planeDomain(spec);
    std::fill(domain.rain.begin(), domain.rain.end(), 0.0f);
    return domain;
}

void run(const char* name, Domain domain, int steps) {
    FlowScratch scratch;
    const Stock before = stockOf(domain);
    std::printf("  %-44s start: loose %.1f", name, before.loose);
    for (int step = 1; step <= steps; ++step) {
        floodLevels(domain, scratch);
        shapeSlopes(domain, scratch);
        if (step != 1 && step != steps) continue;
        const Stock now = stockOf(domain);
        const double total = (now.rock + now.loose) - (before.rock + before.loose);
        std::printf("  | after %d step%s: loose %.1f, rock %+.1f, rock + loose %+.1f (%+.1f %% of the loose start)", step, step > 1 ? "s" : "", now.loose,
                    now.rock - before.rock, total, before.loose > 0 ? 100 * total / before.loose : 0.0);
    }
    std::printf("\n");
}

} // namespace

// terra measure conservation [--levels 23]: slope processes on closed, hand-built ground. Figures
// are metres summed over cells; a process that only moves material leaves rock + loose unchanged.
int measureConservation(const Arguments& arguments) {
    std::printf("conservation of slope processes (shapeSlopes alone, dry quartzite floor, no rain)\n");
    for (const char digit : arguments.text("levels", "23")) {
        if (digit < '1' || digit > '3') continue;
        const Level level = Level(digit - '0');
        const double cell = specOf(level).cell;
        std::printf("level %c (%.0f m cells)\n", digit, cell);
        const int middle = DomainCells / 2;

        Domain pile = floorDomain(level);
        pile.ground.sediment[Sand][pile.index(middle, middle)] = float(2 * cell);
        run("one column of sand two cells high", pile, 10);

        Domain ridge = floorDomain(level);
        for (int z = 40; z < DomainCells - 40; ++z) ridge.ground.sediment[Sand][ridge.index(middle, z)] = float(2 * cell);
        run("a wall of sand two cells high", ridge, 10);

        Domain dunes = floorDomain(level);
        for (int z = 1; z < DomainCells - 1; ++z)
            for (int x = 1; x < DomainCells - 1; ++x) dunes.ground.sediment[Sand][dunes.index(x, z)] = float(0.1 * cell * (1 + std::sin(x * 0.4) * std::sin(z * 0.4)));
        run("gentle dunes a fifth of a cell high", dunes, 10);

        // Steeper than sand holds (62 %) but not steeper than the rock does.
        Domain mantle = floorDomain(level, 0.9);
        for (int z = 1; z < DomainCells - 1; ++z)
            for (int x = 1; x < DomainCells - 1; ++x) mantle.ground.sediment[Sand][mantle.index(x, z)] = float(0.02 * cell);
        run("a 90 % slope under a thin mantle of sand", mantle, 10);

        Domain stack = floorDomain(level);
        stack.ground.rock[stack.index(middle, middle)] += float(4 * cell);
        run("one column of bare rock four cells high", stack, 10);
    }
    return 0;
}
