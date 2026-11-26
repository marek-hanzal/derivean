#include "inspect/fn/Layer.hpp"

#include "core/fn/Hash.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

#include <cstdio>

namespace derivean::sim {
namespace {

struct Scale {
    double low, high;
    bool logarithmic;
    const char* unit;
};

// Pale yellow through orange to deep purple: light is little, dark is much.
Rgb sequential(double t) {
    constexpr double Stops[5][3] = {{252, 248, 205}, {250, 196, 95}, {226, 104, 52}, {150, 36, 88}, {48, 14, 70}};
    const double at = saturate(t) * 4;
    const int below = std::min(3, int(at));
    const double f = at - below;
    return {uint8_t(mix(Stops[below][0], Stops[below + 1][0], f)), uint8_t(mix(Stops[below][1], Stops[below + 1][1], f)),
            uint8_t(mix(Stops[below][2], Stops[below + 1][2], f))};
}

// Blue below zero, white at zero, red above.
Rgb diverging(double t) {
    const double s = std::clamp(t, -1.0, 1.0);
    return s < 0 ? Rgb{uint8_t(mix(245, 40, -s)), uint8_t(mix(245, 90, -s)), uint8_t(mix(245, 200, -s))}
                 : Rgb{uint8_t(mix(245, 205, s)), uint8_t(mix(245, 50, s)), uint8_t(mix(245, 40, s))};
}

double place(const Scale& scale, double value) {
    if (!scale.logarithmic) return (value - scale.low) / (scale.high - scale.low);
    return std::log(std::max(value, scale.low) / scale.low) / std::log(scale.high / scale.low);
}

Rgb shaded(Rgb color, double shade) {
    return {uint8_t(std::min(255.0, color.r * shade)), uint8_t(std::min(255.0, color.g * shade)), uint8_t(std::min(255.0, color.b * shade))};
}

void drawLine(std::vector<uint8_t>& image, int side, Vec2 from, Vec2 to, Rgb color) {
    const int steps = std::max(1, int(length(to - from)));
    for (int k = 0; k <= steps; ++k) {
        const Vec2 at = from + (to - from) * (double(k) / steps);
        const int x = int(std::lround(at.x)), z = int(std::lround(at.z));
        if (x >= 0 && z >= 0 && x < side && z < side) putPixel(image, side, x, z, color);
    }
}

// A shaft with two barbs at its head.
void drawArrow(std::vector<uint8_t>& image, int side, Vec2 from, Vec2 to, Rgb color) {
    drawLine(image, side, from, to, color);
    const Vec2 back = normalized(from - to) * 5.0, across = perpendicular(back) * 0.6;
    drawLine(image, side, to, to + back + across, color);
    drawLine(image, side, to, to + back - across, color);
}

void paintPlates(const World& world, TileKey first, int side, double cell, const Field<float>& ground, std::vector<uint8_t>& image, std::string& legend) {
    const Planet& planet = world.planet;
    const double activity = epochActivity(planet.epoch, planet.time);
    const auto pointOf = [&](int x, int z) {
        return planetPoint(planet.seed, {(double(first.x) * TileCells + x + 0.5) * cell, (double(first.z) * TileCells + z + 0.5) * cell});
    };
    const auto cellOf = [](Vec2 point) {
        return std::pair{wrapIndex(floorToInt(point.x / PlanetCell), PlanetCells), wrapIndex(floorToInt(point.z / PlanetCell), PlanetCells)};
    };
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            const Vec2 point = pointOf(x, z);
            const auto [cx, cz] = cellOf(point);
            const uint64_t tint = hash(planet.epoch.seed, planet.owner.at(cx, cz));
            Rgb color{uint8_t(110 + (tint & 127)), uint8_t(110 + ((tint >> 8) & 127)), uint8_t(110 + ((tint >> 16) & 127))};
            const ContactSample contact = contactAt(planet, point);
            // A contact is drawn as wide as the planet resolves it: half a planet cell either side.
            if (contact.distance < 0.5 * PlanetCell) {
                const double closing = contact.convergence * activity;
                color = closing > 0.002 ? Rgb{225, 55, 45} : closing < -0.002 ? Rgb{55, 110, 235} : Rgb{240, 215, 70};
            } else {
                color = shaded(color, hillshade(ground, x, z, cell, 2.0));
            }
            putPixel(image, side, x, z, color);
        }
    }
    // Motion of the plate under each lattice point against the plate across its nearest contact.
    constexpr int Lattice = 96;
    constexpr double PixelsPerMetreYear = 1500.0;   // 10 mm/yr draws 15 pixels
    for (int z = Lattice / 2; z < side; z += Lattice) {
        for (int x = Lattice / 2; x < side; x += Lattice) {
            const auto [cx, cz] = cellOf(pointOf(x, z));
            const ContactCell& nearest = planet.contact.at(cx, cz);
            if (nearest.distance >= 2 * ContactReach) continue;
            const double reach = nearest.distance / PlanetCell + 1.0;
            const uint16_t own = planet.owner.at(cx, cz);
            const uint16_t other = planet.owner.at(wrapIndex(cx + int(std::lround(nearest.normalX * reach)), PlanetCells),
                                                   wrapIndex(cz + int(std::lround(nearest.normalZ * reach)), PlanetCells));
            const Vec2 relative = (planet.epoch.plates[own].velocity - planet.epoch.plates[other].velocity) * (activity * PixelsPerMetreYear);
            drawArrow(image, side, {double(x), double(z)}, Vec2{double(x), double(z)} + relative, {20, 20, 20});
        }
    }
    legend = "plates: one colour per plate; contacts red closing, blue opening, yellow sliding; arrows show motion against the plate across the nearest contact, 15 px per 10 mm/yr";
}

} // namespace

