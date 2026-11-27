#include "atmosphere/type/Atmosphere.hpp"
#include "climate/type/Climate.hpp"
#include "core/service/Workers.hpp"
#include "planet/type/Planet.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double Era = 4.6e9;
constexpr int Coast = AirCells / 2;    // air column where the land begins
constexpr int Ridge = Coast + 16;      // first of the four air columns of the ridge

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Atmosphere: " << message << " (" << value << ")\n";
    std::exit(1);
}

Calendar onDay(double day) { return {Era, day * DaySeconds}; }

// Ocean on the western half, land on the eastern half, and a ridge from north to south a little
// inland: where the westerlies blow, the ridge stands downwind of the sea.
Planet coastPlanet(double surfaceWater) {
    Planet planet;
    planet.seed = 42;
    planet.time = Era;
    planet.seaLevel = 0;
    planet.surfaceWater = surfaceWater;
    planet.elevation = Field<float>(PlanetCells, PlanetCells);
    planet.precipitation = Field<float>(PlanetCells, PlanetCells, 0.8f);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x)
            planet.elevation.at(x, z) = x < 2 * Coast ? -3000.0f : x >= 2 * Ridge && x < 2 * Ridge + 8 ? 2600.0f : 150.0f;
    return planet;
}

bool same(const Atmosphere& a, const Atmosphere& b) {
    bool equal = a.step == b.step && a.ocean == b.ocean && a.runoff == b.runoff && a.zonalTemperature == b.zonalTemperature &&
                 a.snowLadder.cells == b.snowLadder.cells && a.snowCover.cells == b.snowCover.cells;
    for (const auto field : {&Atmosphere::vorticity, &Atmosphere::vorticityBefore, &Atmosphere::stream, &Atmosphere::eddy,
                             &Atmosphere::windX, &Atmosphere::windZ, &Atmosphere::airTemperature, &Atmosphere::upperTemperature,
                             &Atmosphere::surfaceTemperature, &Atmosphere::soil, &Atmosphere::snow, &Atmosphere::rainRate,
                             &Atmosphere::snowRate, &Atmosphere::shade, &Atmosphere::lift, &Atmosphere::glide})
        equal = equal && (a.*field).cells == (b.*field).cells;
    for (const auto layers : {&Atmosphere::vapour, &Atmosphere::liquid, &Atmosphere::ice, &Atmosphere::falling})
        for (size_t band = 0; band < AirBands; ++band) equal = equal && (a.*layers)[band].cells == (b.*layers)[band].cells;
    return equal;
}

bool westerly(const Atmosphere& air, int z) { return std::abs(air.latitude[size_t(z)]) > 35 && std::abs(air.latitude[size_t(z)]) < 55; }

// Mean over the rows where the wind comes from the sea, for air columns [from, to).
double westerlyMean(const Atmosphere& air, const AirField& field, int from, int to) {
    double sum = 0, cells = 0;
    for (int z = 0; z < AirCells; ++z)
        for (int x = from; westerly(air, z) && x < to; ++x) sum += field.at(x, z), cells += 1;
    return sum / cells;
}

// Mean between two northern latitudes of the pack on one rung, or of the snow line, over the air columns [from, to).
double rowsMean(const Atmosphere& air, double south, double north, int rung, int from = 0, int to = AirCells) {
    double sum = 0, cells = 0;
    for (int z = 0; z < AirCells; ++z)
        for (int x = from; air.latitude[size_t(z)] > south && air.latitude[size_t(z)] < north && x < to; ++x)
            sum += rung < 0 ? snowLineOf(air, x, z) : air.snowLadder.at(x, z)[size_t(rung)], cells += 1;
    return sum / cells;
}

