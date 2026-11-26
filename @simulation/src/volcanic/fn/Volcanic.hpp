#pragma once

#include "landscape/fn/Build.hpp"
#include "volcanic/fn/Emplace.hpp"

namespace derivean::sim {

// The step itself, `erupt`, is declared with the other processes in landscape/fn/Process.hpp.

// Begin.cpp — what a domain starts with: a region finds its vents and their histories in the
// planet's record; a finer level takes the pile, the vents and the conduits of its parent; every
// level takes the pile its finished neighbours ended with under the cells they own.
void beginVolcanic(Domain& domain, const Setting& setting, const TileLookup& lookup);
// Sizes the per-cell arrays the first time something volcanic touches the domain.
void allocateVolcanic(Domain& domain);

// Lay.cpp — what the vents of one step laid, cell by cell, before it becomes ground.
struct Laying {
    std::vector<float> lava, fall, ash;   // m
    std::vector<float> drop;              // m the cell founders
    std::vector<uint16_t> vent;            // index of the vent the top of it came from
    bool any{};
};
// The domain's ground as the emplacement laws see it.
Emplacement fieldOf(const Domain& domain);
// What comes out of a vent at one go, in m³ by the way it travels.
struct Outburst {
    Vec2 site;
    double fall{};       // fragments that fall around the vent
    double lava{};
    double flows{};      // currents of hot ash
    double ash{};        // carried off by the wind
    double founders{};   // volume by which the chamber's roof goes down afterwards
    Rheology rheology;   // of the lava
    Rheology current;    // of the currents of ash
    int pulses{1};       // parts in which fall and lava alternate
};
void emplace(Domain& domain, Emplacement& field, Laying& laying, int vent, const Outburst& outburst);
// Makes ground of it: seals the loose cover under lava and thick fall, starts or raises the pile,
// leaves thin ash as loose cover, lowers what foundered. A region then lets the plate sink under
// the load; a finer level takes the new surface as the one it is held to.
void settleLaying(Domain& domain, const Laying& laying);

// Pile.cpp — the traits of the rock a cell's surface lies in, where that is a pile or a conduit's
// plug; false where it is the old layering. Called by refreshUnits before its own work.
bool volcanicUnit(Domain& domain, size_t cell, uint64_t seed);
// What a finished tile keeps; null where the tile holds nothing volcanic.
// It first blends the pile along the edges of finished neighbours (Seams.cpp `blendPileSeams`), as
// the surface over it was blended.
std::shared_ptr<const VolcanicTile> volcanicOf(Domain& domain);
void blendPileSeams(Domain& domain);
// 0..1, share of lava in what was laid into a cell's pile; 1 where nothing is recorded.
inline float lavaShare(const VolcanicDomain& volcanic, size_t cell) {
    return volcanic.allLaid[cell] > 0 ? std::clamp(volcanic.lavaLaid[cell] / volcanic.allLaid[cell], 0.0f, 1.0f) : 1.0f;
}
// Readers of rock.
PileColumn pileColumn(const Tile& tile, size_t cell);
// `height` is metres above the pile's floor, which lies `column.thickness` under the rock surface.
PileBed pileBedAt(uint64_t seed, const VolcanicTile& volcanic, const PileColumn& column, float height);
// The same for a reader that crosses tile edges (the voxel column).
PilePoint pilePoint(const Tile& tile, size_t cell);
PileBed pileBed(uint64_t seed, const PilePoint& point, float height);
// A sheet of lava cools from its top and its base, and shrinks as it cools: it cracks into upright
// columns, the wider the thicker the sheet (the slower it cooled). In sound rock they stay shut;
// within the weathered skin of an exposed face, `depth` metres deep, each column has weathered back
// by its own amount and the joints between wide columns stand open. True where a point is open air
// for that reason. `step` is the spacing of the points drawn: columns and joints narrower than a
// few points are not drawn, so they never alias into noise.
bool columnOpen(uint64_t seed, const PilePoint& point, const PileBed& bed, double x, double z, float depth, float step);
// The plug of a conduit: it cuts the pile and the old beds alike, at any depth. Air outside every conduit.
Material neckAt(const VolcanicTile& volcanic, Vec2 position);

} // namespace derivean::sim
