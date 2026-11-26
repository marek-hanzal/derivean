#pragma once

#include "Arguments.hpp"

#include "groundwater/type/Groundwater.hpp"
#include "measure/type/Mosaic.hpp"

// The ground water of a square block of local tiles, with the ground it lies under.
struct WaterBlock {
    derivean::sim::Mosaic ground;
    std::vector<std::shared_ptr<const derivean::sim::GroundwaterTile>> tiles;   // row-major, ground.tiles per side
};

// GroundwaterMaps.cpp — `<out>/groundwater-<tag>-table.png` (depth to water: blue at the surface
// through green and yellow to red at 150 m, open water dark), `-springs.png` (vents on shaded
// relief: table white, contact yellow, fault red, karst magenta; larger with discharge) and
// `-conduits.png` (soluble ground tinted, passages black, abandoned levels orange, shafts red).
void writeWaterMaps(const WaterBlock& block, const std::string& out, const std::string& tag);

// GroundwaterSeek.cpp — a place of one kind, found on the planet as the landscapes read it:
// "limestone" for thick carbonate rock standing high in the rain, "hot" for the hottest ground with rain.
derivean::sim::Vec2 seekWaterPlace(derivean::sim::World& world, const std::string& kind);

// GroundwaterTiming.cpp — what one local tile's ground water costs, stage by stage, on the calling
// thread alone and with the pool.
void timeGroundwater(derivean::sim::World& world, derivean::sim::TileKey local);
