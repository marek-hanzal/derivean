#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// The path a chain of the parent's tunnel stretches took through a finer domain: world points in
// order downstream, each with the stretch (index into the domain's tunnels) it belongs to.
struct EskerTrack {
    std::vector<Vec2> points;
    std::vector<int32_t> stretch;
};

// The tunnel fills of a finer domain while the last sheet passes: the ridge they will stand as and
// the gravel and sand set aside for it from the cover the parent spread them in.
struct EskerFill {
    std::vector<float> ridge;   // m per domain cell
    double coarseShare{};       // of what was set aside, the share that is blocks and gravel; the rest is sand
    double ownedTaken{};        // m·cells of it set aside from the tile's own cells
};

} // namespace derivean::sim
