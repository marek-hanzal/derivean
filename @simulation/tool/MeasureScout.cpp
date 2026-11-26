#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "inspect/fn/Inspect.hpp"
#include "landscape/fn/Forcing.hpp"
#include "material/constant/Materials.hpp"
#include "measure/fn/Scout.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

void printPose(const char* name, const CameraPose& pose) {
    std::printf("%s X %.0f Y %.1f Z %.0f yaw %.1f pitch %.1f", name, pose.x, pose.y, pose.z, pose.yaw, pose.pitch);
}

// Relief around a site at the level it was verified on, the site under a red cross.
std::string savePicture(World& world, const ScoutSite& site, const std::string& out, const std::string& name) {
    const bool region = site.level == Level::Region;
    const Mosaic mosaic = gatherAround(world, site.level, site.at, region ? 60e3 : 4000);
    std::vector<uint8_t> image = reliefImage(mosaic.ground, mosaic.seaLevel, mosaic.cell, region ? 6.0 : 1.5, &mosaic.water);
    const int x = cellX(mosaic, site.at), z = cellZ(mosaic, site.at);
    for (int d = -10; d <= 10; ++d) {
        if (mosaic.contains(x + d, z)) putPixel(image, mosaic.side(), x + d, z, {235, 40, 40});
        if (mosaic.contains(x, z + d)) putPixel(image, mosaic.side(), x, z + d, {235, 40, 40});
    }
    const std::string path = out + "/scout-" + name + ".png";
    writePng(path, mosaic.side(), mosaic.side(), image);
    return path;
}

struct Listing {
    World& world;
    Vec2 start;
    std::string out;

    void print(const char* letter, const char* title, const std::vector<ScoutSite>& sites) const {
        std::printf("\n%s %s%s\n", letter, title, sites.empty() ? ": none found" : "");
        for (size_t i = 0; i < sites.size(); ++i) {
            const ScoutSite& site = sites[i];
            std::printf("%s.%zu X %.0f Z %.0f ground %.1f m (level %d, %.0f km from the start) | %s\n", letter, i + 1, site.at.x, site.at.z, double(site.ground),
                        int(site.level), length(site.at - start) / 1000, site.reason.c_str());
            printPose("    ground:", site.groundShot);
            printPose(" | air:", site.airShot);
            std::printf("\n    picture: %s\n", savePicture(world, site, out, std::string(letter) + std::to_string(i + 1)).c_str());
        }
    }
};

void printForms(const char* letter, const std::vector<RockForms>& all) {
    for (size_t i = 0; i < all.size(); ++i) {
        const RockForms& forms = all[i];
        std::printf("    %s.%zu block of 96 m at 0.5 m: %.2f %% of %d rock columns have air under rock, deepest gap %.1f m, beds %.1f m thick; %d separate forms", letter, i + 1,
                    100 * forms.roofedShare, forms.columns, forms.tallest, forms.bedThickness, forms.formations);
        if (forms.formations >= 2) std::printf(", %.1f m apart on average (scatter %.2f; near 0 means evenly spaced copies)", forms.spacing, forms.spacingCv);
        std::printf("\n");
    }
}

// Camera positions the owner compares from build to build.
constexpr Vec2 Watched[] = {{36, 43570}, {-289, 55305}, {-418, 61444}};

// What stands at a watched position. Where that is water, the nearest land with relief is looked
// for on the planet grid, read the way the landscape reads it.
void printWatched(World& world, Vec2 at) {
    const InspectRecord record = inspectAt(world, at);
    const bool wet = record.water.standing;
    std::printf("X %.0f Z %.0f: %s, ground %.1f m, slope %.1f deg, %s over %s", at.x, at.z,
                wet ? (record.water.level > record.water.seaLevel + 0.5 ? "lake" : "sea") : "land", double(record.surface.ground), double(record.surface.slope),
                definitionOf(record.cover.surface).name.data(), definitionOf(record.rock.bed).name.data());
    if (wet) {
        const Planet& planet = world.planet;
        const auto heightAt = [&](Vec2 where) { return samplePlanet(planet.elevation, planetPoint(planet.seed, where)); };
        Vec2 found = at;
        double best = 1e30, height = 0, relief = 0;
        const double step = PlanetCell / 2;
        for (int dz = -60; dz <= 60; ++dz)
            for (int dx = -60; dx <= 60; ++dx) {
                const Vec2 here = at + Vec2{dx * step, dz * step};
                const double distance = length(here - at), ground = heightAt(here);
                if (distance >= best || ground < planet.seaLevel + 50) continue;
                double low = ground, high = ground;
                for (int k = 0; k < 8; ++k) {
                    const double other = heightAt(here + Vec2{double(NeighborX[k]), double(NeighborZ[k])} * step);
                    low = std::min(low, other), high = std::max(high, other);
                }
                if (high - low < 300) continue;
                best = distance, found = here, height = ground, relief = high - low;
            }
        if (best < 1e30) std::printf("; nearest land with relief on the planet grid: X %.0f Z %.0f, %.0f km away, about %.0f m high, %.0f m of relief over 65 km", found.x,
                                     found.z, best / 1000, height, relief);
        else std::printf("; no land with relief within 2000 km");
    }
    std::printf("\n");
}

