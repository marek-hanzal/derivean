#include "Ground.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Reader.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr Rgb Sky{188, 214, 236}, Lake{58, 120, 176};

Rgb colorOf(Material material, double shade = 1.0) {
    if (material == Material::Air) return Sky;
    const uint32_t color = definitionOf(material).color;
    return {uint8_t(std::min(255.0, ((color >> 16) & 255) * shade)), uint8_t(std::min(255.0, ((color >> 8) & 255) * shade)),
            uint8_t(std::min(255.0, (color & 255) * shade))};
}

Rgb blend(Rgb a, Rgb b, double t) { return {uint8_t(a.r + (b.r - a.r) * t), uint8_t(a.g + (b.g - a.g) * t), uint8_t(a.b + (b.b - a.b) * t)}; }

// The heights with their best plane taken out, so the shade shows the ground's own relief and not
// which way the whole slope faces.
Field<float> detrended(const Field<float>& stepped) {
    // Voxel tops come in steps of one voxel; averaged over a few they shade as the slope they approximate.
    Field<float> top = stepped;
    for (int pass = 0; pass < 2; ++pass) {
        const Field<float> from = top;
        for (int z = 1; z < top.height - 1; ++z)
            for (int x = 1; x < top.width - 1; ++x) {
                float sum = 0;
                for (int dz = -1; dz <= 1; ++dz)
                    for (int dx = -1; dx <= 1; ++dx) sum += from.at(x + dx, z + dz);
                top.at(x, z) = sum / 9;
            }
    }
    double sx = 0, sz = 0, sh = 0, sxx = 0, szz = 0, sxh = 0, szh = 0;
    const double cx = 0.5 * (top.width - 1), cz = 0.5 * (top.height - 1), n = double(top.size());
    for (int z = 0; z < top.height; ++z)
        for (int x = 0; x < top.width; ++x) {
            const double dx = x - cx, dz = z - cz, h = top.at(x, z);
            sx += dx, sz += dz, sh += h, sxx += dx * dx, szz += dz * dz, sxh += dx * h, szh += dz * h;
        }
    const double slopeX = sxh / std::max(1e-9, sxx), slopeZ = szh / std::max(1e-9, szz), mean = sh / n;
    Field<float> flat(top.width, top.height);
    for (int z = 0; z < top.height; ++z)
        for (int x = 0; x < top.width; ++x) flat.at(x, z) = float(top.at(x, z) - mean - slopeX * (x - cx) - slopeZ * (z - cz));
    return flat;
}

// Writes an image `scale` times enlarged, pixel for pixel.
void writeScaled(const std::string& path, int width, int height, const std::vector<uint8_t>& rgb, int scale) {
    std::vector<uint8_t> large(size_t(width) * scale * size_t(height) * scale * 3);
    for (int y = 0; y < height * scale; ++y)
        for (int x = 0; x < width * scale; ++x)
            for (int c = 0; c < 3; ++c) large[(size_t(y) * size_t(width * scale) + size_t(x)) * 3 + c] = rgb[(size_t(y / scale) * size_t(width) + size_t(x / scale)) * 3 + c];
    writePng(path, width * scale, height * scale, large);
}

} // namespace

void renderSlice(World& world, const Arguments& arguments, double x, double z, const std::string& name) {
    const double span = arguments.number("span", 96), step = arguments.number("step", 0.5);
    const std::string out = arguments.text("out", ".") + "/" + name;
    const int side = int(span / step), scale = std::max(1, int(arguments.number("scale", 1)));
    const SurfaceSample here = surveyAt(world, {x, z}, Level::Ground);
    const double tall = arguments.number("tall", 80);
    const int rows = int(tall / step);
    const double bottom = arguments.number("bottom", std::floor(here.ground - 0.6 * tall));

    const Block volume{x - span / 2, bottom, z - span / 2, step, side, rows, side};
    std::vector<Material> cells(size_t(side) * size_t(side) * size_t(rows));
    sampleBlock(world, volume, cells.data());
    const auto cell = [&](int ix, int iy, int iz) { return cells[(size_t(iy) * size_t(side) + size_t(iz)) * size_t(side) + size_t(ix)]; };
    Field<float> top(side, side, float(bottom)), water(side, side, -1e9f);
    Field<uint8_t> skin(side, side, uint8_t(Material::Air));
    int roofed = 0, wet = 0;
    std::vector<int> share(size_t(MaterialCount), 0);
    for (int iz = 0; iz < side; ++iz)
        for (int ix = 0; ix < side; ++ix) {
            bool air = false, roof = false;
            for (int iy = rows - 1; iy >= 0; --iy) {
                const Material material = cell(ix, iy, iz);
                if (material != Material::Air && skin.at(ix, iz) == uint8_t(Material::Air)) {
                    top.at(ix, iz) = float(bottom + (iy + 1) * step);
                    skin.at(ix, iz) = uint8_t(material);
                } else if (material == Material::Air && skin.at(ix, iz) != uint8_t(Material::Air)) {
                    air = true;
                } else if (material != Material::Air && air) {
                    roof = true;
                }
            }
            roofed += roof;
            ++share[skin.at(ix, iz)];
            const SurfaceSample surface = surfaceAt(world, {volume.originX + (ix + 0.5) * step, volume.originZ + (iz + 0.5) * step}, Level::Ground);
            if (surface.water > surface.ground + 0.01f) water.at(ix, iz) = surface.water, ++wet;
        }
    const Field<float> relief = detrended(top);
    std::vector<uint8_t> plan(size_t(side) * size_t(side) * 3);
    for (int iz = 0; iz < side; ++iz)
        for (int ix = 0; ix < side; ++ix) {
            const Rgb ground = colorOf(Material(skin.at(ix, iz)), hillshade(relief, ix, iz, step, 1.0));
            putPixel(plan, side, ix, iz, water.at(ix, iz) > top.at(ix, iz) - float(step) ? blend(ground, Lake, 0.6) : ground);
        }
    writeScaled(out + "-plan.png", side, side, plan, scale);

    // Sections through the middle, sky on top: west to east, then north to south.
    for (const bool along : {true, false}) {
        std::vector<uint8_t> section(size_t(side) * size_t(rows) * 3);
        const int middle = side / 2;
        for (int iy = 0; iy < rows; ++iy)
            for (int k = 0; k < side; ++k) {
                const int ix = along ? k : middle, iz = along ? middle : k;
                const Material material = cell(ix, iy, iz);
                const bool under = material == Material::Air && bottom + (iy + 0.5) * step < water.at(ix, iz);
                putPixel(section, side, k, rows - 1 - iy, under ? Lake : colorOf(material));
            }
        writeScaled(out + (along ? "-section.png" : "-section-ns.png"), side, rows, section, scale);
    }
    std::printf("%s  X %.0f Z %.0f  ground %.2f m  bedrock %s  columns with air under rock: %d of %d (%.2f %%)  under water: %.1f %%\n", name.c_str(), x, z, here.ground,
                std::string(definitionOf(here.bedrock).name).c_str(), roofed, side * side, 100.0 * roofed / double(side * side), 100.0 * wet / double(side * side));
    std::printf("  legend (share of the plan):");
    for (int m = 0; m < MaterialCount; ++m)
        if (share[size_t(m)] > 0)
            std::printf("  %s #%06x %.1f %%", std::string(definitionOf(Material(m)).name).c_str(), definitionOf(Material(m)).color, 100.0 * share[size_t(m)] / double(side * side));
    std::printf("\n  %s-plan.png  %s-section.png  %s-section-ns.png\n", out.c_str(), out.c_str(), out.c_str());
}

