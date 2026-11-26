#pragma once

#include "measure/type/Mosaic.hpp"

#include <string>
#include <vector>

namespace derivean::sim {

// Where to put a camera. Angles in degrees: yaw 0 looks north (-Z) and grows turning left towards
// the west, as the game's controls do; pitch is positive looking up.
struct CameraPose {
    double x{}, y{}, z{};
    double yaw{}, pitch{};
};

// One place worth showing, with the numbers that make it so.
struct ScoutSite {
    Vec2 at;              // m
    float ground{};       // m, at `at`, from `level`
    Level level{Level::Region};
    double score{};       // larger is better within one kind of site
    std::string reason;
    Vec2 stand;           // where a viewer on the ground sees it from
    float lookUp{};       // m above the ground at `at` that the viewer should aim for
    CameraPose groundShot, airShot;
};

} // namespace derivean::sim
