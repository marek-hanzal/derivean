#include "Arguments.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "inspect/fn/Layer.hpp"
#include "simulation/fn/Spawn.hpp"
#include "simulation/type/World.hpp"

#include <chrono>
#include <cstdio>

using namespace derivean::sim;

// terra map --level 1|2|3 --x metres --z metres --tiles n: relief of n×n tiles around a point.
int mapCommand(const Arguments& arguments) {
    const Level level = Level(int(arguments.number("level", 1)));
    const int count = int(arguments.number("tiles", 2));
    const double exaggeration = arguments.number("exaggeration", level == Level::Region ? 6.0 : 1.5);
    const std::string out = arguments.text("out", ".");
    auto clock = std::chrono::steady_clock::now();
    const auto lap = [&] {
        const auto now = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(now - clock).count();
        clock = now;
        return seconds;
    };
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    std::printf("planet %.2f s  sea level %.0f m\n", lap(), world.planet.seaLevel);

    Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    if (arguments.text("at", "") == "shore") focus = findShore(world.planet, focus);
    if (arguments.text("at", "") == "landing") focus = findLanding(world, focus);
    std::printf("focus %.0f %.0f\n", focus.x, focus.z);
    const TileKey center = tileAt(level, focus);
    const int first = -(count / 2);
    for (int dz = 0; dz < count; ++dz)
        for (int dx = 0; dx < count; ++dx) world.atlas.request({level, center.x + first + dx, center.z + first + dz});
    const int side = count * TileCells;
    Field<float> ground(side, side), water(side, side), flow(side, side);
    Field<uint8_t> rock(side, side), bare(side, side);
    Field<float> ice(side, side), iceAge(side, side);
    TileLedger ledger;
    for (int dz = 0; dz < count; ++dz) {
        for (int dx = 0; dx < count; ++dx) {
            const auto tile = world.atlas.require({level, center.x + first + dx, center.z + first + dz});
            for (int z = 0; z < TileCells; ++z)
                for (int x = 0; x < TileCells; ++x) {
                    const size_t from = size_t(z) * TileCells + size_t(x);
                    ground.at(dx * TileCells + x, dz * TileCells + z) = tile->now.rock[from] + tile->now.sediment[0][from] + tile->now.sediment[1][from] + tile->now.sediment[2][from];
                    water.at(dx * TileCells + x, dz * TileCells + z) = tile->water[from];
                    flow.at(dx * TileCells + x, dz * TileCells + z) = tile->discharge[from];
                    rock.at(dx * TileCells + x, dz * TileCells + z) = tile->rock[from];
                    ice.at(dx * TileCells + x, dz * TileCells + z) = tile->ice[from];
                    iceAge.at(dx * TileCells + x, dz * TileCells + z) = tile->iceAge.empty() ? 0.0f : tile->iceAge[from];
                    bare.at(dx * TileCells + x, dz * TileCells + z) = tile->now.sediment[0][from] + tile->now.sediment[1][from] + tile->now.sediment[2][from] < 2.0f;
                }
            if (arguments.number("verbose", 0) > 0) {
                double rain = 0, loose = 0, load = 0;
                for (size_t i = 0; i < tile->rain.size(); ++i) {
                    rain += tile->rain[i];
                    loose += tile->now.sediment[0][i] + tile->now.sediment[1][i] + tile->now.sediment[2][i];
                    load = std::max(load, double(tile->load[i]));
                }
                std::printf("tile %d %d pass %d: eroded %.3g deposited %.3g km3  mean rain %.2f m/yr  mean cover %.1f m  peak load %.3g m3/yr\n", tile->key.x, tile->key.z,
                            passOf(tile->key), tile->ledger.eroded / 1e9, tile->ledger.deposited / 1e9, rain / double(tile->rain.size()), loose / double(tile->rain.size()), load);
            }
            ledger.eroded += tile->ledger.eroded;
            ledger.deposited += tile->ledger.deposited;
            ledger.uplifted += tile->ledger.uplifted;
            ledger.dissolved += tile->ledger.dissolved;
            ledger.failed += tile->ledger.failed;
        }
    }
    const double seconds = lap();
    const double cell = specOf(level).cell, sea = world.planet.seaLevel;

    std::vector<uint8_t> image = reliefImage(ground, sea, cell, exaggeration, &water);
    if (arguments.text("layer", "relief") == "rock") {
        // Material colours under the hillshade; cover deeper than two metres is dimmed.
        for (int z = 0; z < side; ++z)
            for (int x = 0; x < side; ++x) {
                if (water.at(x, z) > ground.at(x, z) || ground.at(x, z) < sea) continue;
                const uint32_t color = definitionOf(Material(rock.at(x, z))).color;
                const double shade = hillshade(ground, x, z, cell, exaggeration) * (bare.at(x, z) ? 1.0 : 0.72);
                putPixel(image, side, x, z, {uint8_t(std::min(255.0, ((color >> 16) & 255) * shade)), uint8_t(std::min(255.0, ((color >> 8) & 255) * shade)),
                                             uint8_t(std::min(255.0, (color & 255) * shade))});
            }
    }
    // The inspector's own fields (inspect/fn/Layer.hpp) replace the relief when one of them is named.
    if (std::string legend; paintLayer(world, arguments.text("layer", "relief"), {level, center.x + first, center.z + first}, count, image, legend))
        std::printf("%s\n", legend.c_str());
    if (arguments.text("layer", "relief") == "ice") {
        // Present ice in white, ground that was under ice at the last glacial maximum tinted blue.
        size_t now = 0, then = 0;
        for (int z = 0; z < side; ++z)
            for (int x = 0; x < side; ++x) {
                const size_t at = (size_t(z) * size_t(side) + size_t(x)) * 3;
                if (iceAge.at(x, z) > 1) {
                    ++then;
                    image[at] = uint8_t(image[at] * 0.6), image[at + 1] = uint8_t(image[at + 1] * 0.75), image[at + 2] = uint8_t(std::min(255.0, image[at + 2] * 0.75 + 90));
                }
                if (ice.at(x, z) > 1) {
                    ++now;
                    image[at] = image[at + 1] = image[at + 2] = 245;
                }
            }
        std::printf("ice now on %.2f %% of cells, at the glacial maximum on %.2f %%\n", 100.0 * double(now) / double(ground.size()), 100.0 * double(then) / double(ground.size()));
    }
    // Rivers: tint by discharge on dry cells so the drainage reads on top of the relief.
    const double visible = arguments.number("river", level == Level::Region ? 3e8 : level == Level::Local ? 2e6 : 3e4);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            if (water.at(x, z) > ground.at(x, z) || flow.at(x, z) < visible) continue;
            const double strength = std::min(1.0, 0.45 + 0.18 * std::log10(flow.at(x, z) / visible));
            const size_t at = (size_t(z) * size_t(side) + size_t(x)) * 3;
            image[at] = uint8_t(image[at] * (1 - strength) + 30 * strength);
            image[at + 1] = uint8_t(image[at + 1] * (1 - strength) + 90 * strength);
            image[at + 2] = uint8_t(image[at + 2] * (1 - strength) + 200 * strength);
        }
    const std::string path = out + "/map-L" + std::to_string(int(level)) + ".png";
    writePng(path, side, side, image);

    int land = 0;
    double low = 1e9, high = -1e9, mean = 0;
    size_t peak = 0;
    for (size_t i = 0; i < ground.size(); ++i) {
        const double above = ground.cells[i] - sea;
        low = std::min(low, above);
        if (above > high) peak = i;
        high = std::max(high, above);
        if (above > 0) ++land, mean += above;
    }
    std::printf("level %d  %d x %d tiles of %.1f km  built %zu tiles in %.2f s\n", int(level), count, count, tileSpan(level) / 1000,
                world.atlas.finished(), seconds);
    std::printf("highest cell at map pixel %zu, %zu: world x %.0f z %.0f\n", peak % size_t(side), peak / size_t(side),
                ((center.x + first) * double(TileCells) + double(peak % size_t(side)) + 0.5) * cell, ((center.z + first) * double(TileCells) + double(peak / size_t(side)) + 0.5) * cell);
    std::printf("land %.1f %%  mean %.0f m  range %.0f .. %.0f m above sea\n", 100.0 * land / double(ground.size()), land ? mean / land : 0.0, low, high);
    std::printf("km3: uplifted %.4g eroded %.4g deposited %.4g dissolved %.4g failed %.4g\n", ledger.uplifted / 1e9, ledger.eroded / 1e9,
                ledger.deposited / 1e9, ledger.dissolved / 1e9, ledger.failed / 1e9);
    std::printf("%s\n", path.c_str());
    return 0;
}
