#include "Relief.hpp"

#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Scene.hpp"
#include "diagnostics/fn/View.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Block.hpp"
#include "measure/fn/Orientation.hpp"
#include "measure/fn/Plane.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

Material materialNamed(const std::string& name, Material fallback) {
    for (int i = 0; i < MaterialCount; ++i) {
        const std::string_view known = definitionOf(Material(i)).name;
        if (known.size() == name.size() && std::equal(known.begin(), known.end(), name.begin(), [](char a, char b) { return std::tolower(a) == std::tolower(b); })) return Material(i);
    }
    return fallback;
}

// The ground along a line across the river through the middle of the block, every `stride` cells.
void printProfile(const Domain& domain, int stride) {
    const int x = domain.size / 2;
    std::printf("  profile across the river (m from it: ground / loose cover):");
    for (int z = domain.size / 2; z < Halo + TileCells; z += stride) {
        const size_t i = domain.index(x, z);
        std::printf(" %.0f: %.1f/%.1f", (z - domain.size / 2) * domain.cell, domain.ground.rock[i] + domain.sedimentAt(i), domain.sedimentAt(i));
    }
    std::printf("\n  along the river (m from its head: ground / loose cover / m3 per yr):");
    for (int along = Halo; along < Halo + TileCells; along += 32) {
        // The wettest cell of the line across the valley is where the river runs.
        size_t wettest = domain.index(along, domain.size / 2);
        for (int z = Halo; z < Halo + TileCells; ++z)
            if (domain.discharge[domain.index(along, z)] > domain.discharge[wettest]) wettest = domain.index(along, z);
        std::printf(" %.0f: %.0f/%.1f/%.2g", along * domain.cell, domain.ground.rock[wettest] + domain.sedimentAt(wettest), domain.sedimentAt(wettest), domain.discharge[wettest]);
    }
    std::printf("\n");
}

// A uniform plane tilted along a heading that is no direction of the grid, raised steadily against
// its foot: the valleys it grows have nothing to follow but the tilt. Where they run says whether
// the solver has directions of its own.
int tiltedPlane(const Arguments& arguments, Level level, Material rock) {
    PlaneSpec plane;
    plane.level = level;
    plane.rock = rock;
    plane.azimuth = arguments.number("tilt", 27) / DegreesPerRadian;
    plane.slope = arguments.number("slope", 0.03);
    plane.rain = arguments.number("rain", 1.0);
    plane.steps = int(arguments.number("steps", 0));
    BlockSpec spec;
    spec.level = level, spec.rock = rock, spec.led = false;
    spec.uplift = arguments.number("uplift", 3e-4);
    Lap lap;
    Domain domain = planeDomain(plane);
    FlowScratch scratch;
    const int steps = int(std::llround((domain.endTime - domain.startTime) / domain.step));
    // stepBlock keeps the rim cells beside the middle row at rest; here the whole rim is the base level.
    for (int step = 0; step < steps; ++step) {
        const std::vector<float> rim = domain.ground.rock, datum = domain.ground.datum;
        stepBlock(domain, scratch, spec);
        for (size_t i = 0; i < domain.cells(); ++i)
            if (domain.hold[i] == Rim && !domain.outlet[i]) domain.ground.rock[i] = rim[i], domain.ground.datum[i] = datum[i];
    }
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    const Mosaic mosaic = mosaicOf(domain);
    const double river = arguments.number("river", channelHeadOf(level));
    const int chord = int(arguments.number("chord", 16));
    const Rose rose = roseOf(valleyAzimuths(mosaic, river, chord));
    int peak = 0;
    for (int bin = 1; bin < RoseBins; ++bin)
        if (rose.share[size_t(bin)] > rose.share[size_t(peak)]) peak = bin;
    const double tilt = plane.azimuth * DegreesPerRadian, width = 180.0 / RoseBins;
    double near = 0;
    for (int bin = 0; bin < RoseBins; ++bin) {
        const double off = std::abs((bin + 0.5) * width - tilt);
        if (std::min(off, 180 - off) <= 10) near += rose.share[size_t(bin)];
    }
    std::printf("tilted plane  level %d  %s  tilt %.1f deg  uplift %.2g m/yr  %d steps  (%.1f s)\n", int(level), definitionOf(rock).name.data(), tilt, spec.uplift, steps, lap());
    std::printf("  valley segments of %d cells carrying %.2g m3/yr or more: %d | most frequent heading %.1f deg (%.0f %% of them) | within 10 deg of the tilt %.0f %% | grid excess %.2f (axes %.2f, diagonals %.2f; 1 is isotropic, above %.1f follows the grid)\n",
                chord, river, rose.segments, (peak + 0.5) * width, 100 * rose.share[size_t(peak)], 100 * near, rose.gridExcess, rose.axisExcess, rose.diagonalExcess, GridExcessLimit);
    std::printf("  headings in 5 deg bins from 0:");
    for (int bin = 0; bin < RoseBins; ++bin) std::printf(" %.0f", 100 * rose.share[size_t(bin)]);
    std::printf("\n");
    printRelief(mosaic, reliefReport(mosaic));
    if (const std::string out = arguments.text("out", ""); !out.empty()) {
        const std::string path = out + "/slopes-" + arguments.text("name", "tilt") + ".png";
        writeSlopeMap(mosaic, path);
        std::printf("  %s\n", path.c_str());
    }
    return 0;
}

} // namespace

