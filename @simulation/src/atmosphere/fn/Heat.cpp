#include "atmosphere/fn/Cover.hpp"
#include "atmosphere/fn/Kernels.hpp"

#include "climate/fn/Moisture.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double SeasonalSwing = 110.0;            // °C of radiative balance per share of the solar constant
constexpr double OceanInertia = 150 * DaySeconds;  // s for the mixed layer of the sea to follow its balance
constexpr double LandInertia = 20 * DaySeconds;    // s for the ground
constexpr double AirCoupling = 2.5 * DaySeconds;   // s for the air to take the temperature of the surface under it
constexpr double SnowShade = 0.2;                  // share of the sunlight a cell takes up that a full snow cover costs it
constexpr double IceShade = 0.40;                  // the same for sea ice
constexpr double EkmanDepth = 500.0;               // m; friction lifts air by this depth times the cyclonic vorticity
constexpr double Stability = 0.004;                // °C/m; warm air advected at this rate glides up over the cold air ahead
constexpr double LiftLimit = 1500.0;               // m
constexpr double SinkLimit = -800.0;               // m
constexpr double CalmBand = 0.3;                   // share of Coriolis below which friction stops organising ascent
constexpr double AshOnSnow = 0.7;                  // share of a snow cover's shading that a blanket of ash takes away
constexpr double SootedSnow = 0.5;                 // kg/m² of ash that does so in full
constexpr double CloudShade = 0.3;                 // share of sunlight a fully shading sky keeps from the surface
constexpr double CloudBlanket = 4.0;               // °C such a sky adds by holding back the surface's own radiation
constexpr double UsualShade = 0.5;                 // shade the annual-mean climate already contains
constexpr double UpperLapse = 6.5e-3;              // °C/m the column settles to when nothing disturbs it
constexpr double UpperSettling = 4 * DaySeconds;   // s for the upper air to settle over the surface air below it

// Response of a store with first-order inertia to the yearly cycle: how far it trails and how
// much of the swing it keeps.
struct YearlyResponse {
    double lag;       // days
    double damping;
};

YearlyResponse yearlyResponse(double inertia) {
    const double phase = 2 * Pi * inertia / YearSeconds;
    return {std::atan(phase) * YearDays / (2 * Pi), 1.0 / std::sqrt(1.0 + phase * phase)};
}

} // namespace

void shapeHeat(Atmosphere& atmosphere) {
    for (size_t cell = 0; cell < atmosphere.openWater.size(); ++cell) {
        const double water = atmosphere.openWater.cells[cell];
        atmosphere.surfaceGain.cells[cell] = 1.0 - std::exp(-AirStep / (water * OceanInertia + (1.0 - water) * LandInertia));
    }
    for (int z = 0; z < AirCells; ++z) {
        const double planetary = -Coriolis * std::sin(atmosphere.latitude[size_t(z)] * Pi / 180.0);
        atmosphere.spin[size_t(z)] = planetary / std::sqrt(planetary * planetary + CalmBand * CalmBand * Coriolis * Coriolis);
    }
}

void settleHeat(Atmosphere& atmosphere) {
    const double day = double(atmosphere.step) * AirStep / DaySeconds;
    const YearlyResponse sea = yearlyResponse(OceanInertia), land = yearlyResponse(LandInertia);
    for (int z = 0; z < AirCells; ++z) {
        const double latitude = atmosphere.latitude[size_t(z)], mean = atmosphere.meanInsolation[size_t(z)];
        const double overSea = sea.damping * (dailyInsolation(latitude, declinationOn(day - sea.lag)) - mean);
        const double overLand = land.damping * (dailyInsolation(latitude, declinationOn(day - land.lag)) - mean);
        double sum = 0;
        for (int x = 0; x < AirCells; ++x) {
            const double water = atmosphere.openWater.at(x, z);
            const double settled = atmosphere.baseTemperature[size_t(z)] + SeasonalSwing * mix(overLand, overSea, water);
            atmosphere.surfaceTemperature.at(x, z) = settled;
            atmosphere.airTemperature.at(x, z) = settled;
            atmosphere.upperTemperature.at(x, z) = settled - UpperLapse * UpperLevel;
            atmosphere.shade.at(x, z) = UsualShade;
            sum += settled;
        }
        atmosphere.zonalTemperature[size_t(z)] = sum / AirCells;
    }
}

