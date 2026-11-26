#include "Measure.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Measure.hpp"
#include "aeolian/fn/Regime.hpp"
#include "climate/type/Climate.hpp"
#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "hydrology/constant/Water.hpp"
#include "landscape/fn/Forcing.hpp"
#include "landscape/fn/Process.hpp"
#include "planet/fn/Sample.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr double Degree = 57.29577951;

// Compass bearing sand travels towards: 0 north (-Z), 90 east.
double bearing(Vec2 drift) { return std::fmod(std::atan2(drift.x, -drift.z) * Degree + 360.0, 360.0); }

void printRegime(const WindRegime& regime) {
    std::printf("  wind: drift potential %.1f m2/yr, resultant %.1f m2/yr towards %.0f deg, steadiness %.2f | mean wind %.1f m/s, spread %.1f m/s\n", regime.potential,
                length(regime.resultant), bearing(regime.resultant), steadiness(regime), length(regime.mean), regime.spread);
    std::printf("  rose (share of the drift by bearing):");
    for (int k = 0; k < RoseSectors; ++k)
        if (regime.drift[size_t(k)] > 0.03 * regime.potential) std::printf("  %.0f deg %.0f %%", bearing(sectorHeading(k)), 100 * regime.drift[size_t(k)] / regime.potential);
    std::printf("\n");
}

// terra aeolian --find dry: dry inland country with wind, relief to shed sediment and a river to
// bring it, to pick places from.
int findDry(World& world, int count) {
    struct Place {
        double score, dryness, relief, river;
        Vec2 at;
    };
    std::vector<Place> places;
    const Planet& planet = world.planet;
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            float low = 1e9f, high = -1e9f, river = 0;
            bool inland = true;
            for (int oz = -2; oz <= 2; ++oz)
                for (int ox = -2; ox <= 2; ++ox) {
                    const int nx = wrapIndex(x + ox, PlanetCells), nz = wrapIndex(z + oz, PlanetCells);
                    low = std::min(low, planet.elevation.at(nx, nz)), high = std::max(high, planet.elevation.at(nx, nz));
                    inland = inland && planet.elevation.at(nx, nz) > planet.seaLevel + 20;
                    river = std::max(river, world.backdrop.discharge.at(nx, nz));
                }
            const double dryness = planet.precipitation.at(x, z) / evaporationDemand(planet.temperature.at(x, z));
            if (!inland || dryness > 0.25 || planet.temperature.at(x, z) < 5) continue;
            places.push_back({(high - low) * std::log10(std::max(10.0f, river)), dryness, high - low, river, worldPoint(planet.seed, planetCellCenter(x, z))});
        }
    std::sort(places.begin(), places.end(), [](const Place& a, const Place& b) { return a.score > b.score; });
    for (int k = 0; k < std::min(count, int(places.size())); ++k) {
        const Place& place = places[size_t(k)];
        const WindRegime regime = regimeAt(planet.seed, place.at, planet.time);
        std::printf("x %.0f z %.0f  rain over demand %.2f  relief %.0f m  river %.1e m3/yr  lat %.0f  drift potential %.1f m2/yr towards %.0f deg, steadiness %.2f\n", place.at.x, place.at.z,
                    place.dryness, place.relief, place.river, latitudeAt(planetPoint(planet.seed, place.at).z), regime.potential, bearing(regime.resultant), steadiness(regime));
    }
    return 0;
}

} // namespace

