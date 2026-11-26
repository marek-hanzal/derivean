#include "Groundwater.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr int Zoom = 2;   // pixels per local cell

struct Canvas {
    int side{};
    std::vector<uint8_t> image;
    double originX{}, originZ{}, cell{};

    void dot(double x, double z, int radius, Rgb colour) {
        const int px = int((x - originX) / cell * Zoom), pz = int((z - originZ) / cell * Zoom);
        for (int dz = -radius; dz <= radius; ++dz)
            for (int dx = -radius; dx <= radius; ++dx)
                if (dx * dx + dz * dz <= radius * radius && px + dx >= 0 && pz + dz >= 0 && px + dx < side && pz + dz < side) putPixel(image, side, px + dx, pz + dz, colour);
    }
    void line(double ax, double az, double bx, double bz, int radius, Rgb colour) {
        const int steps = std::max(1, int(std::hypot(bx - ax, bz - az) / cell * Zoom * 1.5));
        for (int step = 0; step <= steps; ++step) dot(ax + (bx - ax) * step / steps, az + (bz - az) * step / steps, radius, colour);
    }
};

Rgb depthColour(double depth) {
    constexpr double Stops[6] = {0, 2, 10, 30, 75, 150};
    constexpr Rgb Colours[6] = {{40, 90, 220}, {60, 170, 220}, {90, 190, 110}, {225, 215, 90}, {225, 140, 60}, {170, 50, 40}};
    for (int i = 1; i < 6; ++i)
        if (depth <= Stops[i]) {
            const double t = (depth - Stops[i - 1]) / (Stops[i] - Stops[i - 1]);
            return {uint8_t(mix(Colours[i - 1].r, Colours[i].r, t)), uint8_t(mix(Colours[i - 1].g, Colours[i].g, t)), uint8_t(mix(Colours[i - 1].b, Colours[i].b, t))};
        }
    return Colours[5];
}

// Shaded relief in one tint per cell; `tint` returns the colour of a cell before shading.
template <typename Tint> Canvas shaded(const WaterBlock& block, Tint tint) {
    const Mosaic& ground = block.ground;
    Canvas canvas{ground.side() * Zoom, std::vector<uint8_t>(size_t(ground.side()) * Zoom * size_t(ground.side()) * Zoom * 3), double(ground.first.x) * TileCells * ground.cell,
                  double(ground.first.z) * TileCells * ground.cell, ground.cell};
    for (int z = 0; z < ground.side(); ++z)
        for (int x = 0; x < ground.side(); ++x) {
            const GroundwaterTile& tile = *block.tiles[size_t(z / TileCells) * size_t(ground.tiles) + size_t(x / TileCells)];
            const size_t cell = size_t(z % TileCells) * TileCells + size_t(x % TileCells);
            const Rgb base = tint(tile, cell, x, z);
            const double shade = hillshade(ground.ground, x, z, ground.cell, 1.5);
            const Rgb lit{uint8_t(std::min(255.0, base.r * shade)), uint8_t(std::min(255.0, base.g * shade)), uint8_t(std::min(255.0, base.b * shade))};
            for (int dz = 0; dz < Zoom; ++dz)
                for (int dx = 0; dx < Zoom; ++dx) putPixel(canvas.image, canvas.side, x * Zoom + dx, z * Zoom + dz, lit);
        }
    return canvas;
}

void save(const Canvas& canvas, const std::string& path) {
    writePng(path, canvas.side, canvas.side, canvas.image);
    std::printf("  %s\n", path.c_str());
}

} // namespace

void writeWaterMaps(const WaterBlock& block, const std::string& out, const std::string& tag) {
    const Mosaic& ground = block.ground;
    const auto water = [&](int x, int z) { return ground.flooded(x, z); };
    std::printf("maps, %d pixels per local cell, north up:\n", Zoom);

    Canvas table = shaded(block, [&](const GroundwaterTile& tile, size_t cell, int x, int z) {
        return water(x, z) ? Rgb{20, 40, 110} : depthColour(ground.ground.at(x, z) - tile.table[cell]);
    });
    save(table, out + "/groundwater-" + tag + "-table.png");

    Canvas springs = shaded(block, [&](const GroundwaterTile& tile, size_t cell, int x, int z) {
        return water(x, z) ? Rgb{50, 80, 150} : tile.state[cell] & Drained ? Rgb{80, 120, 190} : tile.state[cell] & HasPerched ? Rgb{150, 160, 140} : Rgb{170, 170, 170};
    });
    constexpr Rgb KindColour[4] = {{255, 255, 255}, {255, 225, 40}, {235, 40, 30}, {230, 60, 220}};
    for (const auto& tile : block.tiles)
        for (size_t i = 0; i < tile->springs.size(); ++i) {
            const Spring& spring = tile->springs[i];
            const int size = spring.discharge < 1e-3f ? 1 : spring.discharge < 1e-2f ? 2 : spring.discharge < 0.1f ? 3 : 5;
            springs.dot(spring.position.x, spring.position.z, size + 1, {0, 0, 0});
            springs.dot(spring.position.x, spring.position.z, size, spring.temperature > 40 ? Rgb{255, 120, 0} : KindColour[int(tile->records[i].kind)]);
        }
    save(springs, out + "/groundwater-" + tag + "-springs.png");

    Canvas conduits = shaded(block, [&](const GroundwaterTile& tile, size_t cell, int x, int z) {
        if (water(x, z)) return Rgb{50, 80, 150};
        const uint8_t state = tile.state[cell];
        return state & LowerKarst ? Rgb{150, 200, 215} : state & UpperKarst ? Rgb{205, 200, 140} : state & Drained ? Rgb{90, 125, 190} : Rgb{175, 175, 175};
    });
    for (const auto& tile : block.tiles) {
        for (size_t i = 0; i < tile->caves->conduits.size(); ++i) {
            const Conduit& c = tile->caves->conduits[i];
            const ConduitRecord& record = tile->passages[i];
            const bool shaft = record.opens == 1;
            if (shaft) conduits.dot(c.ax, c.az, 3, {230, 30, 30});
            else conduits.line(c.ax, c.az, c.bx, c.bz, c.radius >= 2 ? 1 : 0, record.level ? Rgb{240, 140, 20} : c.radius >= 0.5f ? Rgb{0, 0, 0} : Rgb{70, 70, 90});
        }
        for (size_t i = 0; i < tile->springs.size(); ++i)
            if (tile->records[i].kind == SpringKind::Karst) conduits.dot(tile->springs[i].position.x, tile->springs[i].position.z, 3, {230, 60, 220});
    }
    save(conduits, out + "/groundwater-" + tag + "-conduits.png");
}
