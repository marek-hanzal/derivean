#pragma once

#include "intervention/fn/Intervention.hpp"

#include <string>

namespace derivean::sim {

// The acceptance scenarios, shared by the checks that assert them and the tool that prints them.
// Each starts from the untouched world, makes its edits through `intervene` and reads the result
// back through the same budget and accessors the game would use.

// One moment of a scenario: where everything is, and the scenario's own measurements.
struct ScenarioStage {
    std::string label;
    WorldBudget budget;
    std::vector<double> values;   // one per ScenarioReport::measures
};

struct ScenarioReport {
    std::string title;
    std::vector<std::string> measures;
    std::vector<ScenarioStage> stages;
    std::string failure;          // why the scenario could not be set up; empty when it ran
};

double valueOf(const ScenarioReport& report, size_t stage, const std::string& measure);

// Back to the untouched world at second zero with an empty history.
void forgetInterventions(World& world);

// ScenarioSite.cpp
// A stream on dry land, found from the planet down: the dry planet cells the largest rivers cross,
// then the heaviest-laden stream cell of each one's region tile and of its local tile that has
// relief around it; the first whose site has room for the works of the cascade and the channel of
// the diversion (cascadePlace, diversionPlace). Only tiles with even keys are taken, which need no
// neighbour of their own level and so cost one tile per level. Any interventions are forgotten; the
// atlas keeps the tiles built on the way.
struct StreamSite {
    bool found{};
    TileKey tile;   // site tile
    Vec2 at;        // m, on the stream as the local level sees it, in the middle of that tile
};

StreamSite findStream(World& world);

// Reading a group's grid for a place to work. Cells are indices into the grid.
inline constexpr double SiteWatch = 3600.0;     // s a stream site is watched before the scenarios read it
// The group of the one site a scenario watches.
inline const LiveSet& setOf(World& world) { return world.interventions.overlay.sets.front(); }
// m, the ground or the standing water on it.
double surfaceAt(const LiveSet& set, int32_t cell);
// m³ of standing water over the cells [x0, x1] × [z0, z1].
double waterIn(const LiveSet& set, int x0, int x1, int z0, int z1);

inline constexpr int EditClear = 66;            // cells an edit must keep from the grid's edge to stay within the group's tiles
inline constexpr double StreamFlow = 1e4;       // m³/yr from which a cell counts as part of a stream

// The stream cell nearest the middle of the grid around which a box of the given extents, in
// cells, stays clear of the edge; -1 when there is none. `dry` asks for open bed, not a lake.
int32_t streamAnchor(const LiveSet& set, int west, int east, int north, int south, bool dry);
// Where the water of a cell goes: down the fall line and across each full lake on the way.
std::vector<int32_t> courseFrom(const LiveSet& set, int32_t cell, int steps);
// Unit vector towards where the water of a cell comes from; zero when nothing drains into it.
Vec2 arrivesFrom(const LiveSet& set, int32_t cell);
Vec2 centreOf(const LiveSet& set, int32_t cell);

// The standing water over one cell of a group's grid.
struct LakeState {
    double level{};    // m; the ground where the cell is dry
    double volume{};   // m³
    int cells{};
    bool full{};       // at its sill
};

LakeState lakeAt(const LiveSet& set, int32_t cell);

// Where the cascade's works stand: the open stream bed nearest the middle of the set with next to
// no standing water under their footprint, `flip` turning them towards the incoming water. The
// anchor is -1 where there is none.
struct WorksPlace {
    int32_t anchor{-1};
    int flip{1};
};
WorksPlace cascadePlace(const LiveSet& set);
// Where the diversion's channel starts and ends: the open stream bed nearest the middle that runs
// on as one bed for a while (`old`, its course) and has lower ground or water within reach, the
// `outfall`; `intake` the sill the channel is cut to. The anchor is -1 where there is none.
struct IntakePlace {
    int32_t anchor{-1}, outfall{-1};
    double intake{};
    std::vector<int32_t> old;
};
IntakePlace diversionPlace(const LiveSet& set);

// The untouched site watched for SiteWatch, as every stream scenario starts: the set of its group,
// good until the next edit.
const LiveSet& openSite(World& world, const StreamSite& site);

// ScenarioLakes.cpp
// A walled excavation on a stream holds an upper lake, a small lower lake and a third basin. Once
// the upper lake is full its dam is cut: it runs into the lower one, which reaches its sill and
// overflows into the third.
ScenarioReport runLakeCascade(World& world, const StreamSite& site);

// A channel with its intake under the stream's bed leads the water round a bend to lower ground;
// the old bed falls dry. Then `years` pass.
ScenarioReport runDiversion(World& world, const StreamSite& site, double years);

// ScenarioOcean.cpp
// A strait is cut from the deepest floor of the world ocean to the lowest closed depression above
// its surface. Water does not run uphill: nothing moves, and the bodies stay as they were.
ScenarioReport runStrait(World& world);

} // namespace derivean::sim
