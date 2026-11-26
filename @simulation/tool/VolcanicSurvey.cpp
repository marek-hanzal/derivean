#include "Volcanic.hpp"

#include "core/fn/Hash.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Spawn.hpp"
#include "simulation/type/World.hpp"
#include "volcanic/fn/Source.hpp"

#include <algorithm>
#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr double Km3 = 1e9;
constexpr double Standing = 2e6;   // yr since its last eruption within which a vent's edifice is taken to stand

struct Row {
    const Vent* vent{};
    bool land{};
    double last{};          // yr before the world's age of its last recorded step with an eruption
    double next{1e30};      // yr from the world's age to its next eruption
    double nextVolume{};
};

double nearest(Vec2 from, const std::vector<Row>& rows, bool activeOnly) {
    double best = 1e30;
    for (const Row& row : rows) {
        if (!row.land || (activeOnly ? row.vent->supply <= 0 : row.last > Standing)) continue;
        best = std::min(best, std::hypot(wrapDelta(row.vent->centre.x - from.x, PlanetSpan), wrapDelta(row.vent->centre.z - from.z, PlanetSpan)));
    }
    return best;
}

double percentile(std::vector<double> values, double share) {
    if (values.empty()) return 0;
    std::sort(values.begin(), values.end());
    return values[std::min(values.size() - 1, size_t(share * double(values.size())))];
}

} // namespace

