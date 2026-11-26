#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// The path a chain of the parent's tunnel stretches took through a finer domain: world points in
// order downstream, each with the stretch (index into the domain's tunnels) it belongs to, and per
// step to the next point whether the stream left a ridge there.
struct EskerTrack {
    std::vector<Vec2> points;
    std::vector<int32_t> stretch;
    std::vector<bool> open;   // one fewer than points
};

// m³ of the ridge of a tunnel stretch that crosses a finer tile's edge, by the parent cell (on the
// parent's grid) the stretch starts in and the tile on the other side (-1, 0 or 1 per axis from
// the recording tile; 0, 0 where it does not matter).
struct EskerCrossing {
    int64_t x{}, z{};
    int8_t tx{}, tz{};
    double volume{};
};

// The tunnel fills of a finer domain while the last sheet passes: the ridge they will stand as and
// the gravel and sand set aside for it from the cover the parent spread them in.
struct EskerFill {
    std::vector<float> ridge;   // m per domain cell
    double coarseShare{};       // of what was set aside, the share that is blocks and gravel; the rest is sand
    double ownedTaken{};        // m·cells of it set aside from the tile's own cells
    std::vector<EskerCrossing> arrived, sent;   // as SheetRecord::eskerArrived and eskerSent
};

} // namespace derivean::sim
