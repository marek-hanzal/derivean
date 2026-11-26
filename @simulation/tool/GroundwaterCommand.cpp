#include "Groundwater.hpp"

#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

const char* KindNames[4] = {"table", "contact", "fault", "karst"};

double percentile(std::vector<double>& values, double share) {
    if (values.empty()) return 0;
    std::sort(values.begin(), values.end());
    return values[std::min(values.size() - 1, size_t(share * double(values.size())))];
}

void printBudget(const WaterBudget& b, double area) {
    const double in = b.recharge + b.sunk;
    const auto line = [&](const char* name, double value) { std::printf("  %-34s %10.3g m3/yr  %7.1f mm/yr  %5.1f %% of what enters\n", name, value, 1000 * value / area, in > 0 ? 100 * value / in : 0.0); };
    std::printf("budget over %.0f km2 (rain %.0f mm/yr, of it %.0f mm/yr left by evaporation)\n", area / 1e6, 1000 * b.rain / area, 1000 * b.runoff / area);
    line("recharge through the ground", b.recharge);
    line("streams sinking in soluble rock", b.sunk);
    line("out at springs", b.springs);
    line("out as seepage", b.seepage);
    line("out as baseflow", b.baseflow);
    line("lost through the base", b.deep);
    line("followed no farther (underflow)", b.underflow);
    line("across the edge, in", b.lateralIn);
    line("across the edge, out", b.lateralOut);
    std::printf("  unaccounted: %.2e of what enters; rock leaving in solution %.3g m3/yr\n", imbalance(b), b.dissolved);
    // Until the landscape takes the recharge out of the runoff it routes (groundwater/fn/Recharge.hpp),
    // that water is on the surface in its streams and comes out of the ground again here.
    std::printf("  counted twice until the landscape is wired: %.1f %% of its runoff is also recharge; springs put %.1f %% of the runoff back on ground that already carries it\n",
                b.runoff > 0 ? 100 * b.recharge / b.runoff : 0.0, b.runoff > 0 ? 100 * b.springs / b.runoff : 0.0);
}

void printSprings(const WaterBlock& block, double land) {
    std::vector<double> flow[4], warm;
    double total[4] = {};
    int hot[6] = {}, steaming = 0, sinter = 0, travertine = 0;
    const Spring* hottest = nullptr;
    const Spring* largest = nullptr;
    const SpringRecord *hottestWhy = nullptr, *largestWhy = nullptr;
    for (const auto& tile : block.tiles)
        for (size_t i = 0; i < tile->springs.size(); ++i) {
            const Spring& spring = tile->springs[i];
            const int kind = int(tile->records[i].kind);
            flow[kind].push_back(1000.0 * spring.discharge), total[kind] += spring.discharge;
            warm.push_back(spring.temperature);
            ++hot[spring.temperature < 10 ? 0 : spring.temperature < 20 ? 1 : spring.temperature < 40 ? 2 : spring.temperature < 70 ? 3 : spring.steamShare > 0 ? 5 : 4];
            steaming += spring.steamShare > 0.02f, sinter += spring.silica > 0.02f, travertine += spring.carbonate > 0.02f;
            if (!hottest || spring.temperature > hottest->temperature) hottest = &spring, hottestWhy = &tile->records[i];
            if (!largest || spring.discharge > largest->discharge) largest = &spring, largestWhy = &tile->records[i];
        }
    std::printf("springs on %.0f km2 of land\n", land / 1e6);
    for (int kind = 0; kind < 4; ++kind) {
        const size_t count = flow[kind].size();
        std::printf("  %-8s %6zu  %7.3f per km2  %9.4f m3/s in all  l/s: median %.3g, 90th %.3g, largest %.4g\n", KindNames[kind], count, double(count) / (land / 1e6), total[kind],
                    percentile(flow[kind], 0.5), percentile(flow[kind], 0.9), percentile(flow[kind], 0.9999));
    }
    std::printf("  temperature C: <10: %d  10-20: %d  20-40: %d  40-70: %d  70-boiling: %d  boiling: %d | steam vents %d, sinter-laying %d, travertine-laying %d\n", hot[0], hot[1],
                hot[2], hot[3], hot[4], hot[5], steaming, sinter, travertine);
    const auto why = [](const char* title, const Spring& s, const SpringRecord& r) {
        std::printf("  %s: %s spring X %.0f Z %.0f at %.0f m, %.3g l/s, %.1f C, steam %.2f, silica %.3f carbonate %.3f kg/m3 to lay down\n", title, KindNames[int(r.kind)], s.position.x,
                    s.position.z, double(s.elevation), 1000.0 * s.discharge, double(s.temperature), double(s.steamShare), double(s.silica), double(s.carbonate));
        std::printf("      catchment %.3g km2 at %.0f mm/yr recharge, standing %.0f m above the vent; path %.0f m, down to %.0f m, %.3g yr underground; in %s%s%s; heat %.0f K/km (%.0f %% magmatic), rock there %.0f C; "
                    "fault closeness %.2f; passages %.0f m, %.3g yr to open\n",
                    r.catchment / 1e6, 1000.0 * r.recharge, double(r.drop), double(r.path), double(r.depth), double(r.years), definitionOf(r.aquifer).name.data(),
                    r.floor == Material::Air ? "" : " on ", r.floor == Material::Air ? "" : definitionOf(r.floor).name.data(), 1000.0 * r.heat.gradient, 100.0 * r.heat.magmatic,
                    double(r.reservoir), double(r.crush), double(r.passages), double(r.grew));
    };
    if (largest) why("largest", *largest, *largestWhy);
    if (hottest) why("hottest", *hottest, *hottestWhy);
}

