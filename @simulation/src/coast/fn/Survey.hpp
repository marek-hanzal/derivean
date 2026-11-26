#pragma once

#include "coast/type/Shore.hpp"
#include "measure/type/Distribution.hpp"

namespace derivean::sim {

// What stands at the water's edge. A shore cell is a dry cell with water beside it.
enum class ShoreKind : uint8_t {
    None,
    Cliff,       // bare rock rising CliffRise or more, at CliffSteep or steeper or from a foot the waves have cut
    Rock,        // bare rock that does neither
    Gravel,      // loose cover of which blocks and gravel are a CoarseShare or more
    Sand,
    Mud,
    Count,
};

inline constexpr const char* ShoreKindNames[size_t(ShoreKind::Count)] = {"", "cliff", "low rock", "gravel beach", "sand beach", "mud shore"};

inline constexpr float CliffRise = 10.0f;      // m above the water within CliffReach of the shore
inline constexpr float CliffSteep = 0.5f;      // rise over run from the waterline to that point
inline constexpr double CliffReach = 100.0;    // m, at least two cells
inline constexpr float BenchWater = 2.0f;      // m of water over bare rock up to which it is a platform
inline constexpr double StripWidth = 1500.0;   // m of loose ground between two waters up to which it is a spit or a barrier
inline constexpr float Sheltered = 0.4f;       // share of the open water's waves below which the water behind a strip is a lagoon
// m of water, at high water, of the band the waves offshore are read in: the waves at the shore are
// as high as its depth lets them be, those before it say what the shore is exposed to.
inline constexpr float OffshoreShallow = 5.0f, OffshoreDeep = 15.0f;

// A square of cells as the coast's measurements read it, from a domain or from finished tiles.
struct ShoreGrid {
    int side{};
    double cell{};
    double seaLevel{};
    std::vector<float> rock, cover[SedimentClasses];
    std::vector<float> water;    // m, surface of standing water; NoWater where the cell is dry
    std::vector<float> wave;     // m, the highest train at the cell
    std::vector<float> notch;    // m
    std::vector<float> cut;      // m of rock the waves took

    size_t index(int x, int z) const { return size_t(z) * size_t(side) + size_t(x); }
    float loose(size_t i) const { return cover[0][i] + cover[1][i] + cover[2][i]; }
    float top(size_t i) const { return rock[i] + loose(i); }
    bool wet(size_t i) const { return water[i] != NoWater; }
};

ShoreGrid shoreGridOf(const Domain& domain);
// `tiles` × `tiles` finished tiles of one level, row-major; none may be null.
ShoreGrid shoreGridOf(const std::vector<const Tile*>& tiles, int perSide);

struct ShoreSurvey {
    // Shoreline is counted in cell edges between a dry and a wet cell, so a shore that runs
    // obliquely to the grid reads up to 1.41 times its true length.
    double sea[size_t(ShoreKind::Count)]{};    // m of sea shore by kind
    double lake[size_t(ShoreKind::Count)]{};   // m of lake shore by kind
    double benched{};                          // m of cliff and rock shore with a wave-cut platform before it
    double strip{};                            // m of shore on loose ground with other water within StripWidth behind it
    double lagoonShore{};                      // m of shore on water sheltered behind such a strip
    double lagoon{};                           // m² of that water
    int stacks{};                              // rock standing out of the water alone, no wider than four cells
    Distribution cliffHeight;                  // m above the water
    Distribution platformWidth;                // m, the bench narrower than a cell included
    Distribution beachWidth[2];                // m of dry beach inland of the waterline: sea, lake
    Distribution beachSlope[SedimentClasses];  // rise over run at the waterline by the class the beach is of
    Distribution struck;                       // m, highest train at shore cells
    Distribution offshore;                     // m, highest train over water OffshoreShallow to OffshoreDeep deep
    std::vector<uint8_t> kind;                 // ShoreKind per cell; None away from the water's edge
    std::vector<uint8_t> flag;                 // per cell: 1 platform, 2 strip, 4 lagoon, 8 stack
};

ShoreSurvey surveyShore(const ShoreGrid& grid);

} // namespace derivean::sim
