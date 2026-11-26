#pragma once

#include "atmosphere/constant/Air.hpp"
#include "atmosphere/constant/Snow.hpp"
#include "atmosphere/fn/Spectral.hpp"
#include "atmosphere/type/Cloud.hpp"
#include "atmosphere/type/Column.hpp"
#include "atmosphere/type/Plume.hpp"
#include "chronology/type/Calendar.hpp"
#include "core/type/Field.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace derivean::sim {

struct Planet;
class Workers;

using AirField = Field<double>;
using AirRows = std::array<double, AirCells>;
using AirLayers = std::array<AirField, AirBands>;
using SnowColumn = std::array<double, SnowRungs>;

// kg of water in every store the weather moves it between. Their sum never changes: clouds hold
// only what evaporated, and rain is only what a cloud held.
struct WaterLedger {
    double ocean{};           // open water
    double vapour{};
    double cloudLiquid{};
    double cloudIce{};
    double precipitation{};   // rain and snow still in the air
    double soil{};
    double snow{};
    double runoff{};          // on its way from saturated ground back to the ocean
};

inline double total(const WaterLedger& ledger) {
    return ledger.ocean + ledger.vapour + ledger.cloudLiquid + ledger.cloudIce + ledger.precipitation + ledger.soil + ledger.snow + ledger.runoff;
}

// What the calendar and the zonal-mean state dictate, refreshed once per simulated day.
struct AirSeason {
    double declination{};    // degrees
    AirRows insolation{};    // share of the solar constant at row centres, daily mean
    AirRows jet{};           // 1/s at vertex rows: vorticity of the zonal wind the circulation relaxes to
    AirRows growth{};        // 1/s at vertex rows: rate at which the temperature contrast feeds weather systems
    AirRows drift{};         // m/s along +Z through the north face of each row: surface branch of the overturning cells
    AirRows overturning{};   // m of lifting the cells force at row centres
    AirRows shear{};         // m/s of westerly wind the middle band has over the low band; the high band has twice that
};

// Work space of one step. Nothing in here survives a step, so it is not part of the state.
struct AirWork {
    AirField potential, bend, next;   // circulation
    AirField before;                  // air temperature before the wind carried it
    AirField courantX, courantZ;      // share of a cell crossing its west and north face in a wind step with the low band's wind, drift excluded
    AirLayers slack;                  // slope a cell may carry per unit of content and still stay non-negative, per band
    std::array<int, AirBands> passes{};   // passes the wind step of each band is cut into to stay stable
    std::array<AirRows, AirBands> alongX{}, alongZ{};   // Courant numbers every face of a row has on top of that
    std::array<AirField, 2 + 3 * AirBands> carried, slope;
    AirRows oceanGain{}, overflow{}, evaporated{}, precipitated{};   // kg/m² summed along a row
};

// Weather of the whole planet on the air grid. Cell (x, z) is centred at ((x + ½), (z + ½)) · AirCell;
// vorticity and stream function live on the cell corners at (x, z) · AirCell, which makes the winds
// through the cell faces free of divergence by construction.
//
// Vorticity is measured in the map plane with X east and Z south, so it is positive for a
// clockwise turn seen from above, and the planetary vorticity is negative in the northern half.
struct Atmosphere {
    uint64_t seed{};
    double era{};        // yr, fixed for the session
    double seaLevel{};   // m against the datum, as of the last setSurface
    int64_t step{};      // whole AirSteps since calendar second zero; the state is that of step · AirStep

    // Surface, resampled from the planet by setSurface.
    AirField ground;          // m above the sea, cell mean with open water at zero
    AirField climb;           // m the moist layer was lifted to stand over that ground (terrainLift)
    AirField hollow;          // 0 to 1: how far the cell lies below its neighbours
    AirField openWater;       // share of the cell
    double openArea{};        // m² of open water on the planet
    AirField surfaceGain;     // share of the way the surface moves to its radiative balance in a step
    AirField dragKeep;        // at corners: share of vorticity friction leaves after two steps
    AirField absolute;        // at corners, 1/s: planetary vorticity with the squeeze of the terrain

