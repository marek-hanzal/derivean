#include "Arguments.hpp"

#include "atmosphere/type/Atmosphere.hpp"
#include "climate/fn/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "core/service/Workers.hpp"
#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "planet/type/Planet.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <functional>

using namespace derivean::sim;

namespace {

constexpr int Pixels = 512;
constexpr int PerCell = Pixels / AirCells;

struct Tint {
    double r{}, g{}, b{};
};

Tint blend(Tint a, Tint b, double share) {
    const double t = saturate(share);
    return {mix(a.r, b.r, t), mix(a.g, b.g, t), mix(a.b, b.b, t)};
}

// Colour along evenly spaced stops for a value already scaled to [0, 1].
Tint ramp(std::initializer_list<Tint> stops, double value) {
    const double at = saturate(value) * double(stops.size() - 1);
    const size_t index = std::min(size_t(at), stops.size() - 2);
    return blend(stops.begin()[index], stops.begin()[index + 1], at - double(index));
}

double read(const AirField& field, int px, int pz) {
    return sample(field, (px + 0.5) / PerCell - 0.5, (pz + 0.5) / PerCell - 0.5, Kernel::Linear, Edge::Wrap);
}

struct Maps {
    const Planet& planet;
    const Atmosphere& air;
    std::string directory;

    bool land(int px, int pz) const {
        const int scale = Pixels / PlanetCells;
        return planet.elevation.at(wrapIndex(px / scale, PlanetCells), wrapIndex(pz / scale, PlanetCells)) > planet.seaLevel;
    }

    bool coast(int px, int pz) const {
        const bool here = land(px, pz);
        return here != land(px + 1, pz) || here != land(px, pz + 1);
    }

    void write(const std::string& name, const std::function<Tint(int, int, bool)>& paint, bool arrows = false) const {
        std::vector<uint8_t> image(size_t(Pixels) * Pixels * 3);
        for (int pz = 0; pz < Pixels; ++pz) {
            for (int px = 0; px < Pixels; ++px) {
                Tint tint = paint(px, pz, land(px, pz));
                if (coast(px, pz)) tint = blend(tint, {20, 20, 20}, 0.65);
                putPixel(image, Pixels, px, pz, {uint8_t(std::clamp(tint.r, 0.0, 255.0)), uint8_t(std::clamp(tint.g, 0.0, 255.0)),
                                                 uint8_t(std::clamp(tint.b, 0.0, 255.0))});
            }
        }
        if (arrows) drawArrows(image);
        writePng(directory + "/" + name, Pixels, Pixels, image);
    }

