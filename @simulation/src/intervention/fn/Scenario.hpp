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
// A stream on dry land, found from the planet down: the dry planet cell the largest river crosses,
// then the heaviest-laden stream cell of its region tile and of its local tile that has relief
// around it. Only tiles with even keys are taken, which need no neighbour of their own level and
// so cost one tile per level.
struct StreamSite {
    bool found{};
    TileKey tile;   // site tile
    Vec2 at;        // m, on the stream as the local level sees it, in the middle of that tile
};

StreamSite findStream(World& world);

// Reading a group's grid for a place to work. Cells are indices into the grid.
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