    // Per row, fixed for the session.
    AirRows latitude{}, baseTemperature{}, meanInsolation{}, spin{};   // at row centres
    AirRows vertexLatitude{}, vertexNorth{};                           // at corner rows

    AirSeason season;

    // Circulation.
    AirField vorticity, vorticityBefore;   // 1/s at corners, this step and the one before
    AirField stream;                       // m²/s at corners
    AirField eddy;                         // 1/s at corners: the vorticity of weather-system size, refreshed every few steps
    AirField faceX, faceZ;                 // m/s through the west and north face, low band
    AirField windX, windZ;                 // m/s at cell centres in the low band, direction of travel, drift included

    // Heat.
    AirField surfaceTemperature;   // °C of the ground or the mixed layer of the sea
    AirField airTemperature;       // °C near the surface as a daily mean, referenced to sea level
    AirField upperTemperature;     // °C at UpperLevel; with the air temperature it gives the column its lapse rate
    AirRows zonalTemperature{};    // °C, air temperature averaged along each row
    AirRows sunSwing{};            // −1 to 1 per row: dailySwing of the hour at which the last step began
    AirField shade;                // share of sunlight the clouds of the cell turned away during the last step

    // Water, kg/m² of cell area. Bands are those of ColumnProfile.
    AirLayers vapour, liquid, ice;
    AirLayers falling;             // rain and snow in the air; it falls where it formed and is not carried by the wind
    AirField soil;
    Field<SnowColumn> snowLadder;  // kg/m² of snow water equivalent on ground at each SnowLadder elevation, per unit of that ground
    Field<SnowColumn> snowArea;    // share of the cell's land each rung stands for; zero for rungs outside the cell's heights, which
                                   // still carry a pack so that neighbours and steeper ground than the planet knows can read one
    AirField snow;                 // kg/m² of cell area: the ladder weighed by snowArea and the land share; this is what the ledger counts
    AirField snowCover;            // share of the cell under snow, weighed the same way
    AirField snowUsual;            // share of the cell on ground too cold to lose its snow in the warmest month of the steady
                                   // climate; the heat budget counts only the cover beyond it, which the annual mean does not hold yet
    AirField rainRate, snowRate;   // kg/m²/s reaching the surface during the last step
    AirField lift, glide;          // m the air of the cell was displaced upwards (AirColumn::lift and glide)
    double ocean{};                // kg in open water
    double runoff{};               // kg in transit to the ocean

    // Planet totals of the last step, kg/s.
    double evaporation{}, precipitation{};

    Plume plume;                   // ash, sulphur and what else sources put into the air

    PoissonSolver poisson;
    AirWork work;
};

// Opens a source. From the step that covers its start to the one that covers its end, its ash,
// gas, vapour and heat enter the air over its position at its height: dust and gas are carried by
// the wind of that level, lapilli and sand-size ash land downwind of it within the step, vapour
// joins the band's vapour and heat warms the cell's air.
void addSource(Atmosphere& atmosphere, const AirSource& source);

// Ash on the ground at a world position: the blankets of lapilli and sand-size ash evaluated at
// the point itself, plus the fine ash that settled out of the grid, linear between weather cells.
// Sand-size ash lands within its fall time of the source (height × wind ÷ 1 m/s, some hundred
// kilometres); beyond that only the grid's fine ash is left, and that is where one hands over to
// the other.
AshFall ashFallAt(const Atmosphere& atmosphere, Vec2 position);

PlumeLedger plumeLedger(const Atmosphere& atmosphere);

// Captures the planet's surface, fills the air from the planet's steady climate for the date and
// runs AirSpinUp steps up to it, so the first sample already sees travelling weather. The water
// the air and ground start with is taken out of Planet::surfaceWater: total(waterLedger) equals
// the planet's surface water from the first step on.
Atmosphere startAtmosphere(const Planet& planet, const Calendar& calendar, Workers& workers);