    // A short stroke along the wind every eighth cell, with a dark dot at its tail.
    void drawArrows(std::vector<uint8_t>& image) const {
        for (int z = 4; z < AirCells; z += 8) {
            for (int x = 4; x < AirCells; x += 8) {
                const double wx = air.windX.at(x, z), wz = air.windZ.at(x, z);
                const int strokes = int(std::min(28.0, 1.5 * std::hypot(wx, wz)));
                const Vec2 heading = normalized({wx, wz});
                for (int stroke = 0; stroke <= strokes; ++stroke) {
                    const int px = wrapIndex(int((x + 0.5) * PerCell + heading.x * stroke), Pixels);
                    const int pz = wrapIndex(int((z + 0.5) * PerCell + heading.z * stroke), Pixels);
                    putPixel(image, Pixels, px, pz, stroke < 3 ? Rgb{0, 0, 0} : Rgb{255, 255, 255});
                }
            }
        }
    }
};

constexpr Tint GenusTint[Genera] = {{250, 250, 250}, {150, 40, 160}, {150, 150, 150}, {205, 205, 180}, {70, 80, 120},
                                    {250, 200, 90}, {220, 150, 60}, {110, 220, 255}, {90, 170, 250}, {60, 120, 220}};

void report(const Atmosphere& air, const Planet& planet) {
    const WaterLedger ledger = waterLedger(air);
    const double area = PlanetSpan * PlanetSpan;
    std::printf("ledger kg: ocean %.6e vapour %.4e liquid %.4e ice %.4e falling %.4e soil %.4e snow %.4e runoff %.4e\n", ledger.ocean,
                ledger.vapour, ledger.cloudLiquid, ledger.cloudIce, ledger.precipitation, ledger.soil, ledger.snow, ledger.runoff);
    std::printf("ledger total %.15e kg  planet surface water %.15e kg  difference %.3e\n", total(ledger),
                planet.surfaceWater * WaterDensity, total(ledger) - planet.surfaceWater * WaterDensity);
    double fastest = 0, coldest = 1e9, warmest = -1e9, landRain = 0, landCells = 0, snowy = 0, sky = 0;
    std::array<double, Genera> genera{};
    for (int z = 0; z < AirCells; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = air.ground.index(x, z);
            fastest = std::max(fastest, length(bandWind(air, High, x, z)));
            const double ground = air.airTemperature.cells[cell] - LapseRate * air.ground.cells[cell];
            coldest = std::min(coldest, ground);
            warmest = std::max(warmest, ground);
            const double land = 1.0 - air.openWater.cells[cell];
            landCells += land;
            landRain += land * (air.rainRate.cells[cell] + air.snowRate.cells[cell]);
            if (air.snow.cells[cell] > 5.0) snowy += land;
            const AirColumn column = columnAt(air, x, z);
            const CloudReport clouds = describeClouds(column, profileOf(column), air.rainRate.cells[cell] + air.snowRate.cells[cell]);
            sky += clouds.cover;
            for (size_t genus = 0; genus < Genera; ++genus) genera[genus] += clouds.share[genus];
        }
    }
    const double cells = double(AirCells) * AirCells;
    std::printf("planet mean: precipitation %.2f mm/day  evaporation %.2f mm/day  vapour %.1f kg/m2  cloud cover %.0f %%\n",
                air.precipitation / area * 86400, air.evaporation / area * 86400, ledger.vapour / area, 100 * sky / cells);
    std::printf("land: precipitation %.2f mm/day  snow covered %.1f %%   air at ground %.1f .. %.1f C   fastest high wind %.1f m/s\n",
                landCells > 0 ? landRain / landCells * 86400 : 0.0, landCells > 0 ? 100 * snowy / landCells : 0.0, coldest, warmest, fastest);
    std::printf("sky share by genus, %%:");
    for (size_t genus = 0; genus < Genera; ++genus) std::printf("  %s %.1f", genusName(Genus(genus)), 100 * genera[genus] / cells);
    std::printf("\n");
}

} // namespace

