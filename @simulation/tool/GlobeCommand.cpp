#include "Arguments.hpp"

#include "core/fn/Hash.hpp"
#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "measure/fn/Regularity.hpp"
#include "planet/fn/Band.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/fn/GlobeCover.hpp"
#include "planet/fn/GlobeEvolve.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/type/Planet.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

using namespace derivean::sim;

void printRocks(const Planet& view);       // tool/GlobeRocks.cpp
void printShoreGrain(const Globe& globe);

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

// The numbers of `terra planet`, by the real areas of the cells of the sphere.
void printSurface(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const double sphere = 4 * 3.141592653589793 * PlanetRadius * PlanetRadius;
    double land = 0, high = 0, landHeight = 0, top = -1e9, deepest = 1e9, oceanVolume = 0, continental = 0;
    constexpr double Bands[7] = {200, 500, 1000, 2000, 3000, 4000, 1e9};
    double band[7] = {};
    for (int cell = 0; cell < grid.cells; ++cell) {
        const double area = grid.area[size_t(cell)];
        const double height = globe.elevation.value[size_t(cell)] - globe.seaLevel;
        top = std::max(top, height), deepest = std::min(deepest, height);
        if (height > 0) {
            land += area, landHeight += area * height, high += height > 2000 ? area : 0;
            band[std::find_if(Bands, Bands + 6, [&](double edge) { return height <= edge; }) - Bands] += area;
        } else {
            oceanVolume -= area * height;
        }
        const double crust = viewOf(globe.coupling, globe.pieces, cell, [](const CrustPiece& p) { return p.column.crust; });
        if (crust >= OceanicLimit) continental += area;
    }
    double thickest = 0, felsic = 0, continentalVolume = 0;
    for (const CrustPiece& piece : globe.pieces) {
        thickest = std::max(thickest, double(piece.column.crust));
        felsic += piece.stock.felsic;
        if (piece.column.crust >= float(OceanicLimit)) continentalVolume += piece.stock.felsic;
    }
    std::printf("surface %.0f K  sea level %.0f m  surface water %.3g m3\n", globe.surfaceKelvin, globe.seaLevel, globe.surfaceWater);
    std::printf("land %.1f %%  mean land height %.0f m  above 2 km %.2f %%  highest %.0f m  deepest %.0f m   (Earth 29 %%, ice-free; today 21.4 %%)\n",
                100 * land / sphere, land > 0 ? landHeight / land : 0.0, 100 * high / sphere, top, deepest);
    const auto share = [&](int i) { return land > 0 ? 100 * band[i] / land : 0.0; };
    std::printf("land by height %%: <200 m %.0f | <500 %.0f | <1000 %.0f | <2000 %.0f | <3000 %.1f | <4000 %.1f | above %.1f\n", share(0), share(1), share(2),
                share(3), share(4), share(5), share(6));
    std::printf("thickest crust %.1f km  mean felsic crust %.2f km  continental crust %.1f %% of the area, %.3g km3   (Earth 41 %%, 7.6-8.6e9 km3)\n",
                thickest / 1000, felsic / sphere / 1000, 100 * continental / sphere, continentalVolume / 1e9);
    const double ocean = sphere - land;
    std::printf("ocean %.4g km3  mean depth %.0f m   (Earth 1.335e9 km3, 3 688 m)\n", oceanVolume / 1e9, ocean > 0 ? oceanVolume / ocean : 0.0);
}

// The motion of the lithosphere from the applied Euler vectors, and how the footprints cover the sphere.
void printMotion(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    double sum = 0, area = 0, closing = 0, opening = 0, sliding = 0;
    for (int cell = 0; cell < grid.cells; ++cell) {
        const uint16_t plate = globe.owner.value[size_t(cell)];
        if (plate >= globe.applied.size()) continue;
        const Vec3 v = velocityAt(globe.applied[plate], grid.centre[size_t(cell)]);
        sum += grid.area[size_t(cell)] * dot(v, v), area += grid.area[size_t(cell)];
    }
    for (const int32_t cell : globe.contactZone) {
        const GlobeContact& contact = globe.contact.value[size_t(cell)];
        if (contact.distance > std::sqrt(grid.area[size_t(cell)])) continue;
        (contact.convergence > 0.002 ? closing : contact.convergence < -0.002 ? opening : sliding) += grid.area[size_t(cell)];
    }
    const double boundary = std::max(1.0, closing + opening + sliding);
    std::printf("plates: RMS speed %.1f mm/yr (Earth 43.9 +- 5), boundary closing %.0f %% opening %.0f %% sliding %.0f %%\n", 1000 * std::sqrt(sum / std::max(1.0, area)),
                100 * closing / boundary, 100 * opening / boundary, 100 * sliding / boundary);

    // §4's local measures from the union in every cell (V2, planet/fn/GlobeCover), outside the band and
    // over the whole sphere.
    Band band;
    markBand(globe, 1, band);
    const GlobeCover all = measureCover(globe), outside = measureCover(globe, band.decide);
    const double sphere = 4 * 3.141592653589793 * PlanetRadius * PlanetRadius;
    const auto share = [](long count, long cells) { return 100.0 * double(count) / double(std::max(1L, cells)); };
    std::printf("cover: footprints %.4f of the sphere; open %.3f %% and doubled %.3f %% of it; worst hole %.2f and overlap %.2f of a cell, %.2f %% of cells open and %.2f %% doubled by over 0.1, %.3f %% open by over 0.5\n",
                totalArea(globe.pieces) / sphere, 100 * all.hole, 100 * all.overlap, all.worstHole, all.worstOverlap, share(all.holes[0], all.cells), share(all.overlaps[0], all.cells), share(all.holes[2], all.cells));
    std::printf("cover outside the band: worst hole %.2f and overlap %.2f of a cell, %.2f %% of cells open and %.2f %% doubled by over 0.1\n", outside.worstHole,
                outside.worstOverlap, share(outside.holes[0], outside.cells), share(outside.overlaps[0], outside.cells));
    const GlobeStats& s = globe.stats;
    std::printf("pieces %zu  substeps %ld  births %ld  consumptions %ld  cancelled %ld  deferred %ld  rounds %ld  unions %ld  band cells per step %.0f\n",
                globe.pieces.size(), s.substeps, s.births, s.consumptions, s.cancelled, s.deferred, s.rounds, s.unions,
                double(s.bandCells) / std::max<size_t>(1, globe.seaLevels.size()));
    const char* names[] = {"band", "substeps", "rest", "couple", "contacts", "elevation", "climate", "surface"};
    std::printf("phases s:");
    for (size_t p = 0; p < s.seconds.size(); ++p) std::printf(" %s %.2f", names[p], s.seconds[p]);
    std::printf("\n");
    const GlobeLedger& l = globe.ledger;
    const CrustStock stock = totalStock(globe.pieces);
    std::printf("ledger km3: eroded %.3g deposited %.3g erupted %.3g subducted %.3g accreted %.3g precipitated %.3g stacked %.3g\n", l.eroded / 1e9,
                l.deposited / 1e9, l.erupted / 1e9, l.subducted / 1e9, l.accreted / 1e9, l.precipitated / 1e9, l.stacked / 1e9);
    std::printf("stock km3: felsic %.4g basalt %.4g, interior gained felsic %.3g basalt %.3g; born %.3g km2 consumed %.3g km2\n", stock.felsic / 1e9, stock.basalt / 1e9,
                l.interior.felsic / 1e9, l.interior.basalt / 1e9, l.born / 1e6, l.consumed / 1e6);
}

