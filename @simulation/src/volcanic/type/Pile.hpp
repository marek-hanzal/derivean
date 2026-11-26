#pragma once

#include "material/constant/Materials.hpp"
#include "volcanic/type/Vent.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace derivean::sim {

// Rock that eruptions laid on top of the land does not belong to the layering under it. A cell
// holds it as a pile: volcanic rock from the pile's floor up to the rock surface, the old layering
// (read by depth below the datum, as everywhere) from the floor down. The floor is the surface the
// first lava or ash buried; it may lie below the datum, in a valley cut before, and the top above it.

// Totals in m³ of rock for the replayed window.
struct VolcanicLedger {
    double erupted{};     // brought up by the vents the domain replays
    double lava{}, cone{}, ash{};   // laid on the domain's evolving cells: lava, fall around the vents, ash
    double left{};        // lava and fall that left over the domain's edge or came to lie on ground a finished neighbour owns
    double drifted{};     // ash that fell beyond the domain or on such ground
    double sealed{};      // loose cover buried under lava and part of the pile since
    double foundered{};   // lowered into emptied chambers
    double built{};       // of lava, cone and ash: on the tile's own cells
};

// A caldera a region cut, as the region tile that holds its site records it. That tile is the one
// owner of how much of the pit is still there; a finer level cuts it again at its own cells from it.
struct Caldera {
    uint64_t vent{};
    Vec2 site;
    double volume{};     // m³ that left; sets the radius
    Magma magma{Magma::Basalt};
    double time{};       // yr the roof went down: the end of its step
    float cut{};         // m, the pit as that step's eruptions left it (fn/Caldera.cpp)
    float kept{};        // 0..1, the pit at the end of the region's window against `cut`
    float wall{1};       // how deep the surface dropped at the ring wall against the roof's block, one figure for
                         // the whole ring (fn/Caldera.cpp, from fn/Collapse.cpp's cut)
    // While the region replays: the surface as the roof went down, on the domain's cells of a
    // square around the caldera from (x, z), `side` cells wide, which the pit is read against.
    int x{}, z{}, side{};
    std::vector<float> before;
};

inline constexpr uint16_t NoVent = 0xFFFF;   // a vent index naming no vent; a region lists one vent per centre and epoch, hundreds

// What a finished tile keeps of it. Arrays have TileCells² entries, row-major.
struct VolcanicTile {
    std::vector<float> pile;     // m of volcanic rock under the rock surface: it fills (rock − pile, rock]
    std::vector<float> laid;     // yr before the tile's time that the top of the pile was laid
    std::vector<uint8_t> lava;   // share of lava in the pile in parts of 255; the rest is ash and scoria
    std::vector<uint8_t> flows;  // share of it the currents of hot ash laid, in parts of 255
    std::vector<uint8_t> fall;   // share of it that fell around the vents, in parts of 255
    std::vector<uint8_t> wet;    // share of it laid under standing water, sea or lake, in parts of 255
    std::vector<float> lavaUnit; // m, thickness of one flow of its lava, in the mean by what each laid; 0 for none recorded
    std::vector<uint16_t> vent;   // index into `vents` of the vent that laid the top; NoVent for none
    std::vector<Vent> vents;     // every vent whose products or conduits reach the tile
    // Pile and its shares over the tile and the Apron cells beyond its edge, (TileCells + 2·Apron)²,
    // as its own replay left them: what a later neighbour blends its seam towards (fn/Seams.cpp).
    std::vector<float> wide;
    std::vector<uint8_t> wideLava, wideFlows, wideFall, wideWet;
    std::vector<float> wideUnit;
    std::vector<Caldera> calderas;   // a region's: every caldera whose site lies in the tile
    VolcanicLedger ledger;
};

// The same while a tile is replayed. Arrays have one entry per cell of the domain; they are empty
// until the first vent or the first inherited pile needs them.
struct VolcanicDomain {
    std::vector<Vent> vents;
    std::vector<float> floor;      // m below the datum of the pile's floor; there is a pile where the rock surface stands above it
    std::vector<float> lavaLaid;   // m of lava ever laid into the pile
    std::vector<float> flowLaid;   // m of it the hot currents of chamber-emptying eruptions laid thick and dry enough to weld
    std::vector<float> fallLaid;   // m of it that fell around the vents, came in block-and-ash currents, or in hot ones
                                   // too thin or wet to weld; the rest of what is not lava is ash the wind carried
    std::vector<float> wetLaid;    // m of it laid under standing water
    std::vector<float> unitLaid;   // m², of its lava: each flow's m laid times the thickness that flow came to rest at
    std::vector<float> allLaid;    // m of everything ever laid into it
    std::vector<float> laidAt;     // yr after the domain's start that the top was laid
    std::vector<uint16_t> vent;     // index into `vents`, NoVent for none
    std::vector<uint16_t> plug;     // 1 + index of the vent whose conduit comes up through the cell, 0 for none
    std::vector<uint8_t> inside;   // 1 while the cell's cached rock traits are the pile's or a plug's
    std::vector<Caldera> calderas; // a region's: every caldera it cut, as it is cut
    VolcanicLedger ledger;
};

// What a pile is made of, as shares of all that was laid into it. `lavaUnit` is the thickness of
// one of its flows times the share of lava, so that it blends as the shares do: the thickness
// itself is `lavaUnit / lava`, and none is recorded where it is 0.
struct PileShares {
    float lava{1}, flows{}, fall{}, wet{};
    float lavaUnit{};   // m
};
inline PileShares operator*(const PileShares& shares, float by) { return {shares.lava * by, shares.flows * by, shares.fall * by, shares.wet * by, shares.lavaUnit * by}; }
inline PileShares operator+(const PileShares& a, const PileShares& b) { return {a.lava + b.lava, a.flows + b.flows, a.fall + b.fall, a.wet + b.wet, a.lavaUnit + b.lavaUnit}; }
inline constexpr PileShares NoShares{0, 0, 0, 0, 0};

inline constexpr float NoPile = -1e9f;   // a floor that far above the datum: nothing stands on it
inline constexpr float ThinnestPile = 0.02f;   // m of pile a finer level inherits or a seam blends below which a cell has none

// One cell of a tile's pile, as a reader of rock needs it.
struct PileColumn {
    float thickness{};   // m
    float laid{};        // yr before the tile's time
    uint8_t lava{};      // parts of 255
    uint16_t vent{NoVent};
    uint8_t flows{}, fall{}, wet{};   // parts of 255, as `PileShares`
    float lavaUnit{};    // m, thickness of one flow of its lava; 0 for none recorded
};

// The pile under one point of the ground, as a reader that crosses tile edges carries it: nothing
// in it refers to a tile's own list of vents.
struct PilePoint {
    float thickness{};   // m
    float lava{};        // 0..1, share of lava in it
    Material rock{Material::Basalt};   // what its lava is
    uint64_t vent{};     // the vent that laid its top
    float flows{}, fall{}, wet{};   // 0..1, as `PileShares`
    float lavaUnit{};    // m, thickness of one flow of its lava; 0 for none recorded
};

// One member of a pile: a lava flow or a bed of what the eruptions threw out.
struct PileBed {
    float bottom{}, top{};   // m above the pile's floor
    Material rock{Material::Basalt};
    bool sheet{};            // cooled as one sheet, lava or welded tuff: its columns stand upright between bottom and top
};

} // namespace derivean::sim
