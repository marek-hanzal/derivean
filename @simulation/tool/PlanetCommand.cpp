#include "Arguments.hpp"

#include "climate/type/Climate.hpp"
#include "core/fn/Hash.hpp"
#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "ore/type/Ore.hpp"
#include "planet/fn/Heat.hpp"
#include "planet/type/Planet.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>

using namespace derivean::sim;

namespace {

void save(const std::string& path, const std::vector<uint8_t>& image, int scale) {
    const int n = PlanetCells;
    std::vector<uint8_t> big(size_t(n) * scale * size_t(n) * scale * 3);
    for (int z = 0; z < n * scale; ++z)
        for (int x = 0; x < n * scale; ++x)
            for (int c = 0; c < 3; ++c)
                big[(size_t(z) * size_t(n * scale) + size_t(x)) * 3 + size_t(c)] = image[(size_t(z / scale) * size_t(n) + size_t(x / scale)) * 3 + size_t(c)];
    writePng(path, n * scale, n * scale, big);
}

// Heat from below and the bodies that cut the layering, sampled finer than a planet cell because
// magmatic centres, veined districts and pipes are all much smaller than one.
void saveGeology(const Planet& planet, const std::string& path) {
    constexpr int Size = 1536;
    const double step = PlanetSpan / Size;
    std::vector<uint8_t> image(size_t(Size) * Size * 3);
    long land = 0, hot = 0, boiling = 0, veined = 0, piped = 0, kind[5] = {}, rock[2][5] = {};
    for (int z = 0; z < Size; ++z) {
        for (int x = 0; x < Size; ++x) {
            const Vec2 at{(x + 0.5) * step, (z + 0.5) * step};
            const bool dry = planet.elevation.at(int(at.x / PlanetCell), int(at.z / PlanetCell)) > planet.seaLevel;
            const Geotherm geotherm = geothermAt(planet, at);
            const StrataColumn column = strataAt(planet, at);
            const OreField ore = oreFieldAt(planet.seed, column, column.material);
            const double warm = saturate((geotherm.gradient - 0.02) / 0.2);
            Rgb color = dry ? Rgb{uint8_t(70 + 185 * warm), uint8_t(70 + 60 * warm), uint8_t(70 - 40 * warm)}
                            : Rgb{uint8_t(20 + 200 * warm), uint8_t(30 + 30 * warm), 60};
            const bool pipe = ore.pipeRadius > 0 && length(column.material - ore.pipe) < 1.5 * step;
            if (ore.veining > 0.15f) color = {90, 230, 120};
            if (pipe) color = {120, 200, 255};
            putPixel(image, Size, x, z, color);
            if (!dry) continue;
            ++land;
            ++kind[size_t(column.basement)];
            hot += geotherm.gradient > 0.06f, boiling += geotherm.gradient > 0.15f, veined += ore.veining > 0.15f, piped += pipe;
            // The bed just under the top of the record, and the one a valley 150 m deep cuts into.
            for (int cut = 0; cut < 2; ++cut) {
                const float depth = cut ? 150.0f : 2.0f;
                const Material top = bedAt(planet.seed, column, unitAt(column, depth, 0.0f), depth, column.material).rock;
                const Origin origin = definitionOf(top).origin;
                const bool carbonate = top == Material::Limestone || top == Material::Chalk;
                const bool plutonic = top == Material::Granite || top == Material::Peridotite || top == Material::Kimberlite;
                ++rock[cut][origin == Origin::Metamorphic ? 2 : origin == Origin::Igneous ? (plutonic ? 3 : 4) : 0];
                rock[cut][1] += carbonate;
            }
        }
    }
    writePng(path, Size, Size, image);
    std::printf("heat and ore, share of land: gradient above 60 K/km %.2f %%, above 150 K/km %.3f %%, veined ground %.2f %%, pipes %ld\n",
                100.0 * hot / std::max(1L, land), 100.0 * boiling / std::max(1L, land), 100.0 * veined / std::max(1L, land), piped);
    for (int cut = 0; cut < 2; ++cut)
        std::printf("rock %s %%: sedimentary %.1f (carbonate %.1f) metamorphic %.1f plutonic %.1f volcanic %.1f   (Earth 64 (21) 13 7 6)\n",
                    cut ? "150 m under the land" : "at the top of the land", 100.0 * rock[cut][0] / std::max(1L, land), 100.0 * rock[cut][1] / std::max(1L, land),
                    100.0 * rock[cut][2] / std::max(1L, land), 100.0 * rock[cut][3] / std::max(1L, land), 100.0 * rock[cut][4] / std::max(1L, land));
    std::printf("basement of the land %%: primordial %.1f oceanic %.1f arc %.1f craton %.1f orogen %.1f\n", 100.0 * kind[0] / std::max(1L, land),
                100.0 * kind[1] / std::max(1L, land), 100.0 * kind[2] / std::max(1L, land), 100.0 * kind[3] / std::max(1L, land), 100.0 * kind[4] / std::max(1L, land));
}

} // namespace

