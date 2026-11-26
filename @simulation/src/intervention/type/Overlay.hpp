#pragma once

#include "chronology/type/Calendar.hpp"
#include "hydrology/type/Basins.hpp"
#include "hydrology/type/WaterGrid.hpp"
#include "intervention/type/Edit.hpp"

#include <map>
#include <memory>

namespace derivean::sim {

// Every site tile within this distance of an edit becomes live, and stays live. The rule reads
// only the edit, so the set of live tiles is a function of the history of edits and of nothing else.
inline constexpr double EditReach = 256.0;   // m

// Volcanic rock a Deposit laid on a live cell: it fills the column from its floor up to the rock
// surface, so cutting the rock thins it by itself. The floor is kept below the datum, as the
// replay keeps it, so foundering takes the pile down whole.
struct LivePile {
    float floor{-1e30f};  // m below the datum; above any rock where none was laid
    float laid{};         // m ever laid into it, buried cover included
    float lava{};         // m of that which was lava
    Material kind{Material::Basalt};
    uint64_t vent{};

    float thickness(float rock, float datum) const { return std::max(0.0f, rock - (datum - floor)); }
};

// A site tile the world has been changed on or near. It is the tile's own columns and water as
// they are now; the untouched tile stays in the atlas. Arrays are laid out as in Tile.
struct EditedTile {
    TileKey key;
    std::shared_ptr<const Tile> pristine;
    Ground now;                        // columns as dug, built and reworked; `datum` is the untouched one but where the ground foundered (a Deposit's drop)
    std::vector<float> water;          // m, surface where water stands; below the ground where it is dry
    std::vector<float> discharge;      // m³/yr
    std::vector<double> depth, salt;   // m of water; kg/m² dissolved in it, or lying as a crust where it dried up
    bool touched{};                    // columns changed since its group's grid last read them
    std::vector<LivePile> pile;        // empty until a Deposit lays rock here
};

// Live tiles that touch form one group, solved as one grid; groups that do not touch never
// influence each other. The grid is working storage rebuilt from the tiles whenever the group
// changes; between changes it carries the state and the tiles are refreshed from it.
struct LiveSet {
    std::vector<EditedTile*> tiles;    // by key
    int32_t originX{}, originZ{};      // tile coordinates of the grid's first tile
    int tilesX{}, tilesZ{};
    WaterGrid grid;
    BasinTree basins;
    Lakes lakes;
    SurgeScratch scratch;
    bool stale{true};                  // the solid changed since `basins` was built
    std::vector<int32_t> shore;        // rim cells that hold sea
    std::vector<float> shoreLevel;     // m, the level they held when the tiles were built
    double seaShift{};                 // m the sea has moved since
    double seconds{};                  // calendar second the water has been advanced to
    double disturbed{};                // calendar second of the last edit in the group
    double shaped{};                   // calendar second sediment has been moved up to
    double owed{};                     // yr of rain and evaporation the event solver left to the quiet one
    double awake{};                    // s of calm still to pass before the quiet solver takes over
};

struct Overlay {
    std::map<std::pair<int32_t, int32_t>, std::unique_ptr<EditedTile>> tiles;   // by (z, x): one fixed visiting order
    std::vector<LiveSet> sets;
    Tally adopted;        // standing water of the untouched tiles when they became live
    WaterFlows water;     // what has crossed the edge of the live tiles since
    SolidFlows solid;     // what running water moved
    double dug{}, filled{};   // m³ of solid the edits took out of the world and put into it
};

// The inhabited part of a world: its clock, what was done to it, and what became of that.
struct Interventions {
    Calendar calendar;
    double rate{1.0};     // calendar seconds per second of the authority that drives time
    bool paused{};
    std::vector<Edit> history;   // by time; entries from `applied` on are still to come
    size_t applied{};
    Overlay overlay;
};

} // namespace derivean::sim
