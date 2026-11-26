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

inline constexpr uint16_t NoVent = 0xFFFF;   // a vent index naming no vent; a region lists one vent per centre and epoch, hundreds

// What a finished tile keeps of it. Arrays have TileCells² entries, row-major.
struct VolcanicTile {
    std::vector<float> pile;     // m of volcanic rock under the rock surface: it fills (rock − pile, rock]
    std::vector<float> laid;     // yr before the tile's time that the top of the pile was laid
    std::vector<uint8_t> lava;   // share of lava in the pile in parts of 255; the rest is ash and scoria
    std::vector<uint16_t> vent;   // index into `vents` of the vent that laid the top; NoVent for none
    std::vector<Vent> vents;     // every vent whose products or conduits reach the tile
    VolcanicLedger ledger;
};

// The same while a tile is replayed. Arrays have one entry per cell of the domain; they are empty
// until the first vent or the first inherited pile needs them.
struct VolcanicDomain {
    std::vector<Vent> vents;
    std::vector<float> floor;      // m below the datum of the pile's floor; there is a pile where the rock surface stands above it
    std::vector<float> lavaLaid;   // m of lava ever laid into the pile
    std::vector<float> allLaid;    // m of everything ever laid into it
    std::vector<float> laidAt;     // yr after the domain's start that the top was laid
    std::vector<uint16_t> vent;     // index into `vents`, NoVent for none
    std::vector<uint16_t> plug;     // 1 + index of the vent whose conduit comes up through the cell, 0 for none
    std::vector<uint8_t> inside;   // 1 while the cell's cached rock traits are the pile's or a plug's
    VolcanicLedger ledger;
};

inline constexpr float NoPile = -1e9f;   // a floor that far above the datum: nothing stands on it

// One cell of a tile's pile, as a reader of rock needs it.
struct PileColumn {
    float thickness{};   // m
    float laid{};        // yr before the tile's time
    uint8_t lava{};      // parts of 255
    uint16_t vent{NoVent};
};

// The pile under one point of the ground, as a reader that crosses tile edges carries it: nothing
// in it refers to a tile's own list of vents.
struct PilePoint {
    float thickness{};   // m
    float lava{};        // 0..1, share of lava in it
    Material rock{Material::Basalt};   // what its lava is
    uint64_t vent{};     // the vent that laid its top
};

// One member of a pile: a lava flow or a bed of ash.
struct PileBed {
    float bottom{}, top{};   // m above the pile's floor
    Material rock{Material::Basalt};
    bool sheet{};            // lava that cooled as one sheet: its columns stand upright between bottom and top
};

} // namespace derivean::sim
