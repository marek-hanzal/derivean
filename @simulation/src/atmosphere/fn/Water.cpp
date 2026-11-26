#include "atmosphere/fn/Cover.hpp"
#include "atmosphere/fn/Kernels.hpp"

#include "climate/fn/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {
namespace {

constexpr double RunoffDelay = 12 * DaySeconds;   // s from saturated ground to the ocean
constexpr double OceanReach = 0.5;            // share of the ocean a single step may evaporate at most
constexpr double StartHumidity = 0.75;        // share of capacity the air starts with over wet surfaces
constexpr double WetYear = 1000.0;            // kg/m² of yearly precipitation that keeps ground wet all year
constexpr double WarmWater = 0.5;             // share of the sea's warmth over the air that a surface parcel starts with
constexpr double HeatYield = 0.5;             // °C the upper air warms per kg/m² condensed above the low band
constexpr double HeatLimit = 2.0;             // °C per step at most

} // namespace

AirColumn columnAt(const Atmosphere& atmosphere, int x, int z) {
    const size_t cell = atmosphere.ground.index(x, z);
    AirColumn column;
    column.airTemperature = atmosphere.airTemperature.cells[cell];
    column.upperTemperature = atmosphere.upperTemperature.cells[cell];
    column.ground = atmosphere.ground.cells[cell];
    column.climb = atmosphere.climb.cells[cell];
    column.hollow = atmosphere.hollow.cells[cell];
    column.lift = atmosphere.lift.cells[cell];
    column.glide = atmosphere.glide.cells[cell];
    const double water = atmosphere.openWater.cells[cell];
    const double sun = atmosphere.sunSwing[size_t(z)] * (1.0 - CloudDamping * atmosphere.shade.cells[cell]);
    const double warmWater = WarmWater * std::max(0.0, atmosphere.surfaceTemperature.cells[cell] - column.airTemperature);
    column.surfaceWarmth = (1.0 - water) * LandDaily * sun + water * (SeaDaily * sun + warmWater);
    for (size_t band = 0; band < AirBands; ++band) {
        column.vapour[band] = atmosphere.vapour[band].cells[cell];
        column.liquid[band] = atmosphere.liquid[band].cells[cell];
        column.ice[band] = atmosphere.ice[band].cells[cell];
        column.falling[band] = atmosphere.falling[band].cells[cell];
    }
    return column;
}

// Starting stores from the planet's steady climate: humid air over wet surfaces, ground as wet as
// its yearly precipitation keeps it, and on every rung the snow a year of that climate leaves by the date.
void settleWater(Atmosphere& atmosphere, const Planet& planet) {
    const int n = AirCells;
    const double available = std::max(0.0, planet.surfaceWater) * WaterDensity;
    settleSnow(atmosphere, planet);
    double held = 0;
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const double water = atmosphere.openWater.at(x, z), land = 1.0 - water;
            double yearly = 0;   // kg/m²
            constexpr int PerCell = PlanetCells / AirCells;
            for (int dz = 0; dz < PerCell; ++dz)
                for (int dx = 0; dx < PerCell; ++dx)
                    yearly += WaterDensity * planet.precipitation.at(PerCell * x + dx, PerCell * z + dz) / (PerCell * PerCell);
            const double wetness = saturate(yearly / WetYear);
            const ColumnProfile profile = profileOf(columnAt(atmosphere, x, z));
            for (size_t band = 0; band < AirBands; ++band) {
                atmosphere.vapour[band].at(x, z) = StartHumidity * profile.capacity[band] * (water + land * wetness);
                held += atmosphere.vapour[band].at(x, z);
            }
            atmosphere.soil.at(x, z) = SoilCapacity * land * wetness;
            held += atmosphere.soil.at(x, z) + atmosphere.snow.at(x, z);
        }
    }
    // The air and the ground borrow their first water from the ocean, never more than half of it.
    held *= AirCellArea;
    const double share = held > 0 ? std::min(1.0, 0.5 * available / held) : 0.0;
    for (AirField* field : {&atmosphere.vapour[0], &atmosphere.vapour[1], &atmosphere.vapour[2], &atmosphere.soil})
        for (double& value : field->cells) value *= share;
    for (size_t cell = 0; cell < atmosphere.snowLadder.size(); ++cell) {
        for (double& pack : atmosphere.snowLadder.cells[cell]) pack *= share;
        weighSnow(atmosphere, cell);
    }
    atmosphere.ocean = available - held * share;
    atmosphere.runoff = 0;
}

void prepareWater(Atmosphere& atmosphere) {
    const double hour = 24.0 * (double(atmosphere.step) * AirStep / DaySeconds - std::floor(double(atmosphere.step) * AirStep / DaySeconds));
    for (int z = 0; z < AirCells; ++z)
        atmosphere.sunSwing[size_t(z)] = dailySwing(atmosphere.latitude[size_t(z)], atmosphere.season.declination, hour);
}