void printCaves(const WaterBlock& block, double land) {
    constexpr float Bounds[5] = {0.5f, 1, 2, 5, 1e9f};
    double length[5] = {}, abandoned = 0, shafts = 0, volume = 0, karst = 0, thinnest = 1e9;
    size_t count = 0, swallows = 0;
    for (const auto& tile : block.tiles) {
        for (const uint8_t state : tile->state) karst += (state & (UpperKarst | LowerKarst)) && !(state & Drained) ? 4096.0 : 0.0, swallows += state & Swallows ? 1 : 0;
        for (size_t i = 0; i < tile->caves->conduits.size(); ++i) {
            const Conduit& c = tile->caves->conduits[i];
            const ConduitRecord& record = tile->passages[i];
            const double run = std::sqrt((c.bx - c.ax) * (c.bx - c.ax) + (c.by - c.ay) * (c.by - c.ay) + (c.bz - c.az) * (c.bz - c.az));
            ++count, volume += 3.14159265 * c.radius * c.radius * run;
            if (record.level > 0) abandoned += run;
            else if (record.order == 0 && std::abs(c.bx - c.ax) + std::abs(c.bz - c.az) < 1e-6) shafts += run;
            else length[c.radius < Bounds[0] ? 0 : c.radius < Bounds[1] ? 1 : c.radius < Bounds[2] ? 2 : c.radius < Bounds[3] ? 3 : 4] += run;
            thinnest = std::min(thinnest, double(record.cover));
        }
    }
    std::printf("caves: soluble ground that drains by conduits %.1f km2 (%.1f %% of the land), %zu cells where a stream sinks\n", karst / 1e6, land > 0 ? 100 * karst / land : 0.0, swallows);
    std::printf("  %zu conduits; km of passage carrying water by radius: <0.5 m %.1f, 0.5-1 m %.1f, 1-2 m %.1f, 2-5 m %.1f, >5 m %.1f; abandoned levels %.1f km; shafts %.2f km\n", count,
                length[0] / 1000, length[1] / 1000, length[2] / 1000, length[3] / 1000, length[4] / 1000, abandoned / 1000, shafts / 1000);
    std::printf("  void %.3g m3 = %.4f %% of the top 100 m of that ground; thinnest roof %.1f m\n", volume, karst > 0 ? 100 * volume / (karst * 100.0) : 0.0, count ? thinnest : 0.0);
}

