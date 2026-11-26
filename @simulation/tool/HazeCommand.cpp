#include "Arguments.hpp"

#include "atmosphere/fn/Kernels.hpp"
#include "climate/type/Climate.hpp"
#include "core/service/Workers.hpp"
#include "planet/type/Planet.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

using namespace derivean::sim;

namespace {

constexpr const char* KindNames[HazeKinds] = {"dust", "salt", "sulphate", "organics"};
constexpr double Tg = 1e9;   // kg

// One column at one time: what the haze is made of and how far one sees through it.
struct Moment {
    double day{}, x{}, z{}, humidity{}, ground{}, rainBefore{}, plants{}, water{}, dryHours{};
    double daily{}, celsius{};   // the low band's humidity at the daily mean temperature; the air's °C at the ground now
    std::array<double, HazeKinds> depth{};
    double total{}, dry{}, low{}, high{};   // optical depth swollen and dry; view in km at the ground and 3 km up
};

void print(const char* name, const Moment& m) {
    std::printf("%-12s day %6.2f at x %8.0f z %8.0f  ground %5.0f m  water %.2f  plants %.2f  %.0f °C  RH %3.0f %% (daily mean %3.0f %%)\n", name,
                m.day, m.x, m.z, m.ground, m.water, m.plants, m.celsius, 100 * m.humidity, 100 * m.daily);
    std::printf("             rain in the 5 h before %.1f mm, dry for %.0f h\n", m.rainBefore, m.dryHours);
    std::printf("             AOD %.3f =", m.total);
    for (int kind = 0; kind < HazeKinds; ++kind) std::printf("  %s %.3f (%2.0f %%)", KindNames[kind], m.depth[size_t(kind)], m.total > 0 ? 100 * m.depth[size_t(kind)] / m.total : 0.0);
    std::printf("\n             dry (no water taken up) %.3f, so humidity multiplies it by %.2f\n", m.dry, m.dry > 0 ? m.total / m.dry : 0.0);
    std::printf("             view at the ground %.0f km, at 3 km %.0f km\n", m.low, m.high);
}

// The cases sought, each the median among the columns that fit it, so that none is an extreme.
struct Case {
    const char* name;
    std::vector<Moment> found;
};

const Moment& median(std::vector<Moment>& moments) {
    std::sort(moments.begin(), moments.end(), [](const Moment& a, const Moment& b) { return a.total < b.total; });
    return moments[moments.size() / 2];
}

} // namespace