// One step of the water budget of every column in rows [begin, end). Every transfer is taken from
// one store and added to another; the row sums carry what crosses to the planet-wide stores.
void stepWater(Atmosphere& atmosphere, int begin, int end) {
    const int n = AirCells;
    const ColumnRates rates = columnRates(AirStep);
    const double perStep = 1.0 / AirStep, perSoil = 1.0 / SoilCapacity;
    const double oceanReach = atmosphere.openArea > 0 ? OceanReach * std::max(0.0, atmosphere.ocean) / atmosphere.openArea : 0.0;

    for (int z = begin; z < end; ++z) {
        double oceanGain = 0, overflow = 0, evaporated = 0, precipitated = 0;
        for (int x = 0; x < n; ++x) {
            const size_t cell = size_t(z) * n + size_t(x);
            AirColumn column = columnAt(atmosphere, x, z);
            const ColumnProfile profile = profileOf(column);
            const double water = atmosphere.openWater.cells[cell], land = 1.0 - water;
            const double air = column.airTemperature - LapseRate * column.ground;
            const double airDensity = saturationDensity(air);
            double soil = atmosphere.soil.cells[cell];
            const double windX = atmosphere.windX.cells[cell], windZ = atmosphere.windZ.cells[cell];
            const double speed = std::sqrt(windX * windX + windZ * windZ);
            const double held = airDensity * std::min(1.0, column.vapour[Low] * profile.perCapacity[Low]);

            const double thirst = evaporationRate(speed, airDensity, held) * AirStep;
            double fromOcean = 0, fromSoil = 0;
            if (water > 0) {
                const double surface = atmosphere.surfaceTemperature.cells[cell];
                const double open = water * (1.0 - frozenShare(surface));
                fromOcean = std::min(evaporationRate(speed, saturationDensity(surface), held) * AirStep * open, oceanReach * water);
            }
            if (land > 0) {
                fromSoil = std::min(soil, thirst * (land - atmosphere.snowCover.cells[cell]) * std::min(1.0, soil * perSoil / land));
            } else {
                // Ground that flooded hands its soil water to the ocean; its snow was settled by setSurface.
                oceanGain += soil;
                soil = 0;
            }
            column.vapour[Low] += fromOcean + fromSoil;
            soil -= fromSoil;

            const ColumnYield yield = stepColumn(column, profile, rates);

            // Every rung of the cell's ladder is rained and snowed on alike; what the rungs keep as
            // snow, weighed by the ground they stand for, is taken from what fell on the land. Snow
            // that sublimes joins the low air, melt wets the ground, and a pack over its limit sheds
            // the excess towards the sea.
            const double falling = yield.precipitation, ontoWater = falling * water;
            const SnowYield rungs = stepSnow(atmosphere, cell, column.airTemperature, falling, thirst);
            const double fromSnow = land * rungs.sublimed;
            column.vapour[Low] += fromSnow;
            soil += std::max(0.0, falling - ontoWater - land * rungs.snowfall) + land * rungs.melt;
            overflow += land * rungs.shed;
            const double room = SoilCapacity * land;
            if (soil > room) {
                overflow += soil - room;
                soil = room;
            }

            for (size_t band = 0; band < AirBands; ++band) {
                atmosphere.vapour[band].cells[cell] = column.vapour[band];
                atmosphere.liquid[band].cells[cell] = column.liquid[band];
                atmosphere.ice[band].cells[cell] = column.ice[band];
                atmosphere.falling[band].cells[cell] = column.falling[band];
            }
            atmosphere.soil.cells[cell] = soil;
            const double frozen = snowShare(air);
            atmosphere.snowRate.cells[cell] = falling * frozen * perStep;
            atmosphere.rainRate.cells[cell] = falling * (1.0 - frozen) * perStep;
            atmosphere.shade.cells[cell] = cloudShade(column);
            atmosphere.upperTemperature.cells[cell] += std::min(HeatLimit, HeatYield * yield.condensed);
            oceanGain += ontoWater - fromOcean;
            evaporated += fromOcean + fromSoil + fromSnow;
            precipitated += falling;
        }
        atmosphere.work.oceanGain[size_t(z)] = oceanGain;
        atmosphere.work.overflow[size_t(z)] = overflow;
        atmosphere.work.evaporated[size_t(z)] = evaporated;
        atmosphere.work.precipitated[size_t(z)] = precipitated;
    }
}

// Sums the rows in a fixed order, so the totals do not depend on how the rows were shared out.
void closeWater(Atmosphere& atmosphere) {
    double oceanGain = 0, overflow = 0, evaporated = 0, precipitated = 0;
    for (int z = 0; z < AirCells; ++z) {
        oceanGain += atmosphere.work.oceanGain[size_t(z)];
        overflow += atmosphere.work.overflow[size_t(z)];
        evaporated += atmosphere.work.evaporated[size_t(z)];
        precipitated += atmosphere.work.precipitated[size_t(z)];
    }
    atmosphere.runoff += overflow * AirCellArea;
    const double arriving = atmosphere.runoff * (1.0 - std::exp(-AirStep / RunoffDelay));
    atmosphere.runoff -= arriving;
    atmosphere.ocean += arriving + oceanGain * AirCellArea;
    atmosphere.evaporation = evaporated * AirCellArea / AirStep;
    atmosphere.precipitation = precipitated * AirCellArea / AirStep;
}

WaterLedger waterLedger(const Atmosphere& atmosphere) {
    WaterLedger ledger;
    ledger.ocean = atmosphere.ocean;
    ledger.runoff = atmosphere.runoff;
    const auto sum = [](const AirField& field) {
        double all = 0;
        for (double value : field.cells) all += value;
        return all * AirCellArea;
    };
    for (size_t band = 0; band < AirBands; ++band) {
        ledger.vapour += sum(atmosphere.vapour[band]);
        ledger.cloudLiquid += sum(atmosphere.liquid[band]);
        ledger.cloudIce += sum(atmosphere.ice[band]);
        ledger.precipitation += sum(atmosphere.falling[band]);
    }
    ledger.soil = sum(atmosphere.soil);
    ledger.snow = sum(atmosphere.snow);
    return ledger;
}

double oceanVolume(const Atmosphere& atmosphere) { return atmosphere.ocean / WaterDensity; }

} // namespace derivean::sim