// What the inspector would be told about the largest spring and the widest passage of the block.
void printAccounts(World& world, const WaterBlock& block) {
    const Spring* largest = nullptr;
    const Conduit* widest = nullptr;
    for (const auto& tile : block.tiles) {
        for (const Spring& spring : tile->springs)
            if (!largest || spring.discharge > largest->discharge) largest = &spring;
        for (const Conduit& conduit : tile->caves->conduits)
            if (!widest || conduit.radius > widest->radius) widest = &conduit;
    }
    if (largest) {
        const SpringAccount account = explainSpring(world, largest->position);
        std::printf("explainSpring at X %.0f Z %.0f: %s", largest->position.x, largest->position.z, account.found ? "" : "nothing found\n");
        if (account.found) {
            std::printf("%s spring, vent put on the site level's ground at X %.1f Z %.1f, %.1f m (local level: %.1f m); water came through", KindNames[int(account.record.kind)],
                        account.spring.position.x, account.spring.position.z, double(account.spring.elevation), double(largest->elevation));
            for (const Material rock : account.crossed) std::printf(" %s", definitionOf(rock).name.data());
            std::printf("\n");
        }
    }
    if (widest) {
        const CaveAccount account = explainCave(world, 0.5 * (widest->ax + widest->bx), 0.5 * (widest->ay + widest->by), 0.5 * (widest->az + widest->bz));
        std::printf("explainCave in the widest passage: %s", account.found ? "" : "nothing found\n");
        if (account.found)
            std::printf("radius %.1f m, %.0f l/s, cut in %s%s%s along a joint %s striking %.0f deg, level %d, catchment %.2f km2, %.3g yr to open, %.3g yr dry, %.1f m of roof; drains to spring %llu at X %.0f Z %.0f\n",
                        double(account.conduit.radius), 1000.0 * account.conduit.flux, definitionOf(account.record.rock).name.data(), account.record.floor == Material::Air ? "" : " on ",
                        account.record.floor == Material::Air ? "" : definitionOf(account.record.floor).name.data(), account.record.order == 2 ? "zone" : account.record.order == 1 ? "corridor" : "(none)",
                        57.2958 * account.record.strike, int(account.record.level), account.record.catchment / 1e6, double(account.record.grew), double(account.record.abandoned),
                        double(account.record.cover), static_cast<unsigned long long>(account.outlet.id), account.outlet.position.x, account.outlet.position.z);
    }
}

} // namespace

// terra groundwater [--x --z | --at landing|limestone|hot] [--span m] [--out directory] [--tag name]
// [--time 1] [--explain 1]: the ground water under a square of the world.
int groundwaterCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 centre{arguments.number("x", 0), arguments.number("z", 0)};
    const std::string place = arguments.text("at", "landing");
    if (!arguments.values.count("x") && !arguments.values.count("z")) centre = place == "landing" ? findLanding(world, centre) : seekWaterPlace(world, place);
    const double span = arguments.number("span", 16000);
    const TileKey first = tileAt(Level::Local, {centre.x - span / 2, centre.z - span / 2}), last = tileAt(Level::Local, {centre.x + span / 2, centre.z + span / 2});
    const int tiles = std::max(last.x - first.x, last.z - first.z) + 1;
    requestAround(world, centre, span / 2 + tileSpan(Level::Local), Level::Local);
    WaterBlock block{gatherMosaic(world, first, tiles), {}};
    WaterBudget budget;
    double land = 0, depth = 0;
    std::vector<double> depths;
    for (int z = 0; z < tiles; ++z)
        for (int x = 0; x < tiles; ++x) {
            block.tiles.push_back(groundwaterAt(world, {Level::Local, first.x + x, first.z + z}));
            const GroundwaterTile& tile = *block.tiles.back();
            const WaterBudget& b = tile.budget;
            budget.rain += b.rain, budget.runoff += b.runoff, budget.recharge += b.recharge, budget.sunk += b.sunk, budget.springs += b.springs, budget.seepage += b.seepage;
            budget.baseflow += b.baseflow, budget.deep += b.deep, budget.underflow += b.underflow, budget.dissolved += b.dissolved;
            // Water that crosses between two tiles of the block stays inside it: only the outer edge counts.
            budget.lateralIn += b.lateralIn - b.lateralOut;
            for (int cz = 0; cz < TileCells; ++cz)
                for (int cx = 0; cx < TileCells; ++cx) {
                    const size_t cell = size_t(cz) * TileCells + size_t(cx);
                    if (tile.state[cell] & Drained) continue;
                    land += 4096.0;
                    depths.push_back(block.ground.ground.at(x * TileCells + cx, z * TileCells + cz) - tile.table[cell]);
                    depth += depths.back();
                }
        }
    std::printf("groundwater  seed %llu  age %.3g yr  centre X %.0f Z %.0f  %d x %d local tiles from key %d %d\n", static_cast<unsigned long long>(arguments.seed()), world.options.age,
                centre.x, centre.z, tiles, tiles, first.x, first.z);
    printBudget(budget, double(tiles) * tiles * tileSpan(Level::Local) * tileSpan(Level::Local));
    std::printf("depth to water on land: mean %.1f m, median %.1f m, 90th %.1f m, deepest %.0f m\n", depths.empty() ? 0.0 : depth / double(depths.size()), percentile(depths, 0.5),
                percentile(depths, 0.9), percentile(depths, 0.99999));
    printSprings(block, land);
    printCaves(block, land);
    if (arguments.values.count("out")) writeWaterMaps(block, arguments.text("out", "."), arguments.text("tag", "here"));
    if (arguments.number("time", 0) > 0) timeGroundwater(world, tileAt(Level::Local, centre));
    if (arguments.number("explain", 0) > 0) printAccounts(world, block);
    return 0;
}