bool paintLayer(const World& world, const std::string& name, TileKey first, int tiles, std::vector<uint8_t>& image, std::string& legend) {
    const Scale scale = name == "cover" ? Scale{0.1, 100, true, "m of loose cover"}
                      : name == "discharge" ? Scale{1e3, 1e11, true, "m3/yr"}
                      : name == "erosion" ? Scale{1e-6, 1e-2, true, "m/yr of erosion"}
                      : name == "deposition" ? Scale{1e-6, 1e-2, true, "m/yr of deposition"}
                      : name == "uplift" ? Scale{-1e-3, 1e-3, false, "m/yr of rock uplift (blue sinking, red rising)"}
                      : name == "rain" ? Scale{0, 3, false, "m/yr of precipitation"}
                      : name == "crust" ? Scale{0, 60, false, "km of felsic crust"}
                      : name == "crust-age" ? Scale{1e6, 4.6e9, true, "yr since the lid solidified"}
                      : Scale{0, 0, false, nullptr};
    if (!scale.unit && name != "plates") return false;

    const int side = tiles * TileCells;
    const double cell = specOf(first.level).cell;
    Field<float> ground(side, side), value(side, side);
    const Setting& setting = world.atlas.setting();
    for (int dz = 0; dz < tiles; ++dz) {
        for (int dx = 0; dx < tiles; ++dx) {
            const auto tile = world.atlas.find({first.level, first.x + dx, first.z + dz});
            if (!tile) continue;
            for (int z = 0; z < TileCells; ++z) {
                for (int x = 0; x < TileCells; ++x) {
                    const size_t i = size_t(z) * TileCells + size_t(x);
                    const float cover = tile->now.sediment[0][i] + tile->now.sediment[1][i] + tile->now.sediment[2][i];
                    ground.at(dx * TileCells + x, dz * TileCells + z) = tile->now.rock[i] + cover;
                    float& out = value.at(dx * TileCells + x, dz * TileCells + z);
                    if (name == "cover") out = cover;
                    else if (name == "discharge") out = tile->discharge[i];
                    else if (name == "erosion") out = tile->erosionRate[i];
                    else if (name == "deposition") out = tile->depositionRate[i];
                    else if (name == "uplift") out = tile->uplift[i];
                    else if (name == "rain") out = tile->rain[i];
                    else if (name == "crust" || name == "crust-age") {
                        const Vec2 here{(double(first.x + dx) * TileCells + x + 0.5) * cell, (double(first.z + dz) * TileCells + z + 0.5) * cell};
                        const Vec2 point = planetPoint(setting.seed, here);
                        out = name == "crust" ? float(samplePlanet(setting.backdrop->crust, point, Kernel::Linear) / 1000) : planetColumn(world.planet, point).oceanAge;
                    }
                }
            }
        }
    }
    if (name == "plates") {
        paintPlates(world, first, side, cell, ground, image, legend);
        return true;
    }
    const double exaggeration = first.level == Level::Region ? 6.0 : 1.5;
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            const double t = place(scale, value.at(x, z));
            const Rgb color = name == "uplift" ? diverging(2 * t - 1) : sequential(t);
            putPixel(image, side, x, z, shaded(color, 0.55 + 0.45 * hillshade(ground, x, z, cell, exaggeration)));
        }
    }
    char text[200];
    std::snprintf(text, sizeof(text), "%s: %s from %.3g (pale) to %.3g (dark)%s", name.c_str(), scale.unit, scale.low, scale.high,
                  scale.logarithmic ? ", logarithmic" : "");
    legend = text;
    return true;
}

} // namespace derivean::sim
