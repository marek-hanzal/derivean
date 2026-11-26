#include "Arguments.hpp"

#include "chart/fn/Chart.hpp"
#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <array>
#include <chrono>
#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr Rgb LakeBlue{64, 128, 182}, StreamBlue{30, 90, 200}, IceWhite{245, 245, 245};

Rgb scaled(Rgb color, double factor) {
    const auto channel = [&](uint8_t value) { return uint8_t(std::clamp(value * factor, 0.0, 255.0)); };
    return {channel(color.r), channel(color.g), channel(color.b)};
}

Rgb blend(Rgb a, Rgb b, double t) {
    return {uint8_t(mix(a.r, b.r, t)), uint8_t(mix(a.g, b.g, t)), uint8_t(mix(a.b, b.b, t))};
}

// The fields of a chart as one quick picture: tinted relief, standing water, streams above
// `visible` m³/yr and glacier ice. Pixels read from the planet alone are greyed.
std::vector<uint8_t> paint(const Chart& chart, double seaLevel, double exaggeration, double visible) {
    Field<float> ground(chart.size, chart.size);
    ground.cells = chart.ground;
    std::vector<uint8_t> image(ground.size() * 3);
    for (int z = 0; z < chart.size; ++z)
        for (int x = 0; x < chart.size; ++x) {
            const size_t at = ground.index(x, z);
            const double shade = hillshade(ground, x, z, chart.pitch, exaggeration);
            const ChartWater waters = ChartWater(chart.waters[at]);
            Rgb color = scaled(hypsometricTint(std::max(1.0, chart.ground[at] - seaLevel)), shade);
            if (chart.ice[at] > 1) color = scaled(IceWhite, shade);
            if (waters == ChartWater::Sea) color = scaled(hypsometricTint(std::min(-1.0, double(chart.ground[at] - chart.water[at]))), 0.75 + 0.25 * shade);
            if (waters == ChartWater::Lake) color = scaled(LakeBlue, 0.75 + 0.25 * shade);
            if (waters != ChartWater::Sea && waters != ChartWater::Lake && chart.discharge[at] >= visible)
                color = blend(color, StreamBlue, std::min(1.0, 0.45 + 0.18 * std::log10(chart.discharge[at] / visible)));
            if (chart.level[at] == 0) color = blend(color, Rgb{128, 128, 128}, 0.25);
            putPixel(image, chart.size, x, z, color);
        }
    return image;
}

} // namespace

// terra chart --x metres --z metres --span metres [--size pixels] [--at landing|shore] [--refine 0..3]
// [--out file.png]: what the game's map is given for that square. --refine first builds every tile
// of the levels up to the given one under the square; without it the chart shows what happens to exist.
int chartCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    if (arguments.text("at", "") == "shore") focus = findShore(world.planet, focus);
    if (arguments.text("at", "") == "landing") focus = findLanding(world, focus);
    const ChartRequest request{focus, arguments.number("span", PlanetSpan), int(arguments.number("size", 512))};

    const int refine = std::clamp(int(arguments.number("refine", 0)), 0, int(Level::Site));
    for (int level = int(Level::Region); level <= refine; ++level) {
        const TileKey low = tileAt(Level(level), {focus.x - 0.5 * request.span, focus.z - 0.5 * request.span});
        const TileKey high = tileAt(Level(level), {focus.x + 0.5 * request.span, focus.z + 0.5 * request.span});
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x) world.atlas.request({Level(level), x, z});
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x) world.atlas.require({Level(level), x, z});
    }

    const size_t before = world.atlas.finished();
    const auto start = std::chrono::steady_clock::now();
    const Chart chart = chartOf(world, request);
    const double milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (chart.size <= 0) {
        std::fprintf(stderr, "chart: --span and --size must be positive\n");
        return 2;
    }

    std::array<size_t, 4> byLevel{}, byWater{};
    float low = 1e30f, high = -1e30f;
    for (size_t i = 0; i < chart.ground.size(); ++i) {
        ++byLevel[chart.level[i]];
        ++byWater[chart.waters[i]];
        low = std::min(low, chart.ground[i]);
        high = std::max(high, chart.ground[i]);
    }
    const double share = 100.0 / double(chart.ground.size());
    std::printf("centre %.0f %.0f  span %.0f m  %d px of %.2f m  read in %.1f ms  tiles %zu before, %zu after\n", focus.x, focus.z,
                request.span, chart.size, chart.pitch, milliseconds, before, world.atlas.finished());
    std::printf("read from: planet %.1f %%  region %.1f %%  local %.1f %%  site %.1f %%\n", byLevel[0] * share, byLevel[1] * share,
                byLevel[2] * share, byLevel[3] * share);
    std::printf("ground %.0f .. %.0f m  sea level %.0f m  dry %.1f %%  sea %.1f %%  lake %.2f %%  stream %.2f %%\n", low, high,
                world.planet.seaLevel, byWater[0] * share, byWater[1] * share, byWater[2] * share, byWater[3] * share);

    // Relief and streams are read at the scale of a pixel: flatter country at a coarser pitch needs more lift.
    const double exaggeration = arguments.number("exaggeration", std::clamp(0.35 * std::pow(chart.pitch, 0.45), 1.0, 60.0));
    const double visible = arguments.number("river", std::clamp(30.0 * chart.pitch * chart.pitch, 3e4, 3e11));
    const std::string out = arguments.text("out", "chart.png");
    if (!writePng(out, chart.size, chart.size, paint(chart, world.planet.seaLevel, exaggeration, visible))) {
        std::fprintf(stderr, "chart: cannot write %s\n", out.c_str());
        return 1;
    }
    std::printf("%s\n", out.c_str());
    return 0;
}