int volcanicSurvey(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const Setting& setting = world.atlas.setting();
    const Planet& planet = world.planet;
    const Window window = windowAt(Level::Region, setting.age);
    const std::vector<Vent> vents = ventsNear(setting, {0, 0}, {PlanetSpan, PlanetSpan}, 0);

    // What the planet says reached the surface in the window, against what the vents were dealt.
    double recorded = 0, recordedLand = 0;
    long landCells = 0;
    for (size_t cell = 0; cell < planet.columns.size(); ++cell) {
        const bool land = planet.elevation.cells[cell] > planet.seaLevel;
        landCells += land;
        if (planet.time <= window.start) continue;
        const double volume = std::max(0.0, double(planet.columns.cells[cell].erupted - planet.records.cells[cell].past[size_t(pastMarkIndex(window.start))].erupted)) * PlanetCellArea;
        recorded += volume;
        if (land) recordedLand += volume;
    }
    long meltLand = 0, nowLand = 0, sources[3] = {};
    std::vector<double> perCell;
    for (int z = 0; z < PlanetCells; ++z) {
        for (int x = 0; x < PlanetCells; ++x) {
            const CrustColumn& column = planet.columns.at(x, z);
            if (planet.elevation.at(x, z) <= planet.seaLevel || planet.time <= window.start) continue;
            if (column.erupted - planet.records.at(x, z).past[size_t(pastMarkIndex(window.start))].erupted <= 0) continue;
            ++meltLand;
            ++sources[size_t(column.meltSource)];
            nowLand += double(column.meltTime) + PlanetStep >= planet.time - 1;
            perCell.push_back(double(centresIn(planet, x, z).size()));
        }
    }
    std::printf("  land cells of the planet: %ld, with melt at the surface in the window: %ld (arc %ld, rift %ld, plume %ld), still rising: %ld | centres found per such cell: p10 %.0f p50 %.0f p90 %.0f max %.0f\n",
                landCells, meltLand, sources[0], sources[1], sources[2], nowLand, percentile(perCell, 0.1), percentile(perCell, 0.5), percentile(perCell, 0.9), percentile(perCell, 0.9999));
    std::vector<Row> rows;
    double dealt = 0;
    int land = 0, active = 0, activeLand = 0, standing = 0, byMagma[2] = {};
    for (const Vent& vent : vents) {
        Row row;
        row.vent = &vent;
        row.land = samplePlanet(planet.elevation, planetPoint(setting.seed, vent.centre), Kernel::Linear) > planet.seaLevel;
        row.last = vent.until - (vent.episodes.back().time + RegionStep);
        dealt += vent.erupted;
        Eruption coming;
        if (nextEruption(setting.seed, vent, coming)) row.next = coming.time - vent.until, row.nextVolume = coming.volume;
        land += row.land, active += vent.supply > 0, activeLand += row.land && vent.supply > 0, standing += row.land && row.last <= Standing;
        if (row.land) ++byMagma[size_t(vent.magma)];
        rows.push_back(row);
    }
    std::printf("volcanic survey  seed %llu  age %.3g yr  window %.0f to %.0f Myr\n", static_cast<unsigned long long>(setting.seed), setting.age, window.start / 1e6, window.end / 1e6);
    std::printf("  the planet's record: %.0f km3 reached the surface in the window (%.0f km3 on today's land); dealt to %zu vents: %.0f km3 (%.1f %% of the record)\n", recorded / Km3,
                recordedLand / Km3, vents.size(), dealt / Km3, 100 * dealt / std::max(1.0, recorded));
    std::printf("  vents on land: %d (basalt %d, andesite %d), erupted within %.0f Myr: %d, supplied now: %d (all vents supplied now: %d)\n", land, byMagma[0], byMagma[1], Standing / 1e6,
                standing, activeLand, active);
    const double landArea = double(landCells) * PlanetCellArea;
    std::printf("  land %.2e km2: one standing edifice per %.0f km2, one supplied vent per %.0f km2 (Earth: about 1 350 vents active in the last 10 000 yr on 1.49e8 km2, one per 110 000 km2)\n",
                landArea / 1e6, landArea / 1e6 / std::max(1, standing), landArea / 1e6 / std::max(1, activeLand));

    // Spacing along active belts, and how far a place on land is from a volcano.
    std::vector<double> spacing, toEdifice, toActive;
    for (const Row& row : rows) {
        if (!row.land || row.vent->supply <= 0) continue;
        double best = 1e30;
        for (const Row& other : rows)
            if (&other != &row && other.land && other.vent->supply > 0)
                best = std::min(best, std::hypot(wrapDelta(other.vent->centre.x - row.vent->centre.x, PlanetSpan), wrapDelta(other.vent->centre.z - row.vent->centre.z, PlanetSpan)));
        if (best < 1e29) spacing.push_back(best / 1000);
    }
    const uint64_t draw = stream(setting.seed, 0x53555256);
    for (int k = 0, found = 0; found < 3000 && k < 200000; ++k) {
        const Vec2 place{unit(draw, k, 1) * PlanetSpan, unit(draw, k, 2) * PlanetSpan};
        if (samplePlanet(planet.elevation, planetPoint(setting.seed, place), Kernel::Linear) <= planet.seaLevel) continue;
        ++found;
        toEdifice.push_back(nearest(place, rows, false) / 1000);
        toActive.push_back(nearest(place, rows, true) / 1000);
    }
    std::printf("  between neighbouring supplied vents on land, km: p10 %.0f  p50 %.0f  p90 %.0f\n", percentile(spacing, 0.1), percentile(spacing, 0.5), percentile(spacing, 0.9));
    std::printf("  from a random place on land, km: to the nearest standing edifice p10 %.0f p50 %.0f p90 %.0f | to the nearest supplied vent p10 %.0f p50 %.0f p90 %.0f\n",
                percentile(toEdifice, 0.1), percentile(toEdifice, 0.5), percentile(toEdifice, 0.9), percentile(toActive, 0.1), percentile(toActive, 0.5), percentile(toActive, 0.9));

    // What is coming.
    std::vector<double> wait, size;
    int within[4] = {};
    double perYear = 0;
    for (const Row& row : rows) {
        if (!row.land || row.next > 1e29) continue;
        wait.push_back(row.next), size.push_back(row.nextVolume / Km3);
        perYear += row.vent->supply / meanEruption(row.vent->magma);
        within[0] += row.next <= 10, within[1] += row.next <= 100, within[2] += row.next <= 1000, within[3] += row.next <= 10000;
    }
    std::printf("  next eruption of the supplied vents on land, yr from now: p10 %.0f  p50 %.0f  p90 %.0f | within 10 yr: %d, 100 yr: %d, 1 000 yr: %d, 10 000 yr: %d | its volume, km3: p50 %.3f p90 %.3f\n",
                percentile(wait, 0.1), percentile(wait, 0.5), percentile(wait, 0.9), within[0], within[1], within[2], within[3], percentile(size, 0.5), percentile(size, 0.9));
    std::printf("  eruptions on land per year, in the mean: %.3f (Earth: 50 to 70 a year on 2.7 times this land)\n", perYear);

    const Vec2 landing = findVale(planet, {0, 0});
    const Row* first = nullptr;
    for (const Row& row : rows)
        if (row.land && row.next < 1e29 && length(row.vent->centre - landing) < 400e3 && (!first || row.next < first->next)) first = &row;
    std::printf("  the first view opens near X %.0f Z %.0f: nearest standing edifice %.0f km away, nearest supplied vent %.0f km", landing.x, landing.z, nearest(landing, rows, false) / 1000,
                nearest(landing, rows, true) / 1000);
    if (first)
        std::printf("; the first eruption within 400 km comes in %.0f yr at X %.0f Z %.0f, %.0f km away: %.3f km3 of %s\n", first->next, first->vent->centre.x, first->vent->centre.z,
                    length(first->vent->centre - landing) / 1000, first->nextVolume / Km3, first->vent->magma == Magma::Basalt ? "basalt" : "andesite");
    else std::printf("; no supplied vent within 400 km\n");

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.vent->erupted > b.vent->erupted; });
    for (const Magma magma : {Magma::Andesite, Magma::Basalt}) {
        std::printf("  largest %s vents on land (X, Z of the centre in m):\n", magma == Magma::Basalt ? "basalt" : "andesite");
        int listed = 0;
        for (const Row& row : rows) {
            const Vent& vent = *row.vent;
            if (!row.land || vent.magma != magma || listed++ >= int(arguments.number("list", 8))) continue;
            std::printf("    X %.0f Z %.0f  %.0f km3 in the window, last %.1f Myr ago, supplied now %.2f km3/kyr, %zu conduits, next eruption in %.0f yr\n", vent.centre.x, vent.centre.z,
                        vent.erupted / Km3, row.last / 1e6, vent.supply * 1000 / Km3, vent.feeders.size(), row.next > 1e29 ? -1.0 : row.next);
        }
    }
    return 0;
}