// terra haze --seed n --age years --day d --days n: the haze of the weather over `days` days from
// day d: budgets against Earth, the cases after rain, humid and dusty, and the cost of a step.
int hazeCommand(const Arguments& arguments) {
    const double age = arguments.number("age", 4.6e9), first = arguments.number("day", 0), days = arguments.number("days", 365);
    Workers workers(unsigned(arguments.number("threads", 0)));
    const Planet planet = evolvePlanet(arguments.seed(), age, workers);
    Atmosphere air = startAtmosphere(planet, Calendar{age, first * DaySeconds}, workers);
    const size_t cells = air.ground.size();
    const HazeLedger start = air.haze.totals;

    // Rain of the last six steps per cell, to tell air just washed.
    std::vector<std::array<double, 6>> recent(cells);
    std::vector<double> sumDepth(cells, 0.0), mostDepth(cells, 0.0);
    std::vector<std::array<double, HazeKinds>> sumKinds(cells);
    std::vector<double> sumRain(cells, 0.0), sumHumidity(cells, 0.0), dryHours(cells, 0.0);
    std::array<double, 20> humidities{};   // dry low land by relative humidity, 5 % bins
    std::array<Case, 4> cases{Case{"after rain", {}}, Case{"humid green", {}}, Case{"dusty day", {}}, Case{"humid warm", {}}};
    double hazeSeconds = 0, stepSeconds = 0;
    int samples = 0;   // six-hourly
    const int64_t end = air.step + int64_t(days * AirStepsPerDay);
    while (air.step < end) {
        // The step as the atmosphere runs it, and the haze's part timed on its own over a copy.
        const bool timing = air.step % AirStepsPerDay == 0;
        Atmosphere probe = timing ? air : Atmosphere{};
        auto clock = std::chrono::steady_clock::now();
        stepAtmosphere(air, workers);
        stepSeconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - clock).count();
        if (timing) {
            // The fastest of a few runs over the same state, the caches as warm as in the atmosphere's own step.
            double fastest = 1e9;
            for (int run = 0; run < 4; ++run) {
                Atmosphere again = probe;
                clock = std::chrono::steady_clock::now();
                stepHaze(again, workers);
                fastest = std::min(fastest, std::chrono::duration<double>(std::chrono::steady_clock::now() - clock).count());
            }
            hazeSeconds += fastest;
        }
        for (size_t cell = 0; cell < cells; ++cell) {
            auto& rain = recent[cell];
            std::rotate(rain.begin(), rain.begin() + 1, rain.end());
            rain.back() = (air.rainRate.cells[cell] + air.snowRate.cells[cell]) * AirStep;
            dryHours[cell] = rain.back() > 0.1 ? 0.0 : dryHours[cell] + AirStep / 3600.0;
        }
        // Means every six hours; the cases between 9 and 15 h, when a picture of them can be taken.
        const int64_t hour = air.step % AirStepsPerDay;
        const bool sampled = air.step % 6 == 0, daylight = hour >= 9 && hour <= 15;
        if (!sampled && !daylight) continue;
        samples += sampled;
        for (int z = 0; z < AirCells; ++z)
            for (int x = 0; x < AirCells; ++x) {
                const size_t cell = air.ground.index(x, z);
                Moment m;
                m.day = double(air.step) * AirStep / DaySeconds;
                m.x = (x + 0.5) * AirCell, m.z = (z + 0.5) * AirCell;
                m.depth = hazeDepths(air, x, z);
                for (double d : m.depth) m.total += d;
                m.ground = air.ground.cells[cell];
                m.water = air.openWater.cells[cell];
                m.plants = air.haze.plants.cells[cell];
                const AirColumn column = columnAt(air, x, z);
                const ColumnProfile profile = profileOf(column);
                m.humidity = surfaceHumidity(column, profile);
                m.daily = std::min(1.0, column.vapour[Low] * profile.perCapacity[Low]);
                m.celsius = column.airTemperature - LapseRate * column.ground + column.surfaceWarmth;
                m.dryHours = dryHours[cell];
                if (sampled) {
                    sumDepth[cell] += m.total;
                    for (int kind = 0; kind < HazeKinds; ++kind) sumKinds[cell][size_t(kind)] += m.depth[size_t(kind)];
                    mostDepth[cell] = std::max(mostDepth[cell], m.total);
                    sumHumidity[cell] += m.humidity;
                    sumRain[cell] += (air.rainRate.cells[cell] + air.snowRate.cells[cell]) * DaySeconds;
                }
                for (int k = 0; k < 5; ++k) m.rainBefore += recent[cell][size_t(k)];
                const bool rainingNow = recent[cell].back() > 0.01, lowLand = m.water < 0.5 && m.ground < 300;
                if (!lowLand || !daylight) continue;
                for (double d : hazeDepths(air, x, z, false)) m.dry += d;
                m.low = viewDistance(air, {m.x, m.z}, m.ground) / 1000;
                m.high = viewDistance(air, {m.x, m.z}, 3000.0) / 1000;
                if (!rainingNow && m.rainBefore > 1.0) cases[0].found.push_back(m);
                // Humid continental air: green land, the hour's humidity over 80 %, three days without rain.
                if (!rainingNow && m.plants > 0.6 && m.humidity > 0.8 && m.dryHours >= 72) {
                    cases[1].found.push_back(m);
                    if (m.celsius > 15) cases[3].found.push_back(m);   // the same in the growing season
                }
                if (!rainingNow && m.rainBefore < 0.05) humidities[std::min<size_t>(19, size_t(m.humidity * 20))] += 1;
                if (m.humidity < 0.4 && m.depth[MineralDust] > 0.5 * m.total && m.total > 0.1) cases[2].found.push_back(m);
            }
    }

    const double steps = days * AirStepsPerDay, years = days / YearDays;
    std::printf("seed %llu, days %.0f..%.0f: %.1f ms per atmosphere step, of which the haze %.2f ms (%u workers)\n",
                static_cast<unsigned long long>(arguments.seed()), first, first + days, 1000 * stepSeconds / steps,
                1000 * hazeSeconds / std::max(1.0, std::floor(steps / AirStepsPerDay)), workers.threads());

    // Budgets per year, and per unit area against Earth's.
    double sea = 0, land = 0;
    for (size_t cell = 0; cell < cells; ++cell) sea += air.openWater.cells[cell] * AirCellArea, land += (1 - air.openWater.cells[cell]) * AirCellArea;
    std::printf("planet: land %.2e km², sea %.2e km² (Earth 1.49e8 and 3.61e8)\n", land / 1e6, sea / 1e6);
    for (int kind = 0; kind < HazeKinds; ++kind) {
        const double emitted = air.haze.totals.emitted[size_t(kind)] - start.emitted[size_t(kind)];
        const double washed = air.haze.totals.washed[size_t(kind)] - start.washed[size_t(kind)];
        const double settled = air.haze.totals.settled[size_t(kind)] - start.settled[size_t(kind)];
        double aloft = 0;
        for (double mass : air.haze.mass[size_t(kind)].cells) aloft += mass * AirCellArea;
        const double lifetime = emitted > 0 ? aloft / (emitted / (days * DaySeconds)) / DaySeconds : 0;
        std::printf("  %-9s emitted %8.2f Tg/yr  washed %3.0f %%  settled %3.0f %%  aloft now %.3f Tg  lifetime %.1f days\n", KindNames[kind],
                    emitted / Tg / years, emitted > 0 ? 100 * washed / emitted : 0.0, emitted > 0 ? 100 * settled / emitted : 0.0, aloft / Tg, lifetime);
    }
    std::printf("  Earth per area: dust ~1500 Tg/yr of land, SOA 12-70 Tg/yr, DMS sulphur 28 Tg S/yr (×0.4 aerosol ×4.12 as ammonium sulphate) of sea\n");

    // Mean optical depth by surface.
    struct Region {
        const char* name;
        double sum{}, most{}, count{}, rain{}, humidity{}, planetRain{};
        std::array<double, HazeKinds> kinds{};
    };
    std::array<Region, 5> regions{Region{"open sea"}, Region{"coast"}, Region{"desert (plants < 0.1)"}, Region{"steppe (0.1-0.4)"}, Region{"green (> 0.6)"}};
    for (size_t cell = 0; cell < cells; ++cell) {
        const double water = air.openWater.cells[cell], plants = air.haze.plants.cells[cell], mean = sumDepth[cell] / std::max(1, samples);
        const int region = water > 0.95 ? 0 : water > 0.3 ? 1 : plants < 0.1 ? 2 : plants < 0.4 ? 3 : plants > 0.6 ? 4 : -1;
        if (region < 0) continue;
        regions[size_t(region)].sum += mean, regions[size_t(region)].count += 1;
        regions[size_t(region)].rain += sumRain[cell] / std::max(1, samples);
        regions[size_t(region)].humidity += sumHumidity[cell] / std::max(1, samples);
        for (int kind = 0; kind < HazeKinds; ++kind) regions[size_t(region)].kinds[size_t(kind)] += sumKinds[cell][size_t(kind)] / std::max(1, samples);
        regions[size_t(region)].most = std::max(regions[size_t(region)].most, mostDepth[cell]);
    }
    std::printf("mean AOD at 550 nm (highest seen in any cell):\n");
    for (const Region& region : regions) {
        std::printf("  %-22s %.3f (%.2f) over %5.0f cells:", region.name, region.count > 0 ? region.sum / region.count : 0.0, region.most, region.count);
        for (int kind = 0; kind < HazeKinds; ++kind) std::printf("  %s %2.0f %%", KindNames[kind], region.sum > 0 ? 100 * region.kinds[size_t(kind)] / region.sum : 0.0);
        std::printf("  | rain %.2f mm/day, RH %.0f %%\n", region.count > 0 ? region.rain / region.count : 0.0, region.count > 0 ? 100 * region.humidity / region.count : 0.0);
    }
    std::printf("low land without rain, 9 to 15 h, by the hour's relative humidity (5 %% bins from 0):");
    double dry = 0;
    for (double count : humidities) dry += count;
    for (double count : humidities) std::printf(" %.1f", dry > 0 ? 100 * count / dry : 0.0);
    std::printf("\n");
    std::printf("  Earth: clean maritime 0.03-0.06, continental 0.1-0.2, Sahara and Sahel 0.3-0.5 with outbreaks over 1\n");

    for (Case& found : cases) {
        if (found.found.empty()) std::printf("%-12s none found\n", found.name);
        else {
            std::printf("%-12s %zu columns, the median and the tenth from the top:\n", found.name, found.found.size());
            print(found.name, median(found.found));
            print(found.name, found.found[found.found.size() * 9 / 10]);
        }
    }
    return 0;
}