int planetCommand(const Arguments& arguments) {
    const uint64_t seed = arguments.seed();
    const double age = arguments.number("age", 4.6e9);
    const std::string out = arguments.text("out", ".");
    Workers workers;
    const auto start = std::chrono::steady_clock::now();
    const Planet planet = evolvePlanet(seed, age, workers);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    int land = 0, high = 0;
    double top = -1e9, deepest = 1e9, thickest = 0, crustVolume = 0, landHeight = 0;
    for (size_t i = 0; i < planet.elevation.size(); ++i) {
        const double height = planet.elevation.cells[i] - planet.seaLevel;
        top = std::max(top, height);
        deepest = std::min(deepest, height);
        thickest = std::max(thickest, double(planet.columns.cells[i].crust));
        crustVolume += planet.columns.cells[i].crust;
        if (height > 0) {
            ++land;
            landHeight += height;
            if (height > 2000) ++high;
        }
    }
    const double cells = double(planet.elevation.size());
    std::printf("seed %llu age %.4g yr  planet time %.4g yr  %.2f s\n", static_cast<unsigned long long>(seed), age, planet.time, seconds);
    std::printf("surface %.0f K  sea level %.0f m  surface water %.3g m3\n", planet.surfaceKelvin, planet.seaLevel, planet.surfaceWater);
    std::printf("land %.1f %%  mean land height %.0f m  above 2 km %.2f %%  highest %.0f m  deepest %.0f m\n",
                100.0 * land / cells, land ? landHeight / land : 0.0, 100.0 * high / cells, top, deepest);
    // Shares of the land by height above the sea; docs/reference-landscapes.md has the Earth's (29 | 28 | 24 | 13 | 3 | 1 | 2).
    constexpr double Bands[7] = {200, 500, 1000, 2000, 3000, 4000, 1e9};
    long band[7] = {};
    for (size_t i = 0; i < planet.elevation.size(); ++i) {
        const double height = planet.elevation.cells[i] - planet.seaLevel;
        if (height > 0) ++band[std::find_if(Bands, Bands + 6, [&](double edge) { return height <= edge; }) - Bands];
    }
    std::printf("land by height %%: <200 m %.0f | <500 %.0f | <1000 %.0f | <2000 %.0f | <3000 %.1f | <4000 %.1f | above %.1f\n", 100.0 * band[0] / std::max(1, land),
                100.0 * band[1] / std::max(1, land), 100.0 * band[2] / std::max(1, land), 100.0 * band[3] / std::max(1, land), 100.0 * band[4] / std::max(1, land),
                100.0 * band[5] / std::max(1, land), 100.0 * band[6] / std::max(1, land));
    std::printf("thickest crust %.1f km  mean felsic crust %.2f km  epoch %d (%d plates)\n", thickest / 1000,
                crustVolume / cells / 1000, planet.epoch.index, int(planet.epoch.plates.size()));
    std::printf("ledger km3: eroded %.3g deposited %.3g erupted %.3g subducted %.3g accreted %.3g precipitated %.3g\n",
                planet.ledger.eroded / 1e9, planet.ledger.deposited / 1e9, planet.ledger.erupted / 1e9,
                planet.ledger.subducted / 1e9, planet.ledger.accreted / 1e9, planet.ledger.precipitated / 1e9);

    const int n = PlanetCells;
    save(out + "/planet-relief.png", reliefImage(planet.elevation, planet.seaLevel, PlanetCell, 12.0), 3);

    std::vector<uint8_t> plates(size_t(n) * n * 3), rain(size_t(n) * n * 3);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const uint64_t tint = hash(planet.epoch.seed, planet.owner.at(x, z));
            const bool dry = planet.elevation.at(x, z) > planet.seaLevel;
            const double dim = dry ? 1.0 : 0.45;
            const ContactCell& contact = planet.contact.at(x, z);
            Rgb color{uint8_t((90 + (tint & 127)) * dim), uint8_t((90 + ((tint >> 8) & 127)) * dim), uint8_t((90 + ((tint >> 16) & 127)) * dim)};
            if (contact.distance < PlanetCell) color = contact.convergence > 0.002 ? Rgb{230, 60, 50} : contact.convergence < -0.002 ? Rgb{60, 120, 240} : Rgb{240, 220, 80};
            putPixel(plates, n, x, z, color);
            const double wet = std::min(1.0, planet.precipitation.at(x, z) / 2.5);
            putPixel(rain, n, x, z, dry ? Rgb{uint8_t(220 - 180 * wet), uint8_t(200 - 60 * wet), uint8_t(120 + 100 * wet)} : Rgb{20, 30, 60});
        }
    }
    save(out + "/planet-plates.png", plates, 3);
    save(out + "/planet-rain.png", rain, 3);
    saveGeology(planet, out + "/planet-geology.png");
    return 0;
}
