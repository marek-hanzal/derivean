#pragma once

#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

enum class EditKind : uint8_t {
    Lower,   // cut the ground down to the stroke's height where it stands above it
    Raise,   // build it up to the stroke's height where it lies below it
    Level,   // both
    Void,    // open a passage from the sea bed at `from` down into the void
};

// One change the game hands back, in simulation terms. A stroke is the band of `width` around the
// segment `from`–`to`; its height runs linearly between the two ends. A channel is a Lower stroke
// with a falling bed, a dam a Raise stroke with a level crest, a pit or a pad a short wide stroke,
// and one site cell is a stroke of one cell's width whose ends coincide at the cell's centre.
struct Edit {
    double time{};                     // calendar seconds; stamped when the edit is made
    EditKind kind{EditKind::Lower};
    Vec2 from, to;                     // m
    double width{};                    // m
    double heightFrom{}, heightTo{};   // m; for Void, heightFrom is the elevation of the mouth
    SedimentClass fill{Sand};          // what a raise is built of
    double area{};                     // m², throat of a void opening
};

inline Edit stroke(EditKind kind, Vec2 from, Vec2 to, double width, double heightFrom, double heightTo, SedimentClass fill = Sand) {
    return {0.0, kind, from, to, width, heightFrom, heightTo, fill, 0.0};
}

inline Edit siteCell(int64_t cellX, int64_t cellZ, double height, SedimentClass fill = Sand) {
    const double cell = specOf(Level::Site).cell;
    const Vec2 centre{(double(cellX) + 0.5) * cell, (double(cellZ) + 0.5) * cell};
    return {0.0, EditKind::Level, centre, centre, cell, height, height, fill, 0.0};
}

// Changes nothing, but makes the tiles around a point live: their water is then followed in time.
inline Edit watch(Vec2 at) {
    return {0.0, EditKind::Raise, at, at, 0.0, -1e30, -1e30, Sand, 0.0};
}

inline Edit voidOpening(Vec2 at, double area, double mouth) {
    return {0.0, EditKind::Void, at, at, 0.0, mouth, mouth, Sand, area};
}

} // namespace derivean::sim
