#pragma once

#include "core/type/Vec.hpp"
#include "icesheet/type/Continental.hpp"
#include "icesheet/type/IceGrid.hpp"
#include "landscape/type/Level.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace derivean::sim {

// What continental ice did to a tile, for the levels below it and for tools. Arrays have
// TileCells² entries, row-major; a tile no sheet ever reached carries no record at all.
// The thickness of the ice itself is in `Tile::ice` (now) and `Tile::iceAge` (last maximum).
struct SheetRecord {
    std::vector<float> slideX, slideZ;   // m/yr the ice of the last maximum slid over the bed; zero where frozen or bare
    std::vector<float> worn;             // m of rock the sheets of the replayed window ground off
    // m of each deposit of the last sheet. All of it lies in `Tile::now.sediment`; this says what it is.
    std::vector<float> till;             // lodged under the ice
    std::vector<float> moraine;          // dropped where the ice ended
    std::vector<float> outwash;          // laid by meltwater beyond the ice
    std::vector<float> esker;            // laid in a stream under the ice
    std::vector<float> meltwater;        // m³/yr of water under the ice of the last maximum
    std::vector<uint8_t> meltTo;         // neighbour index 0..7 that water leaves by, 255 for none
    // Finer levels only: m of debris the last sheet carried on down-ice from each cell. A later
    // neighbour's ice takes it up where it crosses into that neighbour, so the two agree at the seam.
    std::vector<float> passing;
};

// One stage's sheet on the cells of a domain, read off the coarse relaxation over the domain's own ground.
struct SheetField {
    std::vector<float> thickness;        // m of grounded ice; zero where bare or afloat
    std::vector<float> top;              // m, ice surface, or the ground (the sea over a sea floor) where there is none
    std::vector<float> fallX, fallZ;     // surface slope of the sheet, pointing uphill
    std::vector<float> slide;            // m/yr over the bed
    std::vector<float> melt;             // m/yr of water reaching the bed
};

// What a domain cannot find for itself: the sheet beyond its rim, the sink of the crust under the
// whole sheet and the heat from below. A world answers from its planet-wide stages
// (icesheet/type/Continental.hpp); a hand-built domain is given functions of its own.
struct SheetSource {
    std::vector<double> cooling;                                 // °C against the present per stage, warmest first
    std::function<float(int stage, Vec2 position)> thickness;    // m of ice the stage holds at a place
    std::function<float(int stage, Vec2 position)> sink;         // m the crust has given way there
    std::function<float(Vec2 position)> heat;                    // K/m in the rock
    std::function<float(Vec2 position)> halfRange;               // °C between the annual mean and the warmest month

    int stages() const { return int(cooling.size()); }
};

// The ice of a domain while it is replayed.
struct SheetWork {
    SheetSource source;
    bool reached{};                      // some stage brings ice into the domain
    int worked{};                        // steps whose glaciations have been worked into the ground
    std::vector<IceGrid> coarse;         // per stage, on cells four domain cells wide; empty until first solved
    std::vector<std::vector<float>> beyond;   // per stage and coarse cell: m of ice the source holds there
    std::vector<std::vector<float>> sunk;     // per stage and coarse cell: m the source's crust has given way there
    std::vector<float> worn;             // m of rock ground off so far, per domain cell
    std::vector<float> now, lastMaximum; // m of grounded ice per domain cell, once the last sheet has left its record
    std::vector<float> rest;             // m each cell's crust has still to rise since the last sheet left
    // Per domain cell while the last sheet leaves its record; the tile's cells are read off at the end.
    SheetRecord left;
};

inline constexpr int SheetCoarsening = 4;   // domain cells per side of a cell of the coarse relaxation

} // namespace derivean::sim
