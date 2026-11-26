#pragma once

#include "measure/fn/Face.hpp"
#include "measure/type/Distribution.hpp"

namespace derivean::sim {

// Slope of the dry ground, by Horn's 3 × 3 gradient as a DEM of the same cell size would give it.
struct SlopeStats {
    size_t cells{};
    double mean{}, p10{}, p50{}, p90{}, p99{};   // degrees
    double above30{}, above45{}, above60{};      // shares of the dry cells
};

// Highest minus lowest surface inside square windows that step by half their width.
struct WindowRelief {
    double window{};   // m
    int count{};
    double p50{}, p90{}, high{};   // m
};

struct CoverShares {
    double share[size_t(Face::Count)]{};   // of all cells; snow is not told from the ground under it
    double meanCover{};                    // m of loose cover on dry ground
    Distribution talusAngle;               // degrees, steepest fall of cells under a metre or more of coarse debris
};

// Length of channel per area, for two sizes of stream.
struct DrainageStats {
    double brooks{};    // km/km² of cells carrying BrookFlow or more
    double streams{};   // km/km² of cells carrying the level's river threshold or more
};

inline constexpr double BrookFlow = 3e4;   // m³/yr, about a litre per second

// Cross-sections of the largest valleys, taken across the direction of flow.
struct ValleyStats {
    int sections{};
    Distribution floorWidth;     // m of ground within the level's floor rise of the stream
    Distribution wallHeight;     // m, the lower of the two sides
    Distribution wallAngle;      // degrees, floor edge to crest, mean of the two sides
    Distribution depthToWidth;   // wall height over the distance between the two crests
    double canyonShare{};        // share of sections at least a third as deep as they are wide
};

// Divides: cells nothing drains into that stand above both neighbours across the crest.
struct CrestStats {
    int crests{};
    // A surface cut by running water has almost no cell that stands above all eight of its
    // neighbours or below all of them: its high points are joined into crests, its low ones into valleys.
    double peakShare{}, pitShare{};   // of the dry cells
    double roughness{};               // median of |cell − mean of its eight neighbours| over the range of those neighbours
    double rmsNear{}, rmsWide{};      // m, rms of a cell's height against the mean of the 3 × 3 and of the 17 × 17 cells around it
    // Of the cells that stand above all eight neighbours: the median of how far the highest neighbour
    // lies below them over how far the lowest does. A knoll on a crest line reads near zero (the crest
    // goes on almost as high), a cone standing alone near one.
    double peakStanding{};
    Distribution flankAngle;   // degrees, fall within one cell of the crest
    // Fall within one cell of the crest over the fall of the three cells below it. Planes that meet
    // in an edge read 0.75 on average (the edge lies anywhere within the cell), a parabolic dome 0.2.
    Distribution sharpness;
    Distribution curvature;    // 1/m across the crest; positive for a convex top
};

// Connected faces whose steepest fall to a neighbour exceeds 60°.
struct CliffStats {
    int faces{};
    double perSquareKm{};
    double steepShare{};       // share of dry cells that are part of one
    Distribution height;       // m from the top of a face to the ground under it
    Distribution length;       // m along the face
};

struct ReliefReport {
    SlopeStats slope;
    WindowRelief relief[3];    // 1 km, 5 km and 16 km windows; count 0 where the mosaic is too small
    CoverShares cover;
    DrainageStats drainage;
    ValleyStats valleys;
    CrestStats crests;
    CliffStats cliffs;
};

} // namespace derivean::sim