// Every type of docs/reference-landscapes.md and how this search answers for it: a window recipe of
// measure/fn/Catalogue.hpp ("window"), one of the lettered lists above, or the reason nothing was looked for.
struct TypeAnswer {
    const char* id;
    const char* name;
    const char* answer;
};

constexpr TypeAnswer Types[] = {
    {"A1", "collisional high range", "window"},
    {"A2", "glacial trough", "not searched as a trough: list c holds valleys between walls; floor width and flatness were not measured"},
    {"A3", "cirques, aretes, tarns", "nearest: lists e (lakes above 800 m with an outlet) and f (cold summits with glacial ice); enclosure of a basin by walls was not measured"},
    {"A4", "fault-block range front", "not searched: needs a straight slope break 30 km long; the planet does tilt fault blocks, so it may exist"},
    {"B1", "old worn mountains", "window"},
    {"B2", "granite domes", "window"},
    {"B3", "tors and boulder upland", "not searched: needs joint spacing and hilltop convexity at the ground level"},
    {"C1", "sandstone rock city", "list d"},
    {"C2", "table mountains, mesas, buttes", "not searched: needs isolated flat tops ringed by cliffs"},
    {"C3", "cuestas and hogbacks", "not searched: needs one resistant bed followed along strike for 10 km"},
    {"C4", "canyon lands and stepped plateaus", "window"},
    {"C5", "V-gorge with a waterfall", "list i (cuts of 100 m; the recipe asks 200 m and a step at the head, which was not measured)"},
    {"C6", "badlands", "window"},
    {"D1", "karst plateau", "no site: process absent. Limestone is only lowered evenly by solution; nothing sinks streams or opens dolines"},
    {"D2", "tower and cone karst", "no site: process absent, as D1"},
    {"D3", "poljes, karst gorges, shafts", "no site: process absent, as D1"},
    {"E1", "stratovolcano", "no site: process absent. Eruptions thicken the crust of a 65 km planet cell and lay a volcanic bed; nothing builds a cone"},
    {"E2", "shield volcano and lava field", "no site: process absent, as E1; no lava flows over the surface"},
    {"E3", "caldera", "no site: process absent, as E1"},
    {"E4", "columnar basalt, plugs, necks", "no site: process absent, as E1; basalt occurs as beds only"},
    {"E5", "geyser basins and spring terraces", "list g"},
    {"F1", "glaciated shield and lake country", "window"},
    {"F2", "fjords", "window"},
    {"F3", "till plains, drumlins, eskers", "no site: process absent. Ice erodes and leaves moraine, but nothing shapes till under the ice or lays meltwater ridges"},
    {"F4", "frost ground", "not searched: scree exists (the inspector's slope debris); block fields and patterned ground were not looked for"},
    {"G1", "braided gravel river", "no site: process absent. A river is one path of cells; it cannot split into braids"},
    {"G2", "lowland river: floodplain", "window"},
    {"G3", "delta, coastal lowland, tidal flats", "list a (lowland behind a shore; dividing channels and tidal flats were not measured)"},
    {"G4", "lake with a sandy beach", "no site: process absent. No waves sort sand along a shore"},
    {"H1", "cliffed coast", "window (steep ground at the sea only: no waves cut platforms, stacks or arches)"},
    {"H2", "sandy coast, spits, barriers, dunes", "no site: process absent. No waves or longshore drift move sand"},
    {"I1", "sand seas and dunes", "no site: process absent. Wind moves no sand"},
    {"I2", "stone desert and island mountains", "not searched: the search ran with rain fixed at one value, so no ground was dry"},
    {"I3", "range-front fans, salt flats", "not searched: as I2"},
    {"J1", "loess hills", "no site: process absent. Wind lays no silt"},
    {"J2", "open basins and footslopes", "window"},
    {"J3", "rolling upland and hill country", "window"},
};

