#include "chronology/type/Calendar.hpp"
#include "climate/fn/Seasons.hpp"
#include "climate/fn/Snowpack.hpp"
#include "climate/type/Climate.hpp"
#include "climate/type/MoistureBalance.hpp"
#include "planet/type/Planet.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Climate: " << message << " (" << value << ")\n";
    std::exit(1);
}

void testCalendar() {
    const double summer = (172.5 * DaySeconds);
    expect(std::abs(solarDeclination({0, summer}) - AxialTilt) < 0.01, "the northern summer solstice is not at day 172.5");
    expect(std::abs(solarDeclination({0, -10 * DaySeconds}) + AxialTilt) < 1e-9, "the winter solstice is not ten days before day zero");
    expect(std::abs(dayLength(0, {0, summer}) - 12) < 1e-6, "the equator does not have twelve hours of daylight");
    expect(dayLength(80, {0, summer}) == 24 && dayLength(-80, {0, summer}) == 0, "the poles miss midnight sun and polar night");
    expect(dayLength(50, {0, summer}) > 15.5 && dayLength(50, {0, summer}) < 16.8, "midsummer day length at 50° is off", dayLength(50, {0, summer}));
    expect(std::abs(noonSunElevation(50, {0, summer}) - 63.4) < 0.01, "noon sun elevation is off");
    expect(std::abs(hourOfDay({0, 3 * DaySeconds + 6 * 3600.0}) - 6) < 1e-9 && std::abs(dayOfYear({0, 400 * DaySeconds}) - 35) < 1e-9, "day and hour do not wrap");
    expect(std::abs(lunarPhase({0, 0.5 * LunarMonth * DaySeconds}) - 0.5) < 1e-12, "the moon is not full half a month after new");

    // The tide only moves water: at any instant its mean around the planet is zero.
    for (double seconds : {0.0, 12345.0, 7.3 * DaySeconds, 22.1 * DaySeconds}) {
        double sum = 0, highest = 0;
        for (int x = 0; x < 512; ++x) {
            const double height = tideHeight({(x + 0.5) * PlanetSpan / 512, 3e6}, {0, seconds});
            sum += height;
            highest = std::max(highest, height);
        }
        expect(std::abs(sum / 512) < 1e-9, "the tide has a net height", sum / 512);
        expect(highest > NeapTide - 1e-3 && highest < SpringTide + 1e-9, "the tide leaves its neap to spring range", highest);
    }
    expect(std::abs(tideHeight({0, 0}, {0, 0}) - SpringTide) < 1e-9, "new moon does not raise a spring high tide on the line through sun and moon");
}

// The closed forms against a year walked day by day.
void testSeasons() {
    for (double mean : {-25.0, -9.0, -2.0, 0.5, 4.0, 12.0, 26.0}) {
        for (double halfRange : {0.0, 3.0, 12.0, 21.0}) {
            const int samples = 365 * 24;
            double below = 0, degreeDays = 0, cracking = 0;
            for (int i = 0; i < samples; ++i) {
                const double celsius = mean + halfRange * std::cos(2 * 3.141592653589793 * (i + 0.5) / samples);
                if (celsius < SnowLine) below += 1;
                degreeDays += std::max(0.0, celsius) * YearDays / samples;
                if (celsius > -8 && celsius < -3) cracking += 1;
            }
            const SnowBalance balance = annualSnowBalance(mean, halfRange, 0.8);
            expect(std::abs(balance.snowfall - 0.8 * below / samples) < 2e-3, "snowfall share differs from the walked year", balance.snowfall);
            expect(std::abs(positiveDegreeDays(mean, halfRange) - degreeDays) < 0.02 + 1e-3 * degreeDays, "degree days differ from the walked year", degreeDays);
            expect(std::abs(balance.meltPotential - degreeDays * DegreeDayMelt / WaterDensity) < 1e-3 + 1e-3 * balance.meltPotential, "melt potential is off");
            expect(balance.net == balance.snowfall - balance.meltPotential, "net snow balance is not snowfall less melt");
            expect(std::abs(frostCracking(mean, halfRange) - cracking / samples) < 2e-3, "frost cracking share differs from the walked year", cracking / samples);
        }
    }
    ZonalClimate zone;
    zone.latitude = 50;
    zone.temperature = 5;
    zone.seasonal = 12;
    expect(std::abs(seasonalTemperature(zone, 1000, 0.55) - (17 - 1000 * LapseRate)) < 1e-9, "the northern summer peak is misplaced");
    zone.latitude = -50;
    expect(std::abs(seasonalTemperature(zone, 0, 0.55) + 7) < 1e-9, "the southern winter is not opposite the northern summer");
}

