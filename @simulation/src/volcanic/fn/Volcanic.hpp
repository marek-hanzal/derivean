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
    std::vector<uint8_t> vent;            // index of the vent the top of it came from
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
std::shared_ptr<const VolcanicTile> volcanicOf(const Domain& domain);
// Readers of rock.
PileColumn pileColumn(const Tile& tile, size_t cell);
// `height` is metres above the pile's floor, which lies `column.thickness` under the rock surface.
PileBed pileBedAt(uint64_t seed, const VolcanicTile& volcanic, const PileColumn& column, float height);
// The plug of a conduit: it cuts the pile and the old beds alike, at any depth. Air outside every conduit.
Material neckAt(const VolcanicTile& volcanic, Vec2 position);

} // namespace derivean::sim