// terra slice --x --z [--span m] [--step m] [--tall m] [--bottom m] [--scale n] [--out dir]: the
// authoritative material field straight from sampleBlock, as a plan of the highest solid (shaded by
// the ground's own relief, water over it) and as vertical sections through the point from west to
// east and from north to south, with a legend of what the plan shows. `--places "x,z;x,z"` renders
// several places of one world as slice0, slice1 and so on.
int sliceCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    std::vector<Vec2> places;
    const std::string list = arguments.text("places", "");
    for (size_t at = 0; at < list.size();) {
        char* end = nullptr;
        const double x = std::strtod(list.c_str() + at, &end);
        const double z = std::strtod(end + 1, &end);
        places.push_back({x, z});
        at = size_t(end - list.c_str()) + 1;
    }
    if (places.empty()) places.push_back({arguments.number("x", 0), arguments.number("z", 0)});
    for (const Vec2 place : places) requestAround(world, place, 0.5 * arguments.number("span", 96) + 8, Level::Ground);
    for (size_t i = 0; i < places.size(); ++i) renderSlice(world, arguments, places[i].x, places[i].z, places.size() > 1 ? "slice" + std::to_string(i) : "slice");
    return 0;
}

// terra find --rock Name --x --z [--radius m]: steep places where that rock is at the surface.
int findCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const Vec2 centre{arguments.number("x", 0), arguments.number("z", 0)};
    const double radius = arguments.number("radius", 30000);
    const std::string wanted = arguments.text("rock", "Sandstone");
    const double least = arguments.number("min-slope", 0.35), most = arguments.number("max-slope", 1e9);
    const double cell = specOf(Level::Local).cell;
    requestAround(world, centre, radius, Level::Local);
    struct Hit { double slope; Vec2 at; float ground; };
    std::vector<Hit> hits;
    const TileKey low = tileAt(Level::Local, {centre.x - radius, centre.z - radius}), high = tileAt(Level::Local, {centre.x + radius, centre.z + radius});
    size_t surface = 0, total = 0;
    for (int tz = low.z; tz <= high.z; ++tz)
        for (int tx = low.x; tx <= high.x; ++tx) {
            const auto tile = world.atlas.require({Level::Local, tx, tz});
            for (int z = 1; z < TileCells - 1; ++z)
                for (int x = 1; x < TileCells - 1; ++x) {
                    const size_t i = size_t(z) * TileCells + size_t(x);
                    ++total;
                    if (definitionOf(Material(tile->rock[i])).name != wanted) continue;
                    ++surface;
                    const auto ground = [&](size_t k) { return tile->now.rock[k] + tile->now.sediment[0][k] + tile->now.sediment[1][k] + tile->now.sediment[2][k]; };
                    const double slope = std::hypot(ground(i + 1) - ground(i - 1), ground(i + TileCells) - ground(i - TileCells)) / (2 * cell);
                    const double cover = ground(i) - tile->now.rock[i];
                    if (slope < least || slope > most || cover > 2 || tile->water[i] > ground(i)) continue;
                    hits.push_back({slope, {(tx * double(TileCells) + x + 0.5) * cell, (tz * double(TileCells) + z + 0.5) * cell}, float(ground(i))});
                }
        }
    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.slope > b.slope; });
    std::printf("%s at the surface on %.2f %% of %zu local cells; %zu steep bare cells\n", wanted.c_str(), 100.0 * double(surface) / double(total), total, hits.size());
    for (size_t i = 0; i < std::min<size_t>(hits.size(), 12); i += 1) std::printf("  x %.0f  z %.0f  ground %.0f m  slope %.2f\n", hits[i].at.x, hits[i].at.z, hits[i].ground, hits[i].slope);
    return 0;
}
