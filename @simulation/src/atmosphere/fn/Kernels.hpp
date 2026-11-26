#pragma once

#include "atmosphere/type/Atmosphere.hpp"

namespace derivean::sim {

// The pieces of one atmosphere step. Advance.cpp owns their order; each piece owns its constants.

// Season.cpp
AirSeason seasonOf(const Atmosphere& atmosphere);

// Circulation.cpp
void shapeCirculation(Atmosphere& atmosphere);    // friction and terrain squeeze from the surface
void settleCirculation(Atmosphere& atmosphere);   // belts plus the seeded disturbance that weather grows from
void stepCirculation(Atmosphere& atmosphere);     // vorticity of the next step
// Stream function and winds from the vorticity; `eddies` also refreshes Atmosphere::eddy.
void refreshWind(Atmosphere& atmosphere, bool eddies);

// Transport.cpp
void prepareTransport(Atmosphere& atmosphere);
// How a field rides the wind of its band. Temperatures leave out the overturning drift, which
// would squeeze them where it converges; water follows it in the low band and may not go negative.
// Cloud is carried without slopes: it lives for hours, and what matters is where it goes, not how
// sharp its edge stays.
enum class Carried { Temperature, Vapour, Cloud };

// Carries `from` one wind step along the wind of a band into `to` in flux form: what leaves a
// cell enters its neighbour. `from` is used as work space when the step takes several passes.
void advect(const Atmosphere& atmosphere, AirField& from, AirField& to, AirField& slope, int band, Carried kind);

// Share of the low band's rotational wind each band moves with; the westerly shear comes on top.
inline constexpr double BandWind[AirBands] = {1.0, 1.25, 1.5};

// Heat.cpp
void shapeHeat(Atmosphere& atmosphere);                        // thermal inertia from the surface
void settleHeat(Atmosphere& atmosphere);                       // temperatures in step with the date
void stepHeat(Atmosphere& atmosphere, int begin, int end);     // rows [begin, end)

// Water.cpp
void settleWater(Atmosphere& atmosphere, const Planet& planet);
void prepareWater(Atmosphere& atmosphere);                     // what the hour of the day does to every row
void stepWater(Atmosphere& atmosphere, int begin, int end);    // rows [begin, end)
void closeWater(Atmosphere& atmosphere);                       // row sums into ocean and runoff

// Snow.cpp
void shapeSnow(Atmosphere& atmosphere, const Field<float>& elevation, double seaLevel);   // hypsometry of every cell from the planet's heights
void settleSnow(Atmosphere& atmosphere, const Planet& planet);                            // the pack a year of the steady climate leaves by the date
struct SnowYield {
    double snowfall{}, melt{}, sublimed{}, shed{};   // kg/m² of the cell's land over the step, summed over its rungs by area
};
// One step of every rung of a cell under `falling` kg/m² of precipitation and air that could take
// up `thirst` kg/m² from a wet surface; the air at a rung is `seaLevelAir` cooled by LapseRate, the
// same rate that decides between rain and snow in a weather sample.
SnowYield stepSnow(Atmosphere& atmosphere, size_t cell, double seaLevelAir, double falling, double thirst);
void weighSnow(Atmosphere& atmosphere, size_t cell);   // a cell's snow and snow cover from its rungs

// Plume.cpp
void shapePlume(Atmosphere& atmosphere);                       // empty fields
void stepPlume(Atmosphere& atmosphere, Workers& workers);      // sources, transport, fallout and dimming of one step

// HazeSources.cpp
void shapeHaze(Atmosphere& atmosphere, const Planet& planet);   // empty fields and the plant cover from the planet's climate
// What wind, sea and plants put into the low band over one step, rows [begin, end); kg per row and kind added to `rows[z]`.
void raiseHaze(Atmosphere& atmosphere, int begin, int end, std::array<double, HazeKinds>* rows);

// Haze.cpp
void stepHaze(Atmosphere& atmosphere, Workers& workers);   // sources, wash-out, settling and optical depth of one step; Advance.cpp carries it

// Advance.cpp
void stepAtmosphere(Atmosphere& atmosphere, Workers& workers);

inline int wrapAir(int index) { return index & (AirCells - 1); }

} // namespace derivean::sim