void testSnowpack() {
    std::vector<float> swe{0.0f, 40.0f, 3.0f, 100.0f}, melt(4);
    const std::vector<float> before = swe, celsius{-5.0f, 6.0f, 9.0f, 1.0f}, rate{2e-4f, 1e-4f, 0.0f, 3e-4f};
    const double seconds = 6 * 3600.0;
    stepSnowpack<float>(swe, celsius, rate, seconds, melt);
    for (size_t cell = 0; cell < swe.size(); ++cell) {
        const double snowfall = rate[cell] * seconds * snowShare(celsius[cell]);
        expect(std::abs(swe[cell] - (before[cell] + snowfall - melt[cell])) < 1e-4, "the snowpack lost or made water", swe[cell]);
        expect(swe[cell] >= 0 && melt[cell] >= 0, "the snowpack went negative");
    }
    expect(swe[0] > 4.3f && melt[0] == 0, "snow did not settle in frost");
    expect(swe[2] == 0 && melt[2] == 3.0f, "a thin pack did not melt out completely");
    expect(std::abs(melt[1] - DegreeDayMelt * 6 * 0.25) < 1e-4, "melt is not degree-days", melt[1]);
}

// What the geological simulation relies on when it reads the planet's climate.
void testSteadyClimate(uint64_t seed) {
    Workers workers;
    Planet planet = evolvePlanet(seed, 1.2e9, workers);
    const MoistureBalance balance = solveMoisture(planet, workers);
    expect(std::abs(balance.precipitated - balance.evaporated) < 0.01 * balance.evaporated, "steady precipitation does not match evaporation", balance.precipitated / balance.evaporated);

    const int n = PlanetCells;
    double land = 0, landRain = 0, wettest = 0, windward = 0, windwardCells = 0, lee = 0, leeCells = 0;
    for (int z = 0; z < n; ++z) {
        const int upwind = planet.windX.at(0, z) >= 0 ? -1 : 1;
        for (int x = 0; x < n; ++x) {
            const double rain = planet.precipitation.at(x, z);
            expect(std::isfinite(rain) && rain >= 0, "precipitation is not a finite non-negative number", rain);
            expect(std::isfinite(planet.temperature.at(x, z)), "temperature is not finite");
            wettest = std::max(wettest, rain);
            if (planet.elevation.at(x, z) <= planet.seaLevel) continue;
            land += 1;
            landRain += rain;
            const bool seaUpwind = planet.elevation.at(wrapIndex(x + upwind, n), z) <= planet.seaLevel;
            const bool seaDownwind = planet.elevation.at(wrapIndex(x - upwind, n), z) <= planet.seaLevel;
            if (seaUpwind && !seaDownwind) windward += rain, windwardCells += 1;
            if (seaDownwind && !seaUpwind) lee += rain, leeCells += 1;
        }
    }
    expect(land > 0 && landRain / land > 0.3 && landRain / land < 1.2, "land-mean precipitation left 0.3 to 1.2 m/yr", landRain / land);
    expect(wettest < 4.0, "a planet cell receives more than 4 m/yr", wettest);
    expect(windward / windwardCells > 1.05 * lee / leeCells, "windward coasts are not wetter than lee coasts", (windward / windwardCells) / (lee / leeCells));

    // The same planet solved on one extra thread.
    Planet again = planet;
    Workers few(1);
    solvePlanetClimate(again, few);
    expect(again.precipitation.cells == planet.precipitation.cells, "the steady climate depends on the thread count");
}

void testDryPlanet() {
    Workers workers;
    const Planet planet = evolvePlanet(42, 30e6, workers);
    expect(planet.surfaceWater == 0, "the young planet already has surface water");
    for (float rain : planet.precipitation.cells) expect(rain == 0, "it rains on a planet without surface water", rain);
}

} // namespace

void testClimate() {
    testCalendar();
    testSeasons();
    testSnowpack();
    testSteadyClimate(42);
    testSteadyClimate(7);
    testDryPlanet();
}
