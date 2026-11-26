#include "simulation/fn/Live.hpp"

#include "hydrology/constant/Water.hpp"
#include "hydrology/fn/Ocean.hpp"
#include "intervention/fn/Intervention.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {

CellChange changeAt(const World& world, double x, double z) {
    CellChange change;
    if (world.interventions.overlay.tiles.empty()) return change;
    const double cell = specOf(Level::Site).cell;
    const int64_t gx = int64_t(std::floor(x / cell)), gz = int64_t(std::floor(z / cell));
    const TileKey key{Level::Site, int32_t(std::floor(double(gx) / TileCells)), int32_t(std::floor(double(gz) / TileCells))};
    change.tile = editedTile(world, key);
    if (!change.tile) return change;
    change.at = size_t(gz - int64_t(key.z) * TileCells) * TileCells + size_t(gx - int64_t(key.x) * TileCells);
    const Ground& was = change.tile->pristine->now;
    change.rock = change.tile->now.rock[change.at] - was.rock[change.at];
    for (int c = 0; c < SedimentClasses; ++c) change.cover[c] = change.tile->now.sediment[c][change.at] - was.sediment[c][change.at];
    change.datum = change.tile->now.datum[change.at] - was.datum[change.at];
    if (!change.tile->pile.empty()) {
        const LivePile& laid = change.tile->pile[change.at];
        const float thickness = laid.thickness(change.tile->now.rock[change.at], change.tile->now.datum[change.at]);
        if (thickness > 0) {
            // Play keeps only its lava apart; whatever else it laid counts as the fall around its vent, dry.
            const float lava = laid.laid > 0 ? std::min(1.0f, laid.lava / laid.laid) : 0.0f;
            change.pile = {thickness, lava, laid.kind, laid.vent, 0.0f, 1.0f - lava, 0.0f};
        }
    }
    return change;
}

bool changeLattice(const World& world, GroundLattice& lattice) {
    bool moved = false;
    for (int z = 0; z < lattice.countZ; ++z)
        for (int x = 0; x < lattice.countX; ++x) {
            const CellChange change = changeAt(world, double(lattice.originX + x) * lattice.spacing + lattice.shift, double(lattice.originZ + z) * lattice.spacing + lattice.shift);
            if (!change.tile || (change.rock == 0 && change.ground() == 0 && change.datum == 0 && change.pile.thickness == 0)) continue;
            const size_t node = size_t(z) * size_t(lattice.countX) + size_t(x);
            lattice.rock[node] += change.rock;
            lattice.datum[node] += change.datum;
            for (int c = 0; c < SedimentClasses; ++c) lattice.cover[c][node] = std::max(0.0f, lattice.cover[c][node] + change.cover[c]);
            // Lava laid in play lies on whatever pile was there; its kind is the top's.
            if (change.pile.thickness > 0) {
                if (lattice.pile.empty()) lattice.pile.assign(lattice.rock.size(), PilePoint{});
                PilePoint& pile = lattice.pile[node];
                // What stands of the untouched pile: less what the session cut from the rock before laying.
                const float below = std::clamp(pile.thickness + change.rock - change.datum - change.pile.thickness, 0.0f, pile.thickness), total = below + change.pile.thickness;
                const auto mean = [&](float was, float now) { return (was * below + now * change.pile.thickness) / total; };
                // Play records no flow's thickness: what stands of the untouched pile keeps its own.
                pile = {total, mean(pile.lava, change.pile.lava), change.pile.rock, change.pile.vent, mean(pile.flows, change.pile.flows), mean(pile.fall, change.pile.fall), mean(pile.wet, change.pile.wet), below > 0 ? pile.lavaUnit : 0.0f};
            }
            moved = true;
        }
    return moved;
}

void overlayLive(const World& world, Vec2 position, SurfaceSample& sample) {
    if (!sample.found) return;
    if (sample.level == Level::Site) overlaySurface(world, position, sample);
    const CellChange change = sample.level == Level::Ground ? changeAt(world, position.x, position.z) : CellChange{};
    if (change.tile) {
        // Whatever stands on the cover (a spring's deposit) stays on it.
        const float above = sample.ground - sample.rock - sample.sediment[0] - sample.sediment[1] - sample.sediment[2];
        sample.rock += change.rock;
        sample.datum += change.datum;
        sample.ground = sample.rock + above;
        for (int c = 0; c < SedimentClasses; ++c) {
            sample.sediment[c] = std::max(0.0f, sample.sediment[c] + change.cover[c]);
            sample.ground += sample.sediment[c];
        }
        // Live water is followed by site cell; where a cell holds none and was not touched, the
        // pools and streams the ground level found there still stand.
        if (change.tile->depth[change.at] > OpenWater) sample.water = change.tile->water[change.at];
        else if (change.rock != 0 || change.ground() != 0) sample.water = sample.ground - 1.0f;
        return;
    }
    // Sea over untouched ground stands at the level its tile was built for. The sea only moves once
    // an opening drains it; live tiles follow it by themselves.
    const float built = float(world.ocean.pristineLevel);
    if (world.ocean.openings.empty() || sample.water <= sample.ground || sample.water > built + 0.5f || editedTile(world, tileAt(Level::Site, position))) return;
    const float now = float(seaLevelNear(world.ocean, planetPoint(world.options.seed, position)));
    if (std::abs(now - built) > 1e-3f) sample.water = now > sample.ground ? now : sample.ground - 1.0f;
}

} // namespace derivean::sim