void saveImages(const Planet& view, const std::string& out) {
    const int n = PlanetCells;
    save(out + "/globe-relief.png", reliefImage(view.elevation, view.seaLevel, PlanetCell, 12.0), 3);
    std::vector<uint8_t> plates(size_t(n) * n * 3), rain(size_t(n) * n * 3);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const uint64_t tint = hash(view.epoch.seed, view.owner.at(x, z));
            const bool dry = view.elevation.at(x, z) > view.seaLevel;
            const double dim = dry ? 1.0 : 0.45;
            const ContactCell& contact = view.contact.at(x, z);
            Rgb color{uint8_t((90 + (tint & 127)) * dim), uint8_t((90 + ((tint >> 8) & 127)) * dim), uint8_t((90 + ((tint >> 16) & 127)) * dim)};
            if (contact.distance < PlanetCell) color = contact.convergence > 0.002 ? Rgb{230, 60, 50} : contact.convergence < -0.002 ? Rgb{60, 120, 240} : Rgb{240, 220, 80};
            putPixel(plates, n, x, z, color);
            const double wet = std::min(1.0, view.precipitation.at(x, z) / 2.5);
            putPixel(rain, n, x, z, dry ? Rgb{uint8_t(220 - 180 * wet), uint8_t(200 - 60 * wet), uint8_t(120 + 100 * wet)} : Rgb{20, 30, 60});
        }
    }
    save(out + "/globe-plates.png", plates, 3);
    save(out + "/globe-rain.png", rain, 3);
}

} // namespace

// terra globe [--seed n] [--age years] [--level 7] [--speeds today|earth] [--out directory]
int globeCommand(const Arguments& arguments) {
    const uint64_t seed = arguments.seed();
    const double age = arguments.number("age", 4.6e9);
    const int level = int(arguments.number("level", 7));
    const PlateSpeeds speeds = arguments.text("speeds", "today") == "earth" ? PlateSpeeds::Earth : PlateSpeeds::Today;
    const std::string out = arguments.text("out", ".");
    Workers workers;
    const auto start = std::chrono::steady_clock::now();
    const Globe globe = evolveGlobe(seed, age, level, speeds, workers);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("seed %llu age %.4g yr  globe time %.4g yr  level %d (%d cells)  speeds %s  %.2f s\n", static_cast<unsigned long long>(seed), age, globe.time, level,
                globe.grid->cells, speeds == PlateSpeeds::Earth ? "earth" : "today", seconds);
    std::printf("epoch %d (%zu plates)\n", globe.epoch.index, globe.epoch.plates.size());
    printSurface(globe);
    printMotion(globe);
    // The crust with what the interior gained against the primordial body, relative (§5: no balancing account).
    const CrustStock before = totalStock(makeGlobe(seed, level, speeds, age).pieces), after = totalStock(globe.pieces);
    std::printf("balance against the primordial body: felsic %.3g basalt %.3g\n", (after.felsic + globe.ledger.interior.felsic - before.felsic) / before.felsic,
                (after.basalt + globe.ledger.interior.basalt - before.basalt) / before.basalt);
    const Planet view = flatViewOf(globe);
    printRocks(view);
    Field<uint8_t> dry(PlanetCells, PlanetCells);
    for (size_t i = 0; i < dry.size(); ++i) dry.cells[i] = view.elevation.cells[i] > view.seaLevel;
    std::printf("shore of the view in straight axis runs of 5 cells %.1f %%   (flat history 37 %%; a coast with no preferred direction reads up to about 25 %%)\n", 100 * straightShoreShare(dry, true, 5));
    printShoreGrain(globe);
    saveImages(view, out);
    return 0;
}
