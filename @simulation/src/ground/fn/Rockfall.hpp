#pragma once

#include "ground/type/Fall.hpp"

#include <cstdint>

namespace derivean::sim {

class Workers;

inline constexpr int MostBlocksPerSource = 40;

// Releases the blocks of every source and follows each down the surface until it stops: it gains
// speed with the drop, loses it to friction that is larger for a small block on rough ground and
// to the blow where the slope eases, and can be caught between pieces of its own size. A block is
// one lumped mass; it does not break.
// Blocks that would leave the grid or travel farther than `farthest` from their source are not
// followed and are left out.
// Each block reads the surface alone, so sources are followed side by side on `workers` where one is
// given; the blocks come back in the order of their sources either way.
std::vector<FallenBlock> shedBlocks(uint64_t seed, const FallGrid& surface, const std::vector<FallSource>& sources, double farthest, Workers* workers = nullptr);

} // namespace derivean::sim
