#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Ocean.hpp"
#include "hydrology/fn/Water.hpp"
#include "intervention/constant/Pace.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

// The sea a shore stands on may have fallen since its tiles were built: the rim follows it.
void matchSea(World& world, LiveSet& set) {
    if (set.shore.empty()) return;
    const double span = tileSpan(Level::Site);
    const Vec2 middle{(set.originX + 0.5 * set.tilesX) * span, (set.originZ + 0.5 * set.tilesZ) * span};
    const double shift = seaLevelNear(world.ocean, planetPoint(world.options.seed, middle)) - world.ocean.pristineLevel;
    if (shift == set.seaShift) return;
    set.seaShift = shift;
    for (size_t i = 0; i < set.shore.size(); ++i) {
        const size_t cell = size_t(set.shore[i]);
        set.grid.held[cell] = std::max(set.grid.ground[cell], float(double(set.shoreLevel[i]) + shift));
    }
    set.stale = true;
}

} // namespace

// Sediment does not move during an event: an event is short, and the event solver keeps no lakes
// to tell the sediment where the current dies.
void shapeSet(World& world, LiveSet& set) {
    const double years = (set.seconds - set.shaped) / YearSeconds;
    set.shaped = set.seconds;
    if (years <= 0 || set.awake > 0 || set.stale) return;
    Overlay& overlay = world.interventions.overlay;
    routeDischarge(set.grid, set.basins, set.lakes);
    if (!moveSediment(set.grid, set.basins, set.lakes, years, overlay.solid)) return;
    set.basins = buildBasins(set.grid);
    set.lakes = settle(set.grid, set.basins, {}, overlay.water);
}

void evolve(World& world, LiveSet& set, double to, bool force) {
    const double span = to - set.seconds;
    if (span <= 0) return;
    Overlay& overlay = world.interventions.overlay;
    WaterGrid& grid = set.grid;
    matchSea(world, set);
    if (set.awake > 0 && span <= SurgeSpan && to - set.disturbed <= LongestEvent) {
        const double unrest = surge(grid, span, overlay.water, set.scratch);
        set.owed += span / YearSeconds;
        set.awake = unrest > CalmRate ? CalmSpan : set.awake - span;
        set.seconds = to;
        storeSet(set);
        return;
    }
    if (!force && set.awake <= 0 && span < QuietQuantum) return;
    for (auto& face : grid.flux) std::fill(face.begin(), face.end(), 0.0f);
    set.awake = 0;
    while (set.seconds < to) {
        if (set.stale) set.basins = buildBasins(grid);
        set.stale = false;
        const double mark = (std::floor(set.seconds / SedimentStep) + 1.0) * SedimentStep;
        const double end = std::min({to, mark, set.seconds + std::max(FirstQuietStep, set.seconds - set.disturbed)});
        const double years = (end - set.seconds) / YearSeconds;
        set.lakes = fillAndSpill(grid, set.basins, years, years + set.owed, overlay.water);
        set.owed = 0;
        set.seconds = end;
        if (end == mark) shapeSet(world, set);
    }
    routeDischarge(grid, set.basins, set.lakes);
    storeSet(set);
}

} // namespace derivean::sim
