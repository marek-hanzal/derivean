#include "Relief.hpp"

#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Seek.hpp"

#include <cstdio>

using namespace derivean::sim;

void printRelief(const Mosaic& mosaic, const ReliefReport& report) {
    const SlopeStats& s = report.slope;
    std::printf("  slope deg: mean %.1f  p10 %.1f  p50 %.1f  p90 %.1f  p99 %.1f | above 30: %.1f %%  45: %.1f %%  60: %.1f %%  (%zu dry cells)\n", s.mean, s.p10, s.p50,
                s.p90, s.p99, 100 * s.above30, 100 * s.above45, 100 * s.above60, s.cells);
    for (const WindowRelief& relief : report.relief)
        if (relief.count) std::printf("  relief in %.0f km windows: p50 %.0f m  p90 %.0f m  max %.0f m  (%d windows)\n", relief.window / 1000, relief.p50, relief.p90, relief.high, relief.count);
    const CoverShares& c = report.cover;
    std::printf("  surface: bare rock %.1f %%  talus %.1f %%  gravel %.1f %%  fine cover %.1f %%  ice %.1f %%  water %.1f %% | mean cover %.2f m | talus lies at p50 %.1f p95 %.1f deg (%zu cells)\n",
                100 * c.share[size_t(Face::Rock)], 100 * c.share[size_t(Face::Talus)], 100 * c.share[size_t(Face::Gravel)], 100 * c.share[size_t(Face::Fines)],
                100 * c.share[size_t(Face::Ice)], 100 * c.share[size_t(Face::Water)], c.meanCover, c.talusAngle.p50, c.talusAngle.p95, c.talusAngle.count);
    std::printf("  drainage density: %.2f km/km2 of brooks (%.0e m3/yr)  %.2f km/km2 of streams (%.0e m3/yr)\n", report.drainage.brooks, BrookFlow, report.drainage.streams,
                riverThreshold(mosaic.level));
    const ValleyStats& v = report.valleys;
    std::printf("  main valleys (%d sections): floor width p50 %.0f p95 %.0f m | wall height p50 %.0f p95 %.0f m | wall angle p50 %.1f deg | depth/width p50 %.2f p95 %.2f, %.0f %% canyons\n",
                v.sections, v.floorWidth.p50, v.floorWidth.p95, v.wallHeight.p50, v.wallHeight.p95, v.wallAngle.p50, v.depthToWidth.p50, v.depthToWidth.p95, 100 * v.canyonShare);
    const CrestStats& k = report.crests;
    std::printf("  crests (%d): flank angle at the crest p50 %.1f deg | sharpness p25 %.2f p50 %.2f p75 %.2f (edge 0.75, dome 0.2) | curvature p50 %.4f 1/m\n", k.crests,
                k.flankAngle.p50, k.sharpness.p25, k.sharpness.p50, k.sharpness.p75, k.curvature.p50);
    std::printf("  cells above all eight neighbours %.2f %% (standing %.2f of their height above the lowest neighbour over the highest: crest knolls near 0, lone cones near 1)  below all eight %.2f %% | median departure from the mean of the eight, over their range: %.3f | rms against the 3 x 3 mean %.0f m, against 17 x 17 %.0f m\n",
                100 * k.peakShare, k.peakStanding, 100 * k.pitShare, k.roughness, k.rmsNear, k.rmsWide);
    const CliffStats& f = report.cliffs;
    std::printf("  faces above 60 deg: %d (%.2f per km2, %.2f %% of the ground) | height p50 %.0f p95 %.0f max %.0f m | length p50 %.0f max %.0f m\n", f.faces, f.perSquareKm,
                100 * f.steepShare, f.height.p50, f.height.p95, f.height.high, f.length.p50, f.length.high);
}

void writeSlopeMap(const Mosaic& mosaic, const std::string& path) {
    const int side = mosaic.side();
    const Field<float> slope = slopeField(mosaic);
    std::vector<uint8_t> image(mosaic.ground.size() * 3);
    constexpr Rgb Classes[6] = {{226, 226, 214}, {150, 190, 120}, {232, 214, 96}, {236, 150, 60}, {208, 52, 44}, {120, 40, 150}};
    constexpr double Limits[5] = {5, 15, 30, 45, 60};
    const double stream = riverThreshold(mosaic.level);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            Rgb color{70, 130, 190};
            if (mosaic.dry(x, z)) {
                const double angle = std::atan(std::max(0.0f, slope.at(x, z))) * DegreesPerRadian;
                int kind = 0;
                while (kind < 5 && angle > Limits[kind]) ++kind;
                const double shade = 0.55 + 0.45 * hillshade(mosaic.ground, x, z, mosaic.cell, 1.0);
                color = {uint8_t(std::min(255.0, Classes[kind].r * shade)), uint8_t(std::min(255.0, Classes[kind].g * shade)), uint8_t(std::min(255.0, Classes[kind].b * shade))};
                if (mosaic.discharge.at(x, z) >= stream) color = {20, 60, 150};
            }
            putPixel(image, side, x, z, color);
        }
    writePng(path, side, side, image);
}

