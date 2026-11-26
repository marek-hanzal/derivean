#pragma once

#include "chronology/type/Calendar.hpp"
#include "landscape/type/Level.hpp"

namespace derivean::sim {

// Ground water is worked out on the cells of a local tile and `WaterReach` cells of its neighbours.
// Every step of the model is a sweep in which a cell reads its eight neighbours as they stood
// before the sweep, so each sweep lets the wrong values at the field's edge creep one cell inwards
// and no farther. What a tile says about its own cells and the ring around them is then, to the
// bit, what every other tile says about those cells, and no tile reads another's result.
//
// Rings a build spoils from the field's edge: 1 (slope and lie of the beds, clamped at the edge),
// 1 (ways and outlets read the neighbour's column), LevelSweeps, 1 (shares read the neighbour's
// level), RouteSweeps, 1 (the sweep that finds what is still under way), 1 (a vent asks its
// neighbours whether water already runs in from above). The regional inflow (fn/Inflow.cpp) is
// spread over a region cell's 16 × 16 cells by their streams, so a clamped slope in the outer ring
// spoils the inflow 16 cells deep; that is inside the levels' rings and reaches only the routing.
// A stage that adds a read of a neighbour adds a ring.
// Water is followed RouteSweeps cells (3.5 km). What still travels farther is booked as underflow;
// the deep share of the recharge is not routed here at all but handed to the regional flow
// (fn/Regional.cpp). Cost grows with the field's area times the sweeps.
inline constexpr int WaterReach = 96;
inline constexpr int FieldCells = TileCells + 2 * WaterReach;
inline constexpr int LevelSweeps = 36;   // cells over which the level of a valley is felt under the ground beside it
inline constexpr int RouteSweeps = 54;   // cells ground water is followed; what travels farther is booked as underflow
static_assert(LevelSweeps + RouteSweeps + 5 < WaterReach && 16 <= LevelSweeps + 3);

inline constexpr double WaterCell = 64.0;                             // m
inline constexpr float WaterCellArea = float(WaterCell * WaterCell);  // m²
static_assert(WaterCell == LevelSpecs[size_t(Level::Local)].cell);
inline constexpr float PerYear = float(YearSeconds);                  // m/s to m/yr

// Recharge: the share of the water left after evaporation that enters the ground.
inline constexpr float WetShare = 0.06f;        // share of the year in which the rain of a year falls
inline constexpr float StormDepth = 0.025f;     // m of rain one storm brings where a metre falls in a year, and in proportion elsewhere
inline constexpr float StormLength = 2.0e4f;    // s a storm lasts
inline constexpr float ThinCover = 0.05f;       // m of a cover layer under which rain meets what lies below it
inline constexpr float SteepShed = 0.9f;        // slope at which half the water has run off before it could soak in
inline constexpr float MostRecharge = 0.95f;

// Fractures. A joint set conducts by the cube of its aperture over its spacing; apertures close
// with depth and stand open in the zone weather has reached.
inline constexpr float JointConductivity = 1.0e-6f;   // m/s of tight joints 3 m apart in rock that keeps them open
inline constexpr float JointReference = 3.0f;         // m
inline constexpr float OpenJointGain = 10.0f;         // within RockMass::openDepth of the rock surface
inline constexpr float JointDecay = 60.0f;            // m of depth over which joints lose 1/e of their conductivity
inline constexpr float SolutionGain = 30.0f;          // how much more the joints of carbonate rock conduct for being widened by solution
inline constexpr float OpenedZone = 10.0f;            // m of weathered, opened joints where the rock mass is not known: closed forms of a cell
inline constexpr float FaultConductivity = 2.0e-6f;   // m/s along the crushed rock of a domain fault, at any depth it reaches
inline constexpr float FaultDepth = 3000.0f;          // m a fault's crushed rock stays open to
inline constexpr float DriveRatio = 6.0f;             // m of depth water is driven down a fault per metre its catchment stands above the vent
inline constexpr float HottestRock = 300.0f;          // °C beyond which the model follows no water

// Storeys.
inline constexpr float WalkDepth = 160.0f;        // m below the rock surface the beds are read one by one
inline constexpr int WalkBeds = 28;
inline constexpr float CirculationDepth = 220.0f; // m below the rock surface where ordinary circulation ends
inline constexpr float PerchShare = 0.5f;         // a bed that passes less than this share of the recharge holds water up
inline constexpr float LeastRecharge = 0.02f;     // m/yr against which a bed is judged where less than that soaks in
// Share of its conductivity the rock under the base of circulation passes downwards: what a
// regional flow system a kilometre thick and tens of kilometres long can carry away below it.
inline constexpr float DeepGradient = 3.0e-5f;
inline constexpr float LosingShare = 0.01f;     // of a stream's flow its bed loses in a cell (15 % a km) beyond which it stands above the table
inline constexpr float HillLength = 400.0f;       // m water of a slope travels to its outlet: sets the least gradient of a water table
inline constexpr float LeastGradient = 1.0e-3f;
inline constexpr float MostGradient = 4.0f;
inline constexpr float SameSheet = 1.0f;          // m by which the depth of one bed may differ between two columns' reading of it

// Surface water that takes ground water in: a stream of the local level, or any standing water.
inline constexpr float DrainingFlow = 1.0e5f;     // m³/yr
inline constexpr float SwallowLimit = 6.0e7f;     // m³/yr; a stream larger than this keeps its bed across soluble rock

// Vents.
inline constexpr float LeastSpring = 950.0f;      // m³/yr (0.03 l/s) below which emerging water is a seep, not a vent
inline constexpr float GatherShare = 3.0f;        // a vent yields at least this many times what its own cell soaks up: it gathers
inline constexpr float LeastSeep = 30.0f;         // m³/yr below which a cell counts as dry
inline constexpr float FaultShare = 0.4f;         // crush above which rising water is the fault's

// Karst.
inline constexpr float SolubleEnough = 20.0f;     // m/Myr per m/yr of runoff: limestone, chalk, marble, salt
inline constexpr float KarstThickness = 8.0f;     // m of soluble rock that holds a conduit system
inline constexpr float KarstReach = 10.0f;        // m the water table may stand above soluble rock and still drain through it
inline constexpr float PlaneReach = 45.0f;        // m from a cell's centre within which a joint plane passes through the cell
inline constexpr float LeastConduit = 0.25f;      // m radius below which a passage is a fissure and is not listed
inline constexpr float MostConduit = 9.0f;        // m radius
inline constexpr float ConduitScale = 3.0f;       // m radius of the passage 1 m³/s keeps open; radius grows with flow^0.4
inline constexpr float WallRetreat = 2.0e-4f;     // m/yr a limestone wall dissolves back under flowing water

} // namespace derivean::sim