// The surface follows its radiative balance with the inertia of what it is made of; clouds keep
// sunlight from it and hold its own radiation back. The air that arrived with the wind takes on
// the temperature of the surface under it, and the upper air settles over that air. The same pass
// measures how far the air of each cell is being lifted: by friction under cyclonic flow and the
// overturning cells, and by warm air gliding up over colder air.
void stepHeat(Atmosphere& atmosphere, int begin, int end) {
    const int n = AirCells;
    const double airKeep = std::exp(-AirStep / AirCoupling), upperKeep = std::exp(-AirStep / UpperSettling);
    const double ekmanReach = EkmanDepth * LiftTime;
    const double glideReach = LiftTime / (Stability * AirStep);
    for (int z = begin; z < end; ++z) {
        const int south = wrapAir(z + 1);
        const double sun = atmosphere.season.insolation[size_t(z)], meanSun = atmosphere.meanInsolation[size_t(z)];
        const double base = atmosphere.baseTemperature[size_t(z)], spin = atmosphere.spin[size_t(z)];
        const double overturning = atmosphere.season.overturning[size_t(z)];
        double sum = 0;
        for (int x = 0; x < n; ++x) {
            const size_t cell = size_t(z) * n + size_t(x);
            const int east = wrapAir(x + 1);
            const double water = atmosphere.openWater.cells[cell];
            double surface = atmosphere.surfaceTemperature.cells[cell];
            // Ash on snow darkens it: the pack then turns away less of the sun.
            const double sooted = 1.0 - AshOnSnow * saturate(atmosphere.plume.fallen.cells[cell] * (1.0 / SootedSnow));
            const double snowCover = std::max(0.0, atmosphere.snowCover.cells[cell] - atmosphere.snowUsual.cells[cell]) * sooted;
            const double shade = atmosphere.shade.cells[cell];
            const double absorbed = (1.0 - SnowShade * snowCover - IceShade * water * frozenShare(surface)) *
                                    (1.0 - CloudShade * shade) / (1.0 - CloudShade * UsualShade) * (1.0 - atmosphere.plume.dimming.cells[cell]);
            const double balance = base + SeasonalSwing * (absorbed * sun - meanSun) + CloudBlanket * (shade - UsualShade);
            surface += (balance - surface) * atmosphere.surfaceGain.cells[cell];
            atmosphere.surfaceTemperature.cells[cell] = surface;

            const double arrived = atmosphere.airTemperature.cells[cell];
            const double warming = arrived - atmosphere.work.before.cells[cell];
            const double air = surface + (arrived - surface) * airKeep;
            atmosphere.airTemperature.cells[cell] = air;
            sum += air;
            const double settledUpper = air - UpperLapse * UpperLevel;
            atmosphere.upperTemperature.cells[cell] = settledUpper + (atmosphere.upperTemperature.cells[cell] - settledUpper) * upperKeep;

            const double turning = 0.25 * (atmosphere.vorticity.at(x, z) + atmosphere.vorticity.at(east, z) +
                                           atmosphere.vorticity.at(x, south) + atmosphere.vorticity.at(east, south));
            atmosphere.lift.cells[cell] = std::clamp(ekmanReach * turning * spin + overturning, SinkLimit, LiftLimit);
            atmosphere.glide.cells[cell] = std::clamp(glideReach * warming, SinkLimit, LiftLimit);
        }
        atmosphere.zonalTemperature[size_t(z)] = sum / n;
    }
}

} // namespace derivean::sim
