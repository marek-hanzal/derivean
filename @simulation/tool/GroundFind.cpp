#include "Ground.hpp"

#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

struct Place {
    double score;
    Vec2 at;
    float ground, slope, flow, warmth, ice;
    Material rock;
};

struct Kind {
    const char* name;
    std::vector<Place> found;
};

} // namespace

// terra ground --find 1 [--x --z] [--radius m] [--out dir …]: places of each kind of ground around a
// point (the nearest hospitable shore by default), picked from the local level so the search stays
// cheap. With `--out` the first place of each kind is drawn there as `terra slice` would draw it.
int groundFind(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 centre{arguments.number("x", 0), arguments.number("z", 0)};
    if (!arguments.values.count("x")) centre = findShore(world.planet, centre);
    const double radius = arguments.number("radius", 40000), cell = specOf(Level::Local).cell;
    requestAround(world, centre, radius, Level::Local);
    Kind settings[] = {{"cliff in bedded rock", {}}, {"talus", {}}, {"steep headwater channel", {}}, {"lowland river reach", {}},
                          {"glaciated valley floor", {}}, {"cold summit", {}}, {"plain", {}}};
    const TileKey low = tileAt(Level::Local, {centre.x - radius, centre.z - radius}), high = tileAt(Level::Local, {centre.x + radius, centre.z + radius});
    for (int tz = low.z; tz <= high.z; ++tz)
        for (int tx = low.x; tx <= high.x; ++tx) {
            const auto tile = world.atlas.require({Level::Local, tx, tz});
            const auto top = [&](size_t k) { return tile->now.rock[k] + tile->now.sediment[0][k] + tile->now.sediment[1][k] + tile->now.sediment[2][k]; };
            for (int z = 2; z < TileCells - 2; ++z)
                for (int x = 2; x < TileCells - 2; ++x) {
                    const size_t i = size_t(z) * TileCells + size_t(x);
                    if (tile->water[i] > top(i) || top(i) < tile->seaLevel + 2) continue;
                    const float slope = float(std::hypot(top(i + 1) - top(i - 1), top(i + TileCells) - top(i - TileCells)) / (2 * cell));
                    const float cover = top(i) - tile->now.rock[i], coarse = tile->now.sediment[0][i], flow = tile->discharge[i];
                    const Material rock = Material(tile->rock[i]);
                    const MaterialDefinition& d = definitionOf(rock);
                    const bool peak = top(i) > top(i - 1) && top(i) > top(i + 1) && top(i) > top(i - TileCells) && top(i) > top(i + TileCells);
                    const Place place{0, {(tx * double(TileCells) + x + 0.5) * cell, (tz * double(TileCells) + z + 0.5) * cell}, top(i), slope, flow, tile->warmth[i], tile->iceAge[i], rock};
                    const auto offer = [&](int kind, double score) {
                        Place scored = place;
                        scored.score = score;
                        settings[kind].found.push_back(scored);
                    };
                    if (d.origin == Origin::Sedimentary && d.bedding >= 0.4f && d.stableSlope > 60 && cover < 1 && slope > 0.7f) offer(0, slope);
                    if (coarse > 1.5f && slope > 0.5f && slope < 0.85f) offer(1, coarse);
                    if (flow > 2e5f && flow < 4e6f && slope > 0.15f && slope < 0.5f) offer(2, slope * std::log10(flow));
                    if (flow > 3e8f && slope < 0.02f) offer(3, std::log10(flow));
                    if (tile->iceAge[i] > 40 && tile->ice[i] <= 0 && slope < 0.15f) offer(4, tile->iceAge[i]);
                    if (tile->warmth[i] < -1.5f && peak && slope < 0.25f && tile->ice[i] <= 0) offer(5, -tile->warmth[i]);
                    if (slope < 0.006f && flow < 1e6f && cover > 3 && tile->warmth[i] > 4) offer(6, cover);
                }
        }
    std::printf("ground settings within %.0f km of X %.0f Z %.0f: the best local cell of each kind, then the site cells of that kind in the tile around it\n", radius / 1000,
                centre.x, centre.z);
    const double fine = specOf(Level::Site).cell;
    std::vector<std::pair<Vec2, int>> shots;
    for (int kind = 0; kind < int(std::size(settings)); ++kind) {
        Kind& setting = settings[kind];
        std::sort(setting.found.begin(), setting.found.end(), [](const Place& a, const Place& b) { return a.score > b.score; });
        std::printf("%s: %zu local cells\n", setting.name, setting.found.size());
        // Spread the picks: the best, then one from further down the list.
        for (const size_t pick : {size_t(0), setting.found.size() / 20}) {
            if (pick >= setting.found.size()) continue;
            const Place& p = setting.found[pick];
            const TileKey key = tileAt(Level::Site, p.at);
            const auto tile = world.atlas.require(key);
            const auto top = [&](size_t k) { return tile->now.rock[k] + tile->now.sediment[0][k] + tile->now.sediment[1][k] + tile->now.sediment[2][k]; };
            double best = -1;
            size_t where = 0;
            for (int z = 2; z < TileCells - 2; ++z)
                for (int x = 2; x < TileCells - 2; ++x) {
                    const size_t i = size_t(z) * TileCells + size_t(x);
                    if (tile->water[i] > top(i)) continue;
                    const float slope = float(std::hypot(top(i + 1) - top(i - 1), top(i + TileCells) - top(i - TileCells)) / (2 * fine));
                    const float cover = top(i) - tile->now.rock[i], flow = tile->discharge[i];
                    double score = -1;
                    if (kind == 0 && tile->shed[i] > 0 && cover < 0.5f) score = tile->shed[i];
                    if (kind == 1 && tile->now.sediment[0][i] > 1 && slope > 0.5f && cover < 20) score = tile->now.sediment[0][i];
                    if (kind == 2 && flow > 1e5f && flow < 4e6f && slope > 0.12f && slope < 0.6f && cover < 3) score = slope * std::log10(flow);
                    if (kind == 3 && flow > 1e8f && slope < 0.03f) score = std::log10(flow);
                    if (kind == 4 && tile->iceAge[i] > 20 && tile->ice[i] <= 0 && slope < 0.2f) score = tile->now.sediment[0][i] + 0.01 * tile->iceAge[i];
                    if (kind == 5 && tile->warmth[i] < -1 && slope < 0.2f && tile->ice[i] <= 0) score = top(i);
                    if (kind == 6 && slope < 0.02f && flow < 3e4f && cover > 1) score = 1 - slope;
                    if (score > best) best = score, where = i;
                }
            if (best < 0) {
                std::printf("  local --x %.0f --z %.0f (%s, slope %.2f): no site cell of this kind\n", p.at.x, p.at.z, std::string(definitionOf(p.rock).name).c_str(), p.slope);
                continue;
            }
            const int x = int(where % TileCells), z = int(where / TileCells);
            const float slope = float(std::hypot(top(where + 1) - top(where - 1), top(where + TileCells) - top(where - TileCells)) / (2 * fine));
            const Vec2 at{(key.x * double(TileCells) + x + 0.5) * fine, (key.z * double(TileCells) + z + 0.5) * fine};
            std::printf("  --x %.0f --z %.0f   ground %.0f m  slope %.2f  cover %.1f/%.1f/%.1f  flow %.2g m3/yr  shed %.2g m/yr  warmth %.1f C  ice age %.0f m  %s\n", at.x,
                        at.z, top(where), slope, tile->now.sediment[0][where], tile->now.sediment[1][where], tile->now.sediment[2][where], tile->discharge[where],
                        tile->shed[where], tile->warmth[where], tile->iceAge[where], std::string(definitionOf(Material(tile->rock[where])).name).c_str());
            if (pick == 0) shots.push_back({at, kind});
        }
    }
    // The same world drawn at once: the levels above the ground are still being worked on, and a
    // place found by one build may be another place to the next.
    if (arguments.values.count("out")) {
        constexpr const char* Names[] = {"cliff", "talus", "headwater", "river", "glacial", "summit", "plain"};
        for (const auto& [at, kind] : shots) requestAround(world, at, 0.5 * arguments.number("span", 96) + 8, Level::Ground);
        for (const auto& [at, kind] : shots) {
            renderSlice(world, arguments, at.x, at.z, Names[kind]);
            reportGround(world, at, 100);
        }
    }
    return 0;
}
