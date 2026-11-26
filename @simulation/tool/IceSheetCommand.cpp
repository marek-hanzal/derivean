#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Census.hpp"
#include "icesheet/fn/Dating.hpp"
#include "icesheet/service/Glaciations.hpp"
#include "icesheet/type/Sheet.hpp"

#include <algorithm>
#include <cstdio>

using namespace derivean::sim;

namespace {

void printPlanet(World& world, const ContinentalIce& ice, const std::string& out) {
    const Planet& planet = world.planet;
    double sea = 0;
    for (const float height : planet.elevation.cells) sea += height <= planet.seaLevel;
    std::printf("stages of the planet's ice (present %.4g yr; the last maximum was %.0f yr before it):\n", ice.present, sinceMaximum(planet.seed, ice.present));
    for (size_t k = 0; k < ice.stages.size(); ++k) {
        const PlanetStage& stage = ice.stages[k];
        float thickest = 0;
        for (const float thickness : stage.thickness.cells) thickest = std::max(thickest, thickness);
        std::printf("  %zu %-12s cooling %+5.2f C  land under ice %5.1f %%  volume %.3g km3 (%.0f m of sea level)  thickest %.0f m\n", k,
                    k == IceNow ? "present" : k == IceLastMaximum ? "last maximum" : "older maxima", stage.cooling, 100 * stage.landShare, stage.volume / 1e9,
                    stage.volume * IceToWater / (sea * PlanetCellArea), thickest);
    }
    // Present ice white, ground the last maximum also covered pale blue, ground only older maxima reached grey-blue.
    std::vector<uint8_t> image = reliefImage(planet.elevation, planet.seaLevel, PlanetCell, 12.0);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            if (ice.stages[IceNow].thickness.at(x, z) > 0) putPixel(image, PlanetCells, x, z, {245, 247, 250});
            else if (ice.stages[IceLastMaximum].thickness.at(x, z) > 0) putPixel(image, PlanetCells, x, z, {150, 205, 245});
            else if (ice.stages.back().thickness.at(x, z) > 0) putPixel(image, PlanetCells, x, z, {120, 140, 190});
        }
    constexpr int Scale = 3;
    std::vector<uint8_t> big(size_t(PlanetCells) * Scale * size_t(PlanetCells) * Scale * 3);
    for (int z = 0; z < PlanetCells * Scale; ++z)
        for (int x = 0; x < PlanetCells * Scale; ++x)
            for (int c = 0; c < 3; ++c)
                big[(size_t(z) * size_t(PlanetCells * Scale) + size_t(x)) * 3 + size_t(c)] = image[(size_t(z / Scale) * PlanetCells + size_t(x / Scale)) * 3 + size_t(c)];
    const std::string path = out + "/icesheet-planet.png";
    writePng(path, PlanetCells * Scale, PlanetCells * Scale, big);
    std::printf("  %s\n", path.c_str());
    for (const PlaceKind kind : {PlaceKind::Inland, PlaceKind::Coastal, PlaceKind::Margin}) {
        std::printf("ground freed since the last maximum, %s:\n", kind == PlaceKind::Coastal ? "with open sea beside it" : kind == PlaceKind::Margin ? "under thin ice near its margin" : "inland");
        for (const IcePlace& place : freedPlaces(planet, ice, kind, 10))
            std::printf("  --x %.0f --z %.0f   %.0f m above the sea, %.0f m of ice at the maximum (region tile of pass %d)\n", place.at.x, place.at.z, place.height, place.thickness,
                        passOf(tileAt(Level::Region, place.at)));
    }
}

void printCensus(const SheetCensus& c, double cell) {
    const double km2 = cell * cell / 1e6;
    std::printf("  ice now on %.1f %% (thickest %.0f m), at the last maximum on %.1f %% (thickest %.0f m)\n", 100 * c.iceNow, c.thickestNow, 100 * c.iceThen, c.thickestThen);
    std::printf("  ground the last sheet has left: %.1f %% of the square (%.0f km2); of it %.1f %% lakes, %.1f %% under the sea\n", 100 * c.freed, c.freed * double(c.cells) * km2,
                100 * c.freedLakes, 100 * c.freedSea);
    std::printf("  lowest ground under the last sheet %.0f m against the sea; rock ground off: mean %.0f m, most %.0f m; sliding at the maximum: mean %.1f, fastest %.0f m/yr\n", c.deepestBed,
                c.wornMean, c.wornMost, c.slide, c.fastest);
    std::printf("  left by the last sheet, share of the square and mean depth: till %.1f %% %.1f m | end moraine %.2f %% %.1f m | outwash %.2f %% %.1f m | esker fill %.2f %% %.1f m\n", 100 * c.till,
                c.tillDepth, 100 * c.moraine, c.moraineDepth, 100 * c.outwash, c.outwashDepth, 100 * c.esker, c.eskerDepth);
}