// Runs whole steps until the state is that of the last AirStep at or before `now`. The result
// depends only on the start and on `now`, never on how the way there was cut into calls or on the
// number of worker threads.
void advanceAtmosphere(Atmosphere& atmosphere, const Calendar& now, Workers& workers);

// Replaces open water and terrain from a planet-resolution height field (square, a whole number
// of its cells per weather cell). Ground that fell dry stops evaporating at once; stores on ground
// that flooded drain into the ocean. The snow ladders are weighed anew by the heights the field
// shows: a cell never gains snow by that, and what it holds less goes to the ocean.
void setSurface(Atmosphere& atmosphere, const Field<float>& elevation, double seaLevel);

WaterLedger waterLedger(const Atmosphere& atmosphere);

// m³ of liquid for Planet::surfaceWater once the caller reconciles the planet with the weather.
double oceanVolume(const Atmosphere& atmosphere);

struct WeatherSample {
    double temperature{};   // °C of the air at the ground, with the daily cycle
    double rain{};          // mm/h
    double snow{};          // mm/h of water equivalent
    double cloudCover{};    // share of the sky, all bands together
    double cloudWater{};    // kg/m² of liquid and ice
    double humidity{};      // relative, 0 to 1, near the ground
    double snowpack{};      // kg/m² of snow water equivalent lying on ground of that elevation (snowAt)
    Vec2 wind;              // m/s near the ground, direction of travel
};

// Weather at a world position whose ground stands at `groundElevation` (m against the datum).
// Rain and snow split by the temperature at that ground, so a valley gets rain from the cloud
// that snows on the summit above it.
WeatherSample weatherAt(const Atmosphere& atmosphere, Vec2 position, double groundElevation, const Calendar& calendar);

// kg/m² of snow water equivalent on ground at `elevation` (m against the datum) at a world
// position: linear between the rungs of the ladder and between the four weather cells around the
// position, so it has no steps at cell borders; below a cell's snow line it is zero, and beyond
// the ends of the ladder it holds the end rungs. It knows the height of the ground and nothing
// else about it: steep faces that shed their snow and wind that drifts it are not in here.
double snowAt(const Atmosphere& atmosphere, Vec2 position, double elevation);

// m above the sea of the lowest snow in a cell: between its highest bare rung and the first one
// under snow, where the packs of the rungs above would thin out to nothing; the top of the ladder
// where no rung holds any.
double snowLineOf(const Atmosphere& atmosphere, int x, int z);

// The column of air of a cell as the last step saw it.
AirColumn columnAt(const Atmosphere& atmosphere, int x, int z);

// m/s at a cell centre in a band: the low band's wind, faster aloft, plus the westerly shear.
Vec2 bandWind(const Atmosphere& atmosphere, int band, int x, int z);

// Everything an inspector shows about the air over a position, and why its clouds are there.
struct WeatherReport {
    WeatherSample weather;
    AirColumn column;
    ColumnProfile profile;
    CloudReport clouds;
    std::array<Vec2, AirBands> wind;        // m/s per band
    std::array<double, AirBands> humidity{};   // vapour against capacity per band; above 1 in air that is about to condense
    double pressure{};                      // Pa at the ground
    double dewPoint{};                      // °C at the ground
    double verticalSpeed{};                 // m/s of large-scale ascent
    double snowLine{};                      // m above the sea of the lowest snow in the cell (snowLineOf)
    AshFall ash;                            // ash on the ground here
    double aerosolDepth{};                  // visible optical depth of ash and sulphate veil overhead
    WaterLedger ledger;                     // planet-wide water balance
    std::string cause;                      // one sentence on how the column's clouds came about
};

WeatherReport inspectWeather(const Atmosphere& atmosphere, Vec2 position, double groundElevation, const Calendar& calendar);

// Cloud description of the whole planet for a renderer; see CloudField.
CloudField cloudField(const Atmosphere& atmosphere);

// Where the sun stands over a world position; the same sun lights the clouds and casts their shadows.
SunDirection sunDirectionAt(Vec2 position, const Calendar& calendar);

} // namespace derivean::sim