void printCatalogue(const Listing& listing, const ScoutReport& report) {
    std::printf("\nl catalogue types of docs/reference-landscapes.md\n");
    for (const TypeAnswer& type : Types) {
        if (std::string(type.answer).rfind("window", 0) != 0) {
            std::printf("%s %s: %s\n", type.id, type.name, type.answer);
            continue;
        }
        const CatalogueSite* found = nullptr;
        for (const CatalogueSite& entry : report.catalogue)
            if (std::string(CatalogueTypes[entry.type].id) == type.id) found = &entry;
        if (!found) {
            std::printf("%s %s: no site on the %zu dry local cells searched; the recipe's conditions never met in one 5 km window%s\n", type.id, type.name, report.localLand,
                        type.answer + 6);
            continue;
        }
        const ScoutSite& site = found->site;
        std::printf("%s %s: X %.0f Z %.0f ground %.0f m (level %d)%s | %s\n", type.id, type.name, site.at.x, site.at.z, double(site.ground), int(site.level), type.answer + 6,
                    site.reason.c_str());
        printPose("    ground:", site.groundShot);
        printPose(" | air:", site.airShot);
        std::printf("\n    picture: %s\n", savePicture(listing.world, site, listing.out, std::string("type-") + type.id).c_str());
    }
}

} // namespace

// terra scout [--x --z] [--radius m] [--keep n] [--out directory]: showcase sites around a start
// point, coarse to fine. Without a position the search starts from the default landing.
int scoutCommand(const Arguments& arguments) {
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 start{arguments.number("x", 0), arguments.number("z", 0)};
    if (!arguments.values.count("x") && !arguments.values.count("z")) start = findLanding(world, start);
    const double radius = arguments.number("radius", 250e3);
    const ScoutReport report = scoutAround(world, start, radius, int(arguments.number("keep", 3)));
    const char* rain = std::getenv("DERIVEAN_RAIN");
    std::printf("scout  seed %llu  age %.3g yr  start X %.0f Z %.0f  radius %.0f km  sea level %.1f m  rain %s  %.0f s\n",
                static_cast<unsigned long long>(arguments.seed()), world.options.age, start.x, start.z, radius / 1000, world.planet.seaLevel,
                rain ? rain : "from the climate", lap());
    std::printf("tiles built: %zu region, %zu local, %zu site and ground | cameras: yaw 0 looks north (-Z), positive turns west; pitch positive up; Y is metres\n",
                report.regionTiles, report.localTiles, report.siteTiles);

    const Listing listing{world, start, arguments.text("out", ".")};
    listing.print("a", "coast with lowland: a shore with land rising under 3 % for 5 km", report.coast);
    listing.print("b", "local range with a saddle: summits 300 m above a pass within 10 km", report.saddle);
    listing.print("c", "river valley: a trunk stream between walls of 150 m", report.valley);
    listing.print("d", "rock-city ground: sandstone 60 m thick within 100 m of the surface, beds under 3 deg, 80 to 300 m of relief per km", report.city);
    printForms("d", report.cityForms);
    listing.print("e", "mountain water basin: a lake of 0.05 km2 or more above 800 m that spills into a stream", report.basin);
    listing.print("f", "granite or gneiss summits below freezing, 600 m over their valleys", report.summit);
    listing.print("g", "hot ground: more than 150 K/km from below, on land with rain", report.hot);
    listing.print("h", "steep bare sandstone or conglomerate, sampled for rock forms", report.sandstone);
    printForms("h", report.forms);
    std::printf("    steep bare sandstone or conglomerate makes up %.3f %% of the %zu dry local cells looked at\n",
                report.localLand ? 100.0 * double(report.localSandstone) / double(report.localLand) : 0.0, report.localLand);
    listing.print("i", "gorge: a cut of 100 m with level ground within 1 km", report.gorge);
    listing.print("j", "ordinary country: 10 km with under 150 m of relief, no water, no cliff", report.plain);

    std::printf("\nk watched positions\n");
    for (const Vec2 at : Watched) printWatched(world, at);
    printCatalogue(listing, report);
    std::printf("total %.0f s, %zu tiles\n", lap(), world.atlas.finished());
    return 0;
}
