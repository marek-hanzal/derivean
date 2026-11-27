#include "Measure.hpp"
#include "MeasureSphere.hpp"
#include "ProcessUsage.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/EddyBand.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereSeason.hpp"
#include "atmosphere/fn/SphereStencil.hpp"
#include "chronology/type/Calendar.hpp"
#include "climate/fn/Seasons.hpp"
#include "core/fn/Hash.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/constant/Scale.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using namespace derivean::sim;

namespace {

// The flat world's zonal temperature as a profile from pole to pole (one half of its rows, latitude
// falling with Z), at sea level on a fraction of the year, and the annual mean: the bands'
// prescribed temperature (S4 plan, the runs of S4.3).
struct Profile {
    std::vector<double> latitude, annual;
    std::vector<ZonalClimate> zone;
};

Profile flatProfile(uint64_t seed, double age) {
    constexpr int Samples = 3600;
    std::vector<std::pair<double, ZonalClimate>> rows;
    for (int i = 0; i < Samples; ++i) {
        const double z = (i + 0.5) * PlanetSpan / Samples;
        if (northwardZ(z) < 0) rows.emplace_back(latitudeAt(z), zonalClimate(seed, z, age));
    }
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    Profile profile;
    for (const auto& [latitude, zone] : rows)
        if (profile.latitude.empty() || latitude < profile.latitude.back())
            profile.latitude.push_back(latitude), profile.annual.push_back(zone.temperature), profile.zone.push_back(zone);
    return profile;
}

SphereSeason seasonOn(const Profile& profile, const LatitudeBands& bands, double day) {
    std::vector<double> now(profile.zone.size());
    const double year = day / YearDays - std::floor(day / YearDays);
    for (size_t r = 0; r < now.size(); ++r) now[r] = seasonalTemperature(profile.zone[r], 0, year);
    return sphereSeasonOf(bands, day, bandProfile(bands, profile.latitude, now), bandProfile(bands, profile.latitude, profile.annual));
}

// A field of the eddy band with rms `size`: per-cell noise (the cell's index scrambled) through the band.
std::vector<double> bandNoise(const SphereGrid& grid, EddyBand band, double size) {
    std::vector<double> noise(size_t(grid.cells)), banded;
    for (int c = 0; c < grid.cells; ++c) noise[size_t(c)] = 2 * unit(scramble(uint64_t(c))) - 1;
    filterEddyBand(band, noise, banded);
    double square = 0, area = 0;
    for (int c = 0; c < grid.cells; ++c) square += grid.area[size_t(c)] * banded[size_t(c)] * banded[size_t(c)], area += grid.area[size_t(c)];
    const double scale = size / std::sqrt(square / area);
    for (double& value : banded) value *= scale;
    return banded;
}

} // namespace

