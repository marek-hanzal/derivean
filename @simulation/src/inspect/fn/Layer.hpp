#pragma once

#include "simulation/type/World.hpp"

#include <string>
#include <vector>

namespace derivean::sim {

// The inspector's fields as maps over a block of finished tiles, one pixel per cell.
//   cover       m of loose sediment, logarithmic
//   discharge   m³/yr, logarithmic
//   erosion     m/yr now, logarithmic       deposition  the same for what is laid down
//   uplift      m/yr of rock uplift now: blue sinking, red rising
//   rain        m/yr
//   plates      plate under each point, contacts coloured by what they do, arrows of motion against the neighbour
//   crust       km of felsic crust          crust-age   yr since the lid solidified, logarithmic
// Returns false for a name that is none of these and leaves the image alone. `image` must hold
// (tiles · TileCells)² RGB pixels; `legend` says what the colours span.
bool paintLayer(const World& world, const std::string& name, TileKey first, int tiles, std::vector<uint8_t>& image, std::string& legend);

} // namespace derivean::sim