// Cells standing higher above all eight neighbours than the tallest esker: loose ones (their cover
// alone stands that high) are what no pass may leave; rock ones come from the settled replay.
void printColumns(const Mosaic& mosaic) {
    int loose = 0, rock = 0;
    float highest = 0;
    for (int z = 1; z + 1 < mosaic.side(); ++z)
        for (int x = 1; x + 1 < mosaic.side(); ++x) {
            float around = -1e30f;
            for (int k = 0; k < 8; ++k) around = std::max(around, mosaic.ground.at(x + NeighborX[k], z + NeighborZ[k]));
            const float above = mosaic.ground.at(x, z) - around;
            if (above <= EskerTallest) continue;
            (mosaic.cover.at(x, z) > above - EskerTallest ? loose : rock) += 1;
            highest = std::max(highest, above);
        }
    std::printf("  columns more than %.0f m above all neighbours: %d of loose cover, %d of rock; highest %.0f m\n", double(EskerTallest), loose, rock, highest);
}

// Relief with the ice of the last maximum tinted blue and the present ice white, and beside it what
// the last sheet left: till brown, end moraine red, outwash yellow, esker fill green, lakes blue.
void writeMaps(const Mosaic& mosaic, World& world, const std::string& out, const std::string& tag) {
    const int side = mosaic.side();
    std::vector<uint8_t> ice = reliefImage(mosaic.ground, mosaic.seaLevel, mosaic.cell, mosaic.level == Level::Region ? 6.0 : 1.5, &mosaic.water);
    std::vector<uint8_t> left = ice;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const size_t at = (size_t(z) * size_t(side) + size_t(x)) * 3;
            if (mosaic.ice.at(x, z) >= LeastSheet) ice[at] = ice[at + 1] = ice[at + 2] = uint8_t(215 + 35 * std::min(1.0f, mosaic.ice.at(x, z) / 2500.0f));
            else if (mosaic.iceAge.at(x, z) >= LeastSheet)
                ice[at] = uint8_t(ice[at] * 0.55), ice[at + 1] = uint8_t(ice[at + 1] * 0.75), ice[at + 2] = uint8_t(std::min(255.0, ice[at + 2] * 0.7 + 90));
            const auto tile = world.atlas.find({mosaic.level, mosaic.first.x + x / TileCells, mosaic.first.z + z / TileCells});
            if (!tile || !tile->sheet || mosaic.flooded(x, z) || mosaic.ice.at(x, z) >= LeastSheet) continue;
            const size_t i = size_t(z % TileCells) * TileCells + size_t(x % TileCells);
            const SheetRecord& record = *tile->sheet;
            const auto tint = [&](Rgb colour, float depth, float full) {
                const float share = std::min(0.85f, 0.35f + 0.5f * depth / full);
                left[at] = uint8_t(left[at] * (1 - share) + colour.r * share), left[at + 1] = uint8_t(left[at + 1] * (1 - share) + colour.g * share);
                left[at + 2] = uint8_t(left[at + 2] * (1 - share) + colour.b * share);
            };
            if (record.till[i] >= 0.5f) tint({150, 110, 70}, record.till[i], 20);
            if (record.outwash[i] >= 0.5f) tint({235, 215, 90}, record.outwash[i], 20);
            if (record.moraine[i] >= 0.5f) tint({215, 60, 50}, record.moraine[i], 40);
            if (record.esker[i] >= 0.5f) tint({60, 170, 80}, record.esker[i], 5);
        }
    for (const auto& [name, image] : {std::pair{"ice", &ice}, std::pair{"left", &left}}) {
        const std::string path = out + "/icesheet-" + name + "-" + tag + ".png";
        writePng(path, side, side, *image);
        std::printf("  %s\n", path.c_str());
    }
}

} // namespace

// terra icesheet [--seed n] [--age years] [--out directory] [--x m --z m [--level 1|2] [--tiles n]]
// Without a place: the stages of the planet's ice, how much of the land each covers, a map, and
// places freed since the last maximum to look at. With one: what the tiles around it hold of the
// ice and of what it left, and two maps.
int iceSheetCommand(const Arguments& arguments) {
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const double planetSeconds = lap();
    Setting setting = world.atlas.setting();
    const auto ice = continentalIce(setting);
    std::printf("icesheet  seed %llu  age %.3g yr  sea level %.0f m  planet %.1f s  its ice %.1f s\n", static_cast<unsigned long long>(arguments.seed()), world.options.age,
                world.planet.seaLevel, planetSeconds, lap());
    const std::string out = arguments.text("out", ".");
    if (!arguments.values.count("x")) {
        printPlanet(world, *ice, out);
        return 0;
    }
    const Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    const Level level = Level(int(arguments.number("level", 1)));
    const int tiles = int(arguments.number("tiles", 1));
    const Mosaic mosaic = gatherMosaic(world, level, focus, tiles);
    std::printf("level %d  %d x %d tiles around X %.0f Z %.0f  %.1f km across  built %zu tiles in %.1f s\n", int(level), tiles, tiles, focus.x, focus.z, mosaic.side() * mosaic.cell / 1000,
                world.atlas.finished(), lap());
    std::vector<std::shared_ptr<const Tile>> held;
    std::vector<const Tile*> read;
    for (int dz = 0; dz < tiles; ++dz)
        for (int dx = 0; dx < tiles; ++dx) held.push_back(world.atlas.find({level, mosaic.first.x + dx, mosaic.first.z + dz})), read.push_back(held.back().get());
    printCensus(censusOf(read), mosaic.cell);
    printColumns(mosaic);
    writeMaps(mosaic, world, out, "L" + std::to_string(int(level)));
    return 0;
}