// terra measure sphere [--level 6] [--days 60] [--forced 1] [--band km] [--every s] [--tolerance 1e-9]
//                      [--start day] [--noise 1/s] [--snap prefix] [--seed n] [--age years]
// terra measure sphere --compare a.snap --with b.snap
// S4.3e's runs (Codex DREV-CLAUDE-010-03): the sphere's flow on one thread from the season's jet of
// day `start` with band noise, its eddies started from that ζ (startEddies); forced, the closures of
// the flat world's prescribed temperature with the eddy band of `band` km (the default factory's
// SphereEddyWavelength without it) refreshed every `every` s, jet and growth set at each day's start;
// unforced, the same start left alone. Each day one line of diagnostics (printSphereDay) and the
// day's steps, Courant number, refusals and instructions (cycles on Windows); snapshots of ζ and the
// fluxes on days 1, 5, 10, 20, 30, 45, 60 and 90 for --compare.
int measureSphere(const Arguments& arguments) {
    if (arguments.values.count("compare")) return compareSphereSnapshots(arguments.text("compare", ""), arguments.text("with", ""));
    const int level = int(arguments.number("level", 6)), days = int(arguments.number("days", 60));
    const bool forced = arguments.number("forced", 1) != 0;
    const double wavelength = arguments.number("band", SphereEddyWavelength / 1000) * 1000, every = arguments.number("every", SphereEddyEvery);
    const double start = arguments.number("start", 0), noise = arguments.number("noise", 1e-6);
    const std::string snap = arguments.text("snap", "");

    const SphereGrid grid = makeSphereGrid(level);
    SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
    closures.band = makeEddyBand(grid, wavelength, SphereEddyReach);
    closures.eddyEvery = every;
    SphereProbe probe{closures.bands, closures.band, makeSpherePoisson(grid, stencil), {}, {}, {}};
    releaseEdgeTables(stencil);   // the Poissons are made; the steps read none of them
    const Profile profile = flatProfile(arguments.seed(), arguments.number("age", 4.6e9));

    SphereSeason season = seasonOn(profile, closures.bands, start);
    std::vector<double> vorticity;
    spreadBands(closures.bands, season.jet, vorticity);
    const std::vector<double> seed = bandNoise(grid, closures.band, noise);
    for (size_t c = 0; c < vorticity.size(); ++c) vorticity[c] += seed[c];
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    flow.tolerance = arguments.number("tolerance", flow.tolerance);
    if (!setVorticity(flow, grid, stencil, poisson, vorticity)) {
        std::fprintf(stderr, "the start did not invert (residual %.2e)\n", flow.inversion.residual);
        return 3;
    }
    closures.jet = season.jet, closures.growth = season.growth;
    startEddies(closures, flow);
    std::printf("sphere L%d (%d cells) %s  band %.0f km (degrees %d to %d)  every %.0f s  tolerance %.0e  start day %.1f  noise %.1e\n", level,
                grid.cells, forced ? "forced" : "unforced", wavelength / 1000, closures.band.lowest, closures.band.highest, every, flow.tolerance,
                start, noise);
    printSphereDay(probe, grid, flow, 0);

    SphereHour hour;
    const SphereForcing none;
    const int snapDays[] = {1, 5, 10, 20, 30, 45, 60, 90};
    long steps = 0, refusals = 0, allSteps = 0, allRefusals = 0;
    double courant = 0, worst = 0;
    const Usage began = usageNow();
    Usage dayStart = began;
    for (int h = 1; h <= 24 * days; ++h) {
        if (forced && (h - 1) % 24 == 0 && h > 1) {
            season = seasonOn(profile, closures.bands, start + (h - 1) / 24);
            closures.jet = season.jet, closures.growth = season.growth;
        }
        const bool taken = forced ? advanceClosedHour(hour, flow, grid, stencil, poisson, closures)
                                  : advanceSphereHour(hour, flow, grid, stencil, poisson, none);
        refusals += long(hour.refusals.size());
        if (!taken) {
            std::printf("hour %d refused:", h);
            for (const HourTry& t : hour.refusals) std::printf(" (reason %d, %d steps, at %d)", int(t.reason), t.steps, t.at);
            std::printf("\n");
            return 4;
        }
        steps += std::lround(hour.window / flow.step);
        courant = std::max(courant, hour.courant);
        if (h % 24) continue;
        const Usage now = usageNow();
        const int day = h / 24;
        std::printf("work %3d  steps/h %.2f  courant %.3f  refusals %ld  CG %d  instructions %.4g  cycles %.4g\n", day, steps / 24.0, courant, refusals,
                    flow.inversion.iterations, now.instructions - dayStart.instructions, now.cycles - dayStart.cycles);
        printSphereDay(probe, grid, flow, day);
        for (int d : snapDays)
            if (d == day && !snap.empty()) writeSphereSnapshot(snap + "-" + std::to_string(d) + ".snap", flow);
        std::fflush(stdout);
        allSteps += steps, allRefusals += refusals, worst = std::max(worst, courant);
        steps = 0, refusals = 0, courant = 0, dayStart = now;
    }
    const Usage now = usageNow();
    std::printf("summary L%d %s band %.0f km every %.0f s tolerance %.0e: %d days, %.2f steps/h, worst Courant %.3f, refusals %ld, "
                "instructions %.4g a day, cycles %.4g a day\n",
                level, forced ? "forced" : "unforced", wavelength / 1000, every, flow.tolerance, days, allSteps / (24.0 * std::max(days, 1)), worst, allRefusals,
                (now.instructions - began.instructions) / std::max(days, 1), (now.cycles - began.cycles) / std::max(days, 1));
    return 0;
}