// Mean over the land east of the ridge between two northern latitudes.
double northernLand(const Atmosphere& air, const AirField& field, double south, double north) {
    double sum = 0, cells = 0;
    for (int z = 0; z < AirCells; ++z)
        for (int x = Ridge + 8; air.latitude[size_t(z)] > south && air.latitude[size_t(z)] < north && x < AirCells - 4; ++x) sum += field.at(x, z), cells += 1;
    return sum / cells;
}

void expectSound(const Atmosphere& air) {
    double variance = 0;
    for (int z = 0; z < AirCells; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = air.ground.index(x, z);
            const double speed = length(bandWind(air, High, x, z)), turning = air.vorticity.cells[cell];
            expect(std::isfinite(speed) && speed < 110, "the wind left its physical range", speed);
            expect(air.airTemperature.cells[cell] > -90 && air.airTemperature.cells[cell] < 60, "the air temperature left its physical range", air.airTemperature.cells[cell]);
            expect(air.upperTemperature.cells[cell] > -120 && air.upperTemperature.cells[cell] < 30, "the upper air left its physical range", air.upperTemperature.cells[cell]);
            for (size_t band = 0; band < AirBands; ++band)
                expect(air.vapour[band].cells[cell] >= 0 && air.liquid[band].cells[cell] >= 0 && air.ice[band].cells[cell] >= 0 &&
                       air.falling[band].cells[cell] >= 0 && std::isfinite(air.vapour[band].cells[cell]), "water in the air is negative or not finite");
            expect(air.soil.cells[cell] >= 0 && air.snow.cells[cell] >= 0, "water on the ground is negative");
            variance += turning * turning;
        }
    }
    variance /= double(AirCells) * AirCells;
    expect(variance > 1e-12 && variance < 1e-8, "the weather systems died out or blew up", variance);
    expect(air.ocean > 0 && air.runoff >= 0, "the ocean or the runoff went negative");
}

void testDeterminism() {
    const Planet planet = coastPlanet(1.2e18);
    Workers two(2), five(5);
    Atmosphere once = startAtmosphere(planet, onDay(150), two);
    Atmosphere again = startAtmosphere(planet, onDay(150), five);
    expect(same(once, again), "two starts differ");
    advanceAtmosphere(once, onDay(153), two);
    for (double day = 150.25; day <= 153; day += 0.25) advanceAtmosphere(again, onDay(day), five);
    expect(same(once, again), "the state depends on the thread count or on how the way was cut into calls");
}

// A planet without surface water has no weather to speak of, however long it runs.
void testDryPlanet(Workers& workers) {
    Atmosphere air = startAtmosphere(coastPlanet(0.0), onDay(0), workers);
    advanceAtmosphere(air, onDay(5), workers);
    expect(total(waterLedger(air)) == 0 && air.precipitation == 0 && air.evaporation == 0, "a dry planet has water in its air", total(waterLedger(air)));
}

