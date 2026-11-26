#include "Ground.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "ground/type/Detail.hpp"
#include "simulation/fn/Query.hpp"

#include <cstdio>

using namespace derivean::sim;

// terra ground --map dir --x --z [--span m]: the ground tiles under a square as they are stored, one
// pixel per cell: `ground-relief.png` (shade of the surface with its mean slope taken out, water
// blue, stones as their rock), `ground-cover.png` (red coarse, green sand, blue fines, each full at
// one metre; white bare rock) and `ground-flow.png` (discharge, brighter per decade from 100 m³/yr).
int groundMap(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const Vec2 centre{arguments.number("x", 0), arguments.number("z", 0)};
    const double half = 0.5 * arguments.number("span", 100);
    const std::string out = arguments.text("map", ".");
    requestAround(world, centre, half, Level::Ground);
    const TileKey low = tileAt(Level::Ground, {centre.x - half, centre.z - half}), high = tileAt(Level::Ground, {centre.x + half, centre.z + half});
    const int tilesX = high.x - low.x + 1, tilesZ = high.z - low.z + 1, width = tilesX * TileCells, height = tilesZ * TileCells;
    Field<float> ground(width, height), wet(width, height, 0.0f);
    std::vector<uint8_t> relief(size_t(width) * size_t(height) * 3), cover(relief.size()), flow(relief.size());
    Field<uint8_t> stone(width, height, uint8_t(Material::Air));
    double wetCells = 0, most = 0;
    for (int tz = 0; tz < tilesZ; ++tz)
        for (int tx = 0; tx < tilesX; ++tx) {
            const auto tile = world.atlas.require({Level::Ground, low.x + tx, low.z + tz});
            for (int z = 0; z < TileCells; ++z)
                for (int x = 0; x < TileCells; ++x) {
                    const size_t i = size_t(z) * TileCells + size_t(x);
                    const int px = tx * TileCells + x, pz = tz * TileCells + z;
                    const float loose[3] = {tile->now.sediment[0][i], tile->now.sediment[1][i], tile->now.sediment[2][i]};
                    ground.at(px, pz) = tile->now.rock[i] + loose[0] + loose[1] + loose[2] + tile->detail->crust[i];
                    wet.at(px, pz) = std::max(0.0f, tile->water[i] - ground.at(px, pz));
                    wetCells += wet.at(px, pz) > 0.01f;
                    most = std::max(most, double(tile->discharge[i]));
                    const bool bare = loose[0] + loose[1] + loose[2] < 0.03f;
                    putPixel(cover, width, px, pz, bare ? Rgb{255, 255, 255} : Rgb{uint8_t(255 * std::min(1.0f, loose[0])), uint8_t(255 * std::min(1.0f, loose[1])), uint8_t(255 * std::min(1.0f, loose[2]))});
                    const uint8_t bright = uint8_t(std::clamp(40.0 * (std::log10(std::max(1.0f, tile->discharge[i])) - 2.0), 0.0, 255.0));
                    putPixel(flow, width, px, pz, {bright, bright, uint8_t(std::min(255, bright + 40))});
                }
            for (const Clast& clast : tile->detail->clasts) {
                const int reach = std::max(0, int(clast.b / 0.25f));
                for (int z = std::max(0, int(clast.z / 0.25f) - reach); z <= std::min(TileCells - 1, int(clast.z / 0.25f) + reach); ++z)
                    for (int x = std::max(0, int(clast.x / 0.25f) - reach); x <= std::min(TileCells - 1, int(clast.x / 0.25f) + reach); ++x)
                        stone.at(tx * TileCells + x, tz * TileCells + z) = uint8_t(clast.rock);
            }
        }
    // The mean plane out, so that the shade shows the ground's own relief.
    double sx = 0, sz = 0, sxx = 0, szz = 0, sh = 0;
    for (int z = 0; z < height; ++z)
        for (int x = 0; x < width; ++x) {
            const double dx = x - 0.5 * width, dz = z - 0.5 * height;
            sx += dx * ground.at(x, z), sz += dz * ground.at(x, z), sxx += dx * dx, szz += dz * dz, sh += ground.at(x, z);
        }
    Field<float> flat(width, height);
    float lowest = 1e30f, highest = -1e30f;
    for (int z = 0; z < height; ++z)
        for (int x = 0; x < width; ++x) {
            flat.at(x, z) = float(ground.at(x, z) - (x - 0.5 * width) * sx / sxx - (z - 0.5 * height) * sz / szz);
            lowest = std::min(lowest, ground.at(x, z)), highest = std::max(highest, ground.at(x, z));
        }
    for (int z = 0; z < height; ++z)
        for (int x = 0; x < width; ++x) {
            const double shade = hillshade(flat, x, z, 0.25, 2.0);
            Rgb color{uint8_t(std::min(255.0, 150 * shade)), uint8_t(std::min(255.0, 140 * shade)), uint8_t(std::min(255.0, 120 * shade))};
            if (stone.at(x, z) != uint8_t(Material::Air)) {
                const uint32_t rock = definitionOf(Material(stone.at(x, z))).color;
                color = {uint8_t(rock >> 16), uint8_t(rock >> 8), uint8_t(rock)};
            }
            if (wet.at(x, z) > 0.01f) color = {uint8_t(color.r / 3), uint8_t(color.g / 2 + 40), uint8_t(std::min(255, color.b / 2 + 120))};
            putPixel(relief, width, x, z, color);
        }
    writePng(out + "/ground-relief.png", width, height, relief);
    writePng(out + "/ground-cover.png", width, height, cover);
    writePng(out + "/ground-flow.png", width, height, flow);
    std::printf("ground map of %d x %d tiles from X %.0f Z %.0f (0.25 m per pixel): ground %.1f .. %.1f m, %.1f %% under water, largest flow %.3g m3/yr\n%s/ground-relief.png  %s/ground-cover.png  %s/ground-flow.png\n",
                tilesX, tilesZ, low.x * tileSpan(Level::Ground), low.z * tileSpan(Level::Ground), lowest, highest, 100 * wetCells / (double(width) * height), most, out.c_str(), out.c_str(), out.c_str());
    return 0;
}