// terra relief [--at humid-range|cold-range|old-range|dry-plateau|flat-beds|fold-belt|granite|lowland|coast | --x m --z m]
//              [--site-x m --site-z m] [--levels 123] [--tiles n] [--crop cells] [--out directory] [--name tag]
// How rugged a place is at each level. With --at the place is searched for and its coordinates are
// printed so that a later run can be given the same ground; --x/--z name the Local tile, --site-x/
// --site-z the Site tile (the steepest one inside that Local tile when left out).
int reliefCommand(const Arguments& arguments) {
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 local{arguments.number("x", 0), arguments.number("z", 0)};
    std::string name = arguments.text("name", "place");
    if (const std::string at = arguments.text("at", ""); !at.empty()) {
        int kind = 0;
        while (kind < int(Country::Count) && at != CountryNames[kind]) ++kind;
        if (kind == int(Country::Count)) {
            std::fprintf(stderr, "unknown kind of country: %s\n", at.c_str());
            return 2;
        }
        const Sought sought = seekCountry(world, Country(kind));
        if (!sought.found) {
            std::printf("%s: this planet has none\n", at.c_str());
            return 1;
        }
        local = sought.local;
        if (!arguments.values.count("name")) name = at;
        std::printf("%s: planet sample X %.0f Z %.0f (%s)\n", at.c_str(), sought.planet.x, sought.planet.z, sought.why.c_str());
    }
    const std::string levels = arguments.text("levels", "123");
    const bool sites = levels.find('3') != std::string::npos;
    const Vec2 site = !sites ? local : arguments.values.count("site-x") ? Vec2{arguments.number("site-x", 0), arguments.number("site-z", 0)} : steepestSite(world, local);
    std::printf("relief %s  seed %llu  age %.3g yr  --x %.0f --z %.0f --site-x %.0f --site-z %.0f  sea level %.0f m  (%.0f s)\n", name.c_str(),
                static_cast<unsigned long long>(arguments.seed()), world.options.age, local.x, local.z, site.x, site.z, world.planet.seaLevel, lap());
    const int tiles = int(arguments.number("tiles", 2));
    const std::string out = arguments.text("out", "");
    for (const char digit : levels) {
        if (digit < '1' || digit > '3') continue;
        const Level level = Level(digit - '0');
        const size_t before = world.atlas.finished();
        const Mosaic mosaic = gatherMosaic(world, level, level == Level::Site ? site : local, level == Level::Region ? 1 : tiles);
        double low = 1e30, high = -1e30;
        for (const float ground : mosaic.ground.cells) low = std::min(low, double(ground)), high = std::max(high, double(ground));
        std::printf("level %d  %.1f km across  %.0f m cells  ground %.0f .. %.0f m  (%zu tiles built in %.0f s)\n", int(level), mosaic.side() * mosaic.cell / 1000, mosaic.cell,
                    low, high, world.atlas.finished() - before, lap());
        printRelief(mosaic, reliefReport(mosaic));
        TileLedger ledger;
        for (int dz = 0; dz < mosaic.tiles; ++dz)
            for (int dx = 0; dx < mosaic.tiles; ++dx)
                if (const auto tile = world.atlas.find({level, mosaic.first.x + dx, mosaic.first.z + dz})) {
                    ledger.uplifted += tile->ledger.uplifted, ledger.eroded += tile->ledger.eroded, ledger.deposited += tile->ledger.deposited;
                    ledger.failed += tile->ledger.failed, ledger.dissolved += tile->ledger.dissolved, ledger.sedimentOut += tile->ledger.sedimentOut;
                }
        const double ground = double(mosaic.ground.size()) * mosaic.cell * mosaic.cell;
        std::printf("  ledger, m of ground over the tiles: uplifted %.0f  eroded %.0f  deposited %.0f  broken off slopes %.0f  dissolved %.0f  carried out %.0f\n", ledger.uplifted / ground,
                    ledger.eroded / ground, ledger.deposited / ground, ledger.failed / ground, ledger.dissolved / ground, ledger.sedimentOut / ground);
        if (!out.empty()) {
            const std::string path = out + "/slopes-" + name + "-L" + std::string(1, digit) + ".png";
            writeSlopeMap(mosaic, path);
            std::printf("  %s\n", path.c_str());
        }
        if (const int crop = std::min(int(arguments.number("crop", 0)), mosaic.side()); crop > 0 && !out.empty()) {
            // The cells around the place themselves, eight pixels each, shaded without exaggeration.
            constexpr int Zoom = 8;
            const Vec2 centre = level == Level::Site ? site : local;
            const int firstX = std::clamp(cellX(mosaic, centre) - crop / 2, 0, mosaic.side() - crop), firstZ = std::clamp(cellZ(mosaic, centre) - crop / 2, 0, mosaic.side() - crop);
            Field<float> field(crop * Zoom, crop * Zoom);
            for (int z = 0; z < crop * Zoom; ++z)
                for (int x = 0; x < crop * Zoom; ++x) field.at(x, z) = mosaic.ground.at(firstX + x / Zoom, firstZ + z / Zoom);
            const std::string path = out + "/crop-" + name + "-L" + std::string(1, digit) + ".png";
            writePng(path, crop * Zoom, crop * Zoom, reliefImage(field, mosaic.seaLevel, mosaic.cell, 1.0));
            std::printf("  %s\n", path.c_str());
        }
    }
    return 0;
}
