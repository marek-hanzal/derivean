#pragma once

#include "ground/type/Clast.hpp"
#include "landscape/type/Level.hpp"

#include <vector>

namespace derivean::sim {

// Mineral a spring has laid on the ground.
enum class Crust : uint8_t { None, Sinter, Travertine };

// Where the volume of the coarse layer went when its larger pieces became stones, in m³ of solid
// over the tile's own cells. `coarseBefore + dug` equals `matrix + stones` up to rounding.
struct ClastLedger {
    double coarseBefore{};   // loose coarse layer the settling left
    double dug{};            // coarse waste that falling water dug out of soft rock under its pools
    double matrix{};         // what remains as gravel, scree or till between and under the stones
    double stones{};         // the stones themselves
    double offered{};        // stones the processes delivered, including those the layer could not pay for
    int byOrigin[ClastOrigins]{};
};

inline constexpr int ClastBins = 16;                                  // index cells along one side of a tile
inline constexpr double ClastBinSpan = TileCells * 0.25 / ClastBins;  // m

// What a ground tile holds beside its surface. Per-cell arrays have TileCells² entries, row-major.
struct GroundDetail {
    std::vector<Clast> clasts;            // every stone whose centre lies in the tile, sorted by index cell
    std::vector<uint32_t> binStart;       // ClastBins² + 1 offsets into `clasts`
    std::vector<float> crust;             // m of spring deposit on top of the ground
    std::vector<uint8_t> crustKind;       // Crust
    std::vector<uint8_t> coarseKind;      // Material the coarse layer reads as: Gravel, Scree or Till
    std::vector<uint8_t> fineKind;        // Material of the fine layer: Silt, or Clay where hot water altered it
    std::vector<uint8_t> bare;            // 1 where the fine layer carries no soil: washed, frozen, scalded or drowned
    std::vector<float> stepGaps;          // m along its channel from each step of a stream bed to the next one below
    ClastLedger ledger;
};

} // namespace derivean::sim