// Two years over the coast: conservation, stability, seasons, the snow line, the ridge and the loss of the sea.
void testYears(Workers& workers) {
    const Planet planet = coastPlanet(1.2e18);
    Atmosphere air = startAtmosphere(planet, onDay(0), workers);
    const double water = total(waterLedger(air));
    expect(std::abs(water - planet.surfaceWater * 1000.0) < 1e-9 * water, "the air did not borrow its first water from the ocean");
    // The ocean is a hundred thousand times the rest, so the ledger is held to a share of the water that moves.
    const double closed = 1e-7 * (water - waterLedger(air).ocean);

    AirField fallen(AirCells, AirCells), lowCloud(AirCells, AirCells);
    double winter = 0, summer = 0, winterSnow = 0, summerSnow = 0;
    const int days = 2 * 365, highRung = 6;   // 2500 m, which the ridge has ground at
    double lowWinter = 0, lowSummer = 0, highWinter = 0, highSummer = 0, lineWinter = 0, lineSummer = 0, warmLine = 0, coldLine = 0;
    // The ridge between 58° and 66° north: its ground at the high rung stays frozen all year.
    const auto ridgePack = [&] {
        double sum = 0, cells = 0;
        for (int z = 0; z < AirCells; ++z)
            for (int x = Ridge + 1; air.latitude[size_t(z)] > 58 && air.latitude[size_t(z)] < 66 && x < Ridge + 3; ++x) {
                expect(air.snowArea.at(x, z)[size_t(highRung)] > 0.1, "the ridge has no ground at its high rung", air.snowArea.at(x, z)[size_t(highRung)]);
                sum += air.snowLadder.at(x, z)[size_t(highRung)], cells += 1;
            }
        return sum / cells;
    };
    for (int day = 1; day <= days; ++day) {
        advanceAtmosphere(air, onDay(day), workers);
        for (size_t cell = 0; cell < fallen.size(); ++cell) {
            fallen.cells[cell] += air.rainRate.cells[cell] + air.snowRate.cells[cell];
            lowCloud.cells[cell] += air.liquid[Low].cells[cell] + air.ice[Low].cells[cell];
        }
        if (day % 30 == 0 || day == 300) expectSound(air);
        if (day == 60) expect(std::abs(total(waterLedger(air)) - water) < closed, "the water ledger does not close after 60 days", total(waterLedger(air)) - water);
        if (day == 365 + 30) {
            winter = northernLand(air, air.airTemperature, 40, 55), winterSnow = northernLand(air, air.snow, 50, 62);
            lowWinter = rowsMean(air, 38, 46, 0, Ridge + 8, AirCells - 4), highWinter = ridgePack(), lineWinter = rowsMean(air, 40, 50, -1);
            warmLine = rowsMean(air, 10, 20, -1), coldLine = rowsMean(air, 55, 65, -1);
        }
        if (day == 365 + 215) {
            summer = northernLand(air, air.airTemperature, 40, 55), summerSnow = northernLand(air, air.snow, 50, 62);
            lowSummer = rowsMean(air, 38, 46, 0, Ridge + 8, AirCells - 4), highSummer = ridgePack(), lineSummer = rowsMean(air, 40, 50, -1);
        }
    }
    const WaterLedger ledger = waterLedger(air);
    expect(std::abs(total(ledger) - water) < closed, "the water ledger does not close after two years", total(ledger) - water);
    expect(ledger.vapour > 0 && ledger.cloudLiquid > 0 && ledger.cloudIce > 0 && ledger.soil > 0 && ledger.snow > 0, "a water store stayed empty");
    expect(std::abs(oceanVolume(air) * 1000.0 - ledger.ocean) < 1e-12 * ledger.ocean, "the ocean volume is not the ledger's ocean");
    expect(summer - winter > 8 && summer - winter < 50, "mid-latitude summer is not plausibly warmer than winter", summer - winter);
    expect(winterSnow > 20 && summerSnow < 0.2 * winterSnow, "snow does not come with winter and go with summer", winterSnow);

    // The ladder: a cold high rung keeps its snow all year, the sea-level rung of the same temperate
    // rows is bare in summer, and the snow line follows the season and the latitude.
    expect(highWinter > 100 && highSummer > 100, "the cold ridge lost the snow of its high ground", highSummer);
    expect(lowWinter > 5 && lowSummer < 0.01 * lowWinter, "the sea-level rung of the temperate plain is not snowy in winter and bare in summer", lowSummer);
    expect(lineSummer > lineWinter + 500, "the snow line does not rise in summer", lineSummer - lineWinter);
    expect(warmLine > coldLine + 1000, "the snow line is not higher in warm latitudes", warmLine - coldLine);

    // Windward of the ridge against its lee, where the wind comes from the sea.
    const double windward = westerlyMean(air, fallen, Ridge - 2, Ridge + 2), lee = westerlyMean(air, fallen, Ridge + 5, Ridge + 9);
    expect(windward > 1.3 * lee, "the windward side of the ridge is not wetter than its lee", windward / lee);
    expect(westerlyMean(air, lowCloud, Ridge - 2, Ridge + 2) > 1.2 * westerlyMean(air, lowCloud, Ridge + 5, Ridge + 9), "low cloud does not gather on the windward side");

    // One cloud, two grounds: rain in the valley, snow on the summit above it.
    bool found = false;
    for (int z = 0; z < AirCells && !found; ++z) {
        for (int x = 0; x < AirCells && !found; ++x) {
            const Vec2 here{(x + 0.5) * AirCell, (z + 0.5) * AirCell};
            if (air.rainRate.at(x, z) * 3600 < 0.05 || air.airTemperature.at(x, z) < 9 || air.airTemperature.at(x, z) > 15) continue;
            const WeatherSample valley = weatherAt(air, here, planet.seaLevel, onDay(days)), summit = weatherAt(air, here, planet.seaLevel + 4200, onDay(days));
            expect(valley.rain > 0 && valley.snow == 0 && summit.snow > 0 && summit.rain == 0, "valley and summit do not split one cloud into rain and snow", valley.rain);
            expect(summit.temperature < valley.temperature - 20 && valley.cloudCover > 0 && valley.humidity > 0, "the weather sample is inconsistent");
            const WeatherReport report = inspectWeather(air, here, planet.seaLevel, onDay(days));
            expect(!report.cause.empty() && report.clouds.cover > 0 && report.pressure > 9e4 && report.profile.height[AirBands] > 4000, "the inspector has nothing to say");
            found = true;
        }
    }
    expect(found, "no rain over mild ground to sample");
    const CloudField clouds = cloudField(air);
    expect(clouds.size == AirCells && clouds.band[High].cover.size() == size_t(AirCells) * AirCells && clouds.span == PlanetSpan, "the cloud field is malformed");
    const SunDirection noon = sunDirectionAt({0, 0}, {Era, 172.5 * DaySeconds});
    expect(std::abs(noon.x * noon.x + noon.up * noon.up + noon.z * noon.z - 1) < 1e-12 && noon.up > 0.8 && noon.z > 0, "the midsummer noon sun is misplaced", noon.up);

    // The sea falls dry: evaporation from it stops at once, and the rain shrinks to what wet
    // ground and melting snow still give back to the air.
    const double rainBefore = air.precipitation;
    Field<float> dry(PlanetCells, PlanetCells, 50.0f);
    setSurface(air, dry, planet.seaLevel);
    const double oceanBefore = air.ocean;
    advanceAtmosphere(air, onDay(days + 1), workers);
    expect(air.ocean >= oceanBefore, "a sea without a surface still evaporates", air.ocean - oceanBefore);
    advanceAtmosphere(air, onDay(days + 28), workers);
    expect(air.precipitation < 0.5 * rainBefore, "rain did not shrink after the sea was lost", air.precipitation / rainBefore);
    expect(std::abs(total(waterLedger(air)) - water) < closed, "losing the sea broke the water ledger");

    // With the ground dry as well, all that is left is the water the air held that day. It has no
    // sea to run off into, so it keeps cycling between air and ground as a small remnant of rain.
    for (AirField* store : {&air.soil, &air.snow, &air.snowCover}) std::fill(store->cells.begin(), store->cells.end(), 0.0);
    for (SnowColumn& rungs : air.snowLadder.cells) rungs.fill(0.0);
    advanceAtmosphere(air, onDay(days + 28 + 42), workers);
    expect(air.precipitation < 0.1 * rainBefore, "rain did not fade after every source was lost", air.precipitation / rainBefore);
}

} // namespace

void testAtmosphere() {
    // Two threads: a 128 x 128 grid stepped hour by hour gains nothing from more, and waking a
    // whole pool every step costs more than the step. testDeterminism compares other counts.
    Workers workers(2);
    testDeterminism();
    testDryPlanet(workers);
    testYears(workers);
}
