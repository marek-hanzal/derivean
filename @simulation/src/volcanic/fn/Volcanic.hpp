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
    std::vector<float> lava, fall, flows, ash;   // m
    std::vector<float> wet;                      // m of all of them laid under standing water
    std::vector<float> drop;              // m the surface of the cell founders
    std::vector<float> sink;              // m the layering under it goes down: the roof's block
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
    double founders{};   // volume by which the chamber's roof goes down afterwards (`collapse`)
    Rheology rheology;   // of the lava
    Rheology current;    // of the currents of ash
    int pulses{1};       // parts in which fall and lava alternate
};
void emplace(Domain& domain, Emplacement& field, Laying& laying, int vent, const Outburst& outburst);
// The roof going down once an outburst with `founders` has laid what it threw out.
void collapse(Domain& domain, Emplacement& field, Laying& laying, int vent, const Outburst& outburst);
// m: a dry bed of hot currents laid at once thinner than this cools before its shards weld;
// ash-flow sheets show a densely welded zone only once some ten to twenty metres thick (Smith 1960).
inline constexpr float Welding = 15.0f;
// Makes ground of it: seals the loose cover under lava and thick fall, starts or raises the pile,
// leaves thin ash as loose cover, lowers what foundered. A region then lets the plate sink under
// the load; a finer level takes the new surface as the one it is held to.
void settleLaying(Domain& domain, const Laying& laying);

// Caldera.cpp — a region records each caldera it cuts with the surface as the roof went down,
// measures its pit right after the step and at the end of its window; a local level cuts the ring
// fault again at its own cells as far as the region's pit is still there.
Caldera calderaOf(const Domain& domain, const Emplacement& field, const Vent& vent, const Episode& episode);
void recordCalderas(Domain& domain, std::vector<Caldera>&& cut);
std::vector<Caldera> finishCalderas(Domain& domain);
void sharpenCalderas(Domain& domain, const TileLookup& lookup);

// Pile.cpp — the traits of the rock a cell's surface lies in, where that is a pile or a conduit's
// plug; false where it is the old layering. Called by refreshUnits before its own work.
bool volcanicUnit(Domain& domain, size_t cell, uint64_t seed);
// What a finished tile keeps; null where the tile holds nothing volcanic.
// It first blends the pile along the edges of finished neighbours (Seams.cpp `blendPileSeams`), as
// the surface over it was blended. A region's keeps the calderas whose site lies in it (Caldera.cpp).
std::shared_ptr<const VolcanicTile> volcanicOf(Domain& domain);
void blendPileSeams(Domain& domain);
// Shares of what was laid into a cell's pile; all lava, dry, where nothing is recorded.
inline PileShares sharesAt(const VolcanicDomain& volcanic, size_t cell) {
    const float all = volcanic.allLaid[cell];
    if (all <= 0) return {};
    const auto share = [&](const std::vector<float>& part) { return std::clamp(part[cell] / all, 0.0f, 1.0f); };
    return {share(volcanic.lavaLaid), share(volcanic.flowLaid), share(volcanic.fallLaid), share(volcanic.wetLaid)};
}
inline float lavaShare(const VolcanicDomain& volcanic, size_t cell) { return sharesAt(volcanic, cell).lava; }
// A pile `pile` thick of these shares, as if it had all been laid at once.
inline void setShares(VolcanicDomain& volcanic, size_t cell, float pile, const PileShares& shares) {
    volcanic.allLaid[cell] = pile;
    volcanic.lavaLaid[cell] = pile * shares.lava;
    volcanic.flowLaid[cell] = pile * shares.flows;
    volcanic.fallLaid[cell] = pile * shares.fall;
    volcanic.wetLaid[cell] = pile * shares.wet;
}
inline uint8_t partsOf(float share) { return uint8_t(std::lround(255.0f * std::clamp(share, 0.0f, 1.0f))); }
// The same as a finished tile keeps them, at a cell of its own or, `wide`, of its record with the Apron.
inline PileShares sharesOf(const VolcanicTile& tile, size_t cell, bool wide = false) {
    const auto& lava = wide ? tile.wideLava : tile.lava;
    const auto& flows = wide ? tile.wideFlows : tile.flows;
    const auto& fall = wide ? tile.wideFall : tile.fall;
    const auto& wet = wide ? tile.wideWet : tile.wet;
    return {float(lava[cell]) / 255.0f, float(flows[cell]) / 255.0f, float(fall[cell]) / 255.0f, float(wet[cell]) / 255.0f};
}
// Readers of rock.
PileColumn pileColumn(const Tile& tile, size_t cell);
// `height` is metres above the pile's floor, which lies `column.thickness` under the rock surface.
PileBed pileBedAt(uint64_t seed, const VolcanicTile& volcanic, const PileColumn& column, float height);
// The same for a reader that crosses tile edges (the voxel column).
PilePoint pilePoint(const Tile& tile, size_t cell);
PileBed pileBed(uint64_t seed, const PilePoint& point, float height);
// Columns.cpp — the joints of lava that cooled in place.
// A sheet of lava cools from its top and its base, and shrinks as it cools: it cracks into upright
// columns, the wider the thicker the sheet (the slower it cooled). In sound rock they stay shut;
// within the weathered skin of an exposed face, `depth` metres deep, each column has weathered back
// by its own amount and the joints between wide columns stand open. True where a point is open air
// for that reason. `step` is the spacing of the points drawn: columns and joints narrower than a
// few points are not drawn, so they never alias into noise.
bool columnOpen(uint64_t seed, const PilePoint& point, const PileBed& bed, double x, double z, float depth, float step);
// The plug of a conduit: it cuts the pile and the old beds alike, at any depth. Air outside every conduit.
struct Neck {
    Material rock{Material::Air};
    Vec2 site;           // m, the conduit's axis
    float radius{};      // m
    uint64_t vent{};
};
Neck neckOf(const VolcanicTile& volcanic, Vec2 position);
// A plug cools from its wall and cracks into columns that lie across the conduit, the wider the
// wider it is; at an exposed face of a neck they weather as a flow's do. True where a point of
// the neck at height `y` and `depth` metres within its face is open air for that reason.
bool neckOpen(uint64_t seed, const Neck& neck, double x, double y, double z, float depth, float step);

} // namespace derivean::sim
