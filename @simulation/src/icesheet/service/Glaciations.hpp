#pragma once

#include "icesheet/type/Continental.hpp"
#include "landscape/type/Setting.hpp"

#include <memory>

namespace derivean::sim {

// The planet-wide ice of a world, solved once when its first tile asks and kept while the process
// lives. It is a function of the planet and the age alone, so every tile reads the same sheets
// whichever is built first. Null for a setting whose planet has no relief (hand-built domains).
std::shared_ptr<const ContinentalIce> continentalIce(const Setting& setting);

} // namespace derivean::sim
