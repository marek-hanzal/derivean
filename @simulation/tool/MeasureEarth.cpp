#include "Measure.hpp"

#include "measure/fn/Earth.hpp"

#include <cstdio>

using namespace derivean::sim;

// terra measure earth [--tiles n]: the planet next to the whole-Earth rows of
// docs/reference-landscapes.md. The kilometre-scale rows are read from first-pass Region tiles
// spread evenly over the planet, which wait for no neighbour; n is how many of them (0 skips them).
int measureEarth(const Arguments& arguments) {
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const PlanetLikeness p = likenessOf(world.planet);
    std::printf("earth  seed %llu  age %.3g yr  sea level %.0f m  planet %.0f s        | ours | Earth\n", static_cast<unsigned long long>(arguments.seed()), world.options.age,
                world.planet.seaLevel, lap());
    std::printf("  land, share of the surface                               | %.1f %% | 29 %%\n", 100 * p.landShare);
    constexpr const char* Bands[7] = {"below 200 m", "200-500 m", "500-1000 m", "1000-2000 m", "2000-3000 m", "3000-4000 m", "above 4000 m"};
    constexpr double EarthBands[7] = {29, 28, 24, 13, 3, 1, 2};
    for (int band = 0; band < 7; ++band) std::printf("  land %-52s | %.1f %% | %.0f %%\n", Bands[band], 100 * p.bands[band], EarthBands[band]);
    std::printf("  land height: mean; quartiles                             | %.0f m; %.0f, %.0f, %.0f m | 673 m; 161, 408, 846 m\n", p.meanHeight, p.quartiles[0],
                p.quartiles[1], p.quartiles[2]);
    std::printf("  rock at the top of the layering: sedimentary (carbonate)  | %.1f %% (%.1f %%) | 64 %% (about 21 %%)\n", 100 * p.sedimentary, 100 * p.carbonate);
    std::printf("    metamorphic | plutonic | volcanic                      | %.1f | %.1f | %.1f %% | 13 | 7 | 6 %% (water and ice 10 %%)\n", 100 * p.metamorphic,
                100 * p.plutonic, 100 * p.volcanic);
    std::printf("  dryland: rain under 65 %% of the air's demand              | %.1f %% | 41.3 %%\n", 100 * p.dryland);
    std::printf("  land draining to no ocean (steepest way on the 65 km grid)| %.1f %% | 13 %%\n", 100 * p.inland);
    std::printf("  shore cells in runs of 5 or more along a grid axis        | %.1f %% | fractal coast 18 %%\n", 100 * p.straightShore);

    const int wanted = int(arguments.number("tiles", 64));
    if (wanted <= 0) return 0;
    // First-pass tiles have even indices; a stride over them that yields about the wanted number.
    const int perSide = int(PlanetSpan / tileSpan(Level::Region)), stride = 2 * std::max(1, int(std::lround(0.5 * perSide / std::sqrt(double(wanted)))));
    std::vector<TileKey> keys;
    for (int z = 0; z < perSide; z += stride)
        for (int x = 0; x < perSide; x += stride) keys.push_back({Level::Region, x, z});
    for (const TileKey& key : keys) world.atlas.request(key);
    ReliefLikeness r;
    for (const TileKey& key : keys) addRelief(gatherMosaic(world, key, 1), r);
    finish(r);
    std::printf("from %zu first-pass region tiles (%zu km2 of land, %zu windows of 55 km), %.0f s:\n", keys.size(), r.land, r.windows, lap());
    std::printf("  land with more than 200 m of relief in a 5 km window      | %.1f %% | 12 %%\n", 100 * r.reliefOver200);
    std::printf("  Meybeck: plains | lowlands | platforms and hills | plateaus | mountains | %.0f | %.0f | %.0f | %.0f | %.0f %% | 25 | 14 | 23 | 13 | 25 %%\n", 100 * r.plains,
                100 * r.lowlands, 100 * r.hills, 100 * r.plateaus, 100 * r.mountains);
    std::printf("  roughness <5 | 5-10 | 10-20 | 20-40 | 40-80 | 80-160 | >160 m/km | %.0f | %.0f | %.0f | %.0f | %.0f | %.0f | %.0f %% | 25 | 19.5 | 17.8 | 16.8 | 14.7 | 5.8 | 0.4 %%\n",
                100 * r.roughness[0], 100 * r.roughness[1], 100 * r.roughness[2], 100 * r.roughness[3], 100 * r.roughness[4], 100 * r.roughness[5], 100 * r.roughness[6]);
    std::printf("  standing fresh water, share of the land's cells           | %.1f %% | not in the catalogue\n", 100 * r.lakes);
    return 0;
}
