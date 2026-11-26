#include "landscape/fn/Build.hpp"

#include "ground/type/SpringGround.hpp"

namespace derivean::sim {

std::vector<TileKey> groundwaterRead(TileKey key) {
    if (key.level != Level::Ground) return {};
    const SpringBounds bounds = groundSpringBounds(key);
    const TileKey low = tileAt(Level::Local, bounds.low);
    const TileKey high = tileAt(Level::Local, bounds.high);
    std::vector<TileKey> locals;
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x) locals.push_back({Level::Local, x, z});
    return locals;
}

} // namespace derivean::sim