// terra atmosphere --seed n --age years --day d --out directory: weather maps of calendar day d.
int atmosphereCommand(const Arguments& arguments) {
    const double age = arguments.number("age", 4.6e9);
    const double day = arguments.number("day", 200);
    Workers workers(unsigned(arguments.number("threads", 0)));
    const Planet planet = evolvePlanet(arguments.seed(), age, workers);

    auto clock = std::chrono::steady_clock::now();
    const auto lap = [&] {
        const auto now = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(now - clock).count();
        clock = now;
        return seconds;
    };
    Atmosphere air = startAtmosphere(planet, Calendar{age, 0}, workers);
    const double started = lap();
    const int64_t first = air.step;
    advanceAtmosphere(air, Calendar{age, day * DaySeconds}, workers);
    const double advanced = lap();
    const double steps = double(air.step - first);
    std::printf("start %.3f s (%d steps)  advance to day %.1f: %.3f s, %.3f ms per step, %.1f ms per day (%u workers)\n", started,
                AirSpinUp, day, advanced, steps > 0 ? 1000 * advanced / steps : 0.0,
                steps > 0 ? 1000 * advanced / steps * AirStepsPerDay : 0.0, workers.threads());
    report(air, planet);

    const WeatherReport sample = inspectWeather(air, {arguments.number("x", 0), arguments.number("z", 0)}, planet.seaLevel, Calendar{age, day * DaySeconds});
    std::printf("at x %.0f z %.0f: %.1f C, cloud cover %.0f %%, %s\n", arguments.number("x", 0), arguments.number("z", 0),
                sample.weather.temperature, 100 * sample.clouds.cover, sample.cause.c_str());

    const Maps maps{planet, air, arguments.text("out", ".")};
    const CloudField clouds = cloudField(air);
    const auto channel = [&](const std::vector<float>& values, int px, int pz) {
        return double(values[air.ground.index(wrapIndex(px / PerCell, AirCells), wrapIndex(pz / PerCell, AirCells))]);
    };
    const Tint sea{24, 44, 82}, soil{92, 100, 66};
    const char* const bandNames[AirBands] = {"low", "middle", "high"};
    for (int band = 0; band < AirBands; ++band) {
        Field<double> cover(AirCells, AirCells);
        for (size_t cell = 0; cell < cover.size(); ++cell) cover.cells[cell] = clouds.band[size_t(band)].cover[cell];
        maps.write(std::string("air-cloud-") + bandNames[band] + ".png", [&](int px, int pz, bool land) {
            return blend(land ? soil : sea, {250, 250, 250}, read(cover, px, pz));
        });
    }
    // All bands seen from above: low cloud grey-white, middle cloud white, high cloud a thin blue veil on top.
    maps.write("air-cloud.png", [&](int px, int pz, bool land) {
        Tint tint = blend(land ? soil : sea, {200, 200, 196}, channel(clouds.band[Low].cover, px, pz));
        tint = blend(tint, {250, 250, 250}, channel(clouds.band[Middle].cover, px, pz));
        return blend(tint, {215, 235, 255}, 0.7 * channel(clouds.band[High].cover, px, pz));
    });
    // The genus of the thickest layer, dimmed where the sky is mostly clear.
    maps.write("air-genus.png", [&](int px, int pz, bool land) {
        size_t thickest = Low;
        for (size_t band = Middle; band < AirBands; ++band)
            if (channel(clouds.band[band].cover, px, pz) > channel(clouds.band[thickest].cover, px, pz)) thickest = band;
        const size_t cell = air.ground.index(wrapIndex(px / PerCell, AirCells), wrapIndex(pz / PerCell, AirCells));
        return blend(land ? Tint{40, 44, 30} : Tint{10, 18, 34}, GenusTint[clouds.band[thickest].genus[cell]], channel(clouds.band[thickest].cover, px, pz));
    });
    maps.write("air-precipitation.png", [&](int px, int pz, bool land) {
        const double rain = read(air.rainRate, px, pz) * 3600, snow = read(air.snowRate, px, pz) * 3600;   // mm/h
        Tint tint = land ? Tint{60, 60, 52} : Tint{22, 26, 40};
        tint = blend(tint, {40, 150, 255}, std::sqrt(rain / 1.5));
        return blend(tint, {255, 120, 220}, std::sqrt(snow / 0.8));
    });
    maps.write("air-vapour.png", [&](int px, int pz, bool) {
        const double column = read(air.vapour[Low], px, pz) + read(air.vapour[Middle], px, pz) + read(air.vapour[High], px, pz);
        return ramp({{90, 60, 30}, {200, 180, 120}, {120, 190, 150}, {40, 120, 200}, {20, 30, 120}}, column / 60.0);
    });
    maps.write("air-wind.png", [&](int px, int pz, bool) {
        const double speed = std::hypot(read(air.windX, px, pz), read(air.windZ, px, pz));
        return ramp({{16, 24, 60}, {30, 110, 140}, {90, 180, 90}, {230, 200, 60}, {220, 60, 40}}, speed / 32.0);
    }, true);
    maps.write("air-temperature.png", [&](int px, int pz, bool) {
        const double ground = read(air.airTemperature, px, pz) - LapseRate * read(air.ground, px, pz);
        return ramp({{90, 40, 140}, {40, 60, 200}, {80, 190, 230}, {240, 240, 240}, {250, 220, 80}, {230, 110, 40}, {150, 20, 30}},
                    (ground + 45.0) / 80.0);
    });
    maps.write("air-snow.png", [&](int px, int pz, bool land) {
        const double cover = saturate(read(air.snow, px, pz) / 60.0);
        return blend(land ? soil : sea, {255, 255, 255}, land ? std::sqrt(cover) : 0.0);
    });
    maps.write("air-vorticity.png", [&](int px, int pz, bool) {
        const double turning = read(air.vorticity, px, pz) / 6e-5;
        return turning > 0 ? blend({235, 235, 235}, {200, 40, 40}, turning) : blend({235, 235, 235}, {40, 60, 200}, -turning);
    });
    return 0;
}