// terra aeolian --x m --z m [--level region|local|site] [--span tiles] [--scale n] [--exaggeration f] [--out directory] [--name tag]
// terra aeolian --find dry [--count n]
// What wind and evaporation left at a place: the regime, the dunes (heights, spacing, slip faces),
// the sand budget, loess, lag and salt, and where in the square each is thickest. Writes a shaded
// map coloured by what lies on top: sand yellow, loess buff, stone lag dark, salt white, water blue.
int aeolianCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    if (arguments.text("find", "") == "dry") return findDry(world, int(arguments.number("count", 24)));
    const std::string name = arguments.text("level", "site");
    const Level level = name == "region" ? Level::Region : name == "local" ? Level::Local : Level::Site;
    const Vec2 at{arguments.number("x", 0), arguments.number("z", 0)};
    const int span = std::max(1, int(arguments.number("span", 1))), side = span * TileCells;
    const double cell = specOf(level).cell;
    const TileKey centre = tileAt(level, at), first{level, centre.x - (span - 1) / 2, centre.z - (span - 1) / 2};
    Lap lap;
    std::vector<float> top(size_t(side) * size_t(side)), sand(top.size()), loess(top.size()), lag(top.size()), salt(top.size()), water(top.size());
    double cover[SedimentClasses] = {}, dryCells = 0;
    WindRecord middle;
    for (int tz = 0; tz < span; ++tz)
        for (int tx = 0; tx < span; ++tx) world.atlas.request({level, first.x + tx, first.z + tz});
    for (int tz = 0; tz < span; ++tz)
        for (int tx = 0; tx < span; ++tx) {
            const auto tile = world.atlas.require({level, first.x + tx, first.z + tz});
            if (tile->key == centre) middle = tile->aeolian;
            for (int z = 0; z < TileCells; ++z)
                for (int x = 0; x < TileCells; ++x) {
                    const size_t from = size_t(z) * TileCells + size_t(x), to = size_t(tz * TileCells + z) * size_t(side) + size_t(tx * TileCells + x);
                    top[to] = tile->now.rock[from] + tile->now.sediment[0][from] + tile->now.sediment[1][from] + tile->now.sediment[2][from];
                    water[to] = tile->water[from] - top[to];
                    if (water[to] <= 0) {
                        dryCells += 1;
                        for (int c = 0; c < SedimentClasses; ++c) cover[c] += tile->now.sediment[c][from];
                    }
                    if (tile->aeolian.labels.blown.empty()) continue;
                    sand[to] = tile->aeolian.labels.blown[from], loess[to] = tile->aeolian.labels.loess[from];
                    lag[to] = tile->aeolian.labels.lag[from], salt[to] = tile->aeolian.labels.salt[from];
                }
        }
    const double seconds = lap();
    const Vec2 origin{first.x * tileSpan(level), first.z * tileSpan(level)};
    std::printf("aeolian  seed %llu  %s level, %d x %d tiles of %.0f m cells from x %.0f z %.0f, built in %.1f s\n", static_cast<unsigned long long>(arguments.seed()), name.c_str(),
                span, span, cell, origin.x, origin.z, seconds);
    printRegime(middle.regime);
    if (middle.rounds > 0) std::printf("  dune field: %d rounds of the transport model, standing for %.0f yr of this wind\n", middle.rounds, middle.span);

    std::printf("  cover of the dry ground, mean m: coarse %.2f  sand %.2f  fines %.2f  (%.0f %% of the square is dry)\n", cover[Coarse] / std::max(1.0, dryCells),
                cover[Sand] / std::max(1.0, dryCells), cover[Fines] / std::max(1.0, dryCells), 100 * dryCells / double(top.size()));
    const DuneStats dunes = measureDunes(side, cell, top, sand, middle.regime.resultant, ReposeTan[Sand]);
    std::printf("  wind-laid sand: on %.1f %% of the ground, %.2f m as a mean over all of it, in %d bodies\n", 100 * dunes.sandShare, dunes.meanSand, dunes.patches);
    if (dunes.crests > 0 && level != Level::Region)
        std::printf("  dunes: height p50 %.1f m, tallest %.1f m | spacing p50 %.0f m (%d crests) | crests %.0f deg to the wind, order %.2f\n"
                    "         slip faces on %.1f %% of the sand, %.0f %% of them in the lee | windward %.1f deg, lee %.1f deg, steepest %.1f deg\n",
                    dunes.height, dunes.tallest, dunes.spacing, dunes.crests, dunes.crestAngle, dunes.order, 100 * dunes.slipShare, 100 * dunes.leeSlipShare,
                    std::atan(dunes.windward) * Degree, std::atan(dunes.lee) * Degree, std::atan(dunes.steepest) * Degree);
    const auto report = [&](const char* what, const std::vector<float>& field, float least) {
        double sum = 0, share = 0;
        size_t most = 0;
        for (size_t i = 0; i < field.size(); ++i) {
            if (water[i] > 0) continue;
            sum += field[i], share += field[i] >= least;
            if (field[i] > field[most] || water[most] > 0) most = i;
        }
        std::printf("  %s on dry ground: mean %.3f m over the square, at least %.2f m on %.1f %% of it, thickest %.2f m at x %.0f z %.0f\n", what, sum / double(field.size()), least, 100 * share / double(field.size()),
                    field[most], origin.x + (double(most % size_t(side)) + 0.5) * cell, origin.z + (double(most / size_t(side)) + 0.5) * cell);
    };
    report("sand", sand, 1.0f), report("loess", loess, 1.0f), report("stone lag", lag, 0.9f * LagArmour), report("salt", salt, 0.02f);
    const WindLedger& ledger = middle.ledger;
    const double area = double(DomainCells) * DomainCells * cell * cell;
    std::printf("  ledger of the middle tile, m over its domain: sand lifted %.3f laid %.3f drowned %.3f in %.3f out %.3f | dust raised %.3f settled %.3f drowned %.3f out %.3f | salt %.4f\n",
                ledger.sandLifted / area, ledger.sandLaid / area, ledger.sandDrowned / area, ledger.sandIn / area, ledger.sandOut / area, ledger.dustRaised / area,
                ledger.dustSettled / area, ledger.dustDrowned / area, ledger.dustOut / area, ledger.salt / area);

    Field<float> height(side, side);
    height.cells = top;
    const int scale = std::max(1, int(arguments.number("scale", 1)));
    std::vector<uint8_t> image(top.size() * 3 * size_t(scale) * size_t(scale));
    const double exaggeration = arguments.number("exaggeration", level == Level::Region ? 4.0 : 1.5);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const size_t i = size_t(z) * size_t(side) + size_t(x);
            const double shade = hillshade(height, x, z, cell, exaggeration);
            const double tint[3] = {water[i] > 0 ? 60.0 : salt[i] >= 0.02f ? 238 : sand[i] >= 0.3f ? 218 : loess[i] >= 0.5f ? 196 : lag[i] >= 0.9f * LagArmour ? 104 : 150,
                                    water[i] > 0 ? 105.0 : salt[i] >= 0.02f ? 236 : sand[i] >= 0.3f ? 186 : loess[i] >= 0.5f ? 184 : lag[i] >= 0.9f * LagArmour ? 92 : 140,
                                    water[i] > 0 ? 165.0 : salt[i] >= 0.02f ? 230 : sand[i] >= 0.3f ? 128 : loess[i] >= 0.5f ? 132 : lag[i] >= 0.9f * LagArmour ? 84 : 128};
            for (int sz = 0; sz < scale; ++sz)
                for (int sx = 0; sx < scale; ++sx)
                    putPixel(image, side * scale, x * scale + sx, z * scale + sz,
                             {uint8_t(std::clamp(tint[0] * shade, 0.0, 255.0)), uint8_t(std::clamp(tint[1] * shade, 0.0, 255.0)), uint8_t(std::clamp(tint[2] * shade, 0.0, 255.0))});
        }
    const std::string path = arguments.text("out", ".") + "/aeolian-" + arguments.text("name", name) + ".png";
    writePng(path, side * scale, side * scale, image);
    std::printf("%s\n", path.c_str());
    return 0;
}