// terra block [--level 2|3] [--rock name] [--weak name] [--bed m] [--dip rise/run] [--height m] [--rain m/yr] [--flow m3/yr]
//             [--uplift m/yr] [--led 1] [--steps n] [--out directory] [--name tag]
// terra block --tilt degrees [--level n] [--rock name] [--slope rise/run] [--uplift m/yr] [--river m3/yr] [--chord cells]
// A plateau of one rock, or of two in alternating beds, with a river held at its foot, replayed with
// the level's own processes: the controlled ground on which a change to them is judged.
int blockCommand(const Arguments& arguments) {
    BlockSpec spec;
    spec.level = Level(int(arguments.number("level", 3)));
    spec.rock = materialNamed(arguments.text("rock", "granite"), Material::Granite);
    if (arguments.values.count("tilt")) return tiltedPlane(arguments, spec.level, spec.rock);
    spec.weak = materialNamed(arguments.text("weak", "air"), Material::Air);
    spec.bedThickness = arguments.number("bed", spec.bedThickness);
    spec.dip = arguments.number("dip", 0);
    spec.sound = arguments.number("sound", 1);
    spec.height = arguments.number("height", spec.height);
    spec.rain = arguments.number("rain", spec.rain);
    spec.riverFlow = arguments.number("flow", spec.riverFlow);
    spec.uplift = arguments.number("uplift", 0);
    spec.led = arguments.number("led", 0) > 0;
    spec.river = arguments.number("river", 1) > 0;
    spec.floorHalf = int(arguments.number("floor", spec.floorHalf));
    spec.steps = int(arguments.number("steps", 0));
    Lap lap;
    const Domain domain = replayBlock(spec);
    const Mosaic mosaic = mosaicOf(domain);
    std::printf("block  level %d  %s%s%s  beds %.0f m  dip %.2f  height %.0f m  %d steps of %.0f yr  (%.1f s)\n", int(spec.level), definitionOf(spec.rock).name.data(),
                spec.weak == Material::Air ? "" : " over ", spec.weak == Material::Air ? "" : definitionOf(spec.weak).name.data(), spec.bedThickness, spec.dip, spec.height,
                domain.taken, domain.step, lap());
    printRelief(mosaic, reliefReport(mosaic));
    std::printf("  tallest unbroken face across the river: above 60 deg %.0f m  above 45 deg %.0f m  above 30 deg %.0f m\n", tallestFace(domain, std::tan(60 / DegreesPerRadian)),
                tallestFace(domain, 1.0), tallestFace(domain, std::tan(30 / DegreesPerRadian)));
    printProfile(domain, int(arguments.number("stride", 4)));
    std::printf("  ledger m3: failed %.4g  weathered %.4g  eroded %.4g  deposited %.4g  left with the river %.4g\n", domain.ledger.failed, domain.ledger.weathered,
                domain.ledger.eroded, domain.ledger.deposited, domain.ledger.sedimentOut);
    const std::string out = arguments.text("out", "");
    if (out.empty()) return 0;
    const std::string name = arguments.text("name", "block");
    writeSlopeMap(mosaic, out + "/slopes-" + name + ".png");

    Scene scene;
    scene.seaLevel = domain.seaLevel;
    scene.layers.push_back(layerOf(domain));
    finishScene(scene);
    // From above the plateau on one side, looking across the valley and down it.
    const double span = domain.size * domain.cell;
    Camera camera;
    camera.eye = {domain.origin.x + 0.2 * span, 0, domain.origin.z + 0.28 * span};
    camera.eye.y = sceneTop(scene, camera.eye.x, camera.eye.z) + arguments.number("above", 0.5 * spec.height);
    const Vec2 target{domain.origin.x + 0.6 * span, domain.origin.z + 0.56 * span};
    camera.heading = std::atan2(target.x - camera.eye.x, -(target.z - camera.eye.z));
    camera.pitch = std::atan2(sceneTop(scene, target.x, target.z) - camera.eye.y, length(target - Vec2{camera.eye.x, camera.eye.z}));
    camera.fov = 70 / DegreesPerRadian;
    Light light;
    light.azimuth = arguments.number("sun-azimuth", 250) / DegreesPerRadian, light.elevation = arguments.number("sun-elevation", 38) / DegreesPerRadian;
    Workers workers;
    writePng(out + "/view-" + name + ".png", camera.width, camera.height, renderView(scene, camera, light, workers));
    std::printf("  %s/slopes-%s.png  %s/view-%s.png\n", out.c_str(), name.c_str(), out.c_str(), name.c_str());
    return 0;
}
