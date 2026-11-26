#include "landscape/fn/Build.hpp"

#include "ground/type/SpringGround.hpp"

#include <algorithm>

namespace derivean::sim {

std::vector<TileKey> groundwaterRead(TileKey key) {
    if (key.level != Level::Ground) return {};
    const double span = tileSpan(key.level);
    const TileKey low = tileAt(Level::Local, {key.x * span - SpringReach, key.z * span - SpringReach});
    const TileKey high = tileAt(Level::Local, {(key.x + 1) * span + SpringReach, (key.z + 1) * span + SpringReach});
    std::vector<TileKey> locals;
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x) locals.push_back({Level::Local, x, z});
    return locals;
}

std::vector<TileKey> groundwaterSources(TileKey local) {
    std::vector<TileKey> sources;
    const auto around = [&](TileKey key) {
        for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 1; ++dx) {
                const TileKey other{key.level, key.x + dx, key.z + dz};
                if (std::find(sources.begin(), sources.end(), other) == sources.end()) sources.push_back(other);
            }
    };
    around(local);
    for (const int dz : {-1, 1})
        for (const int dx : {-1, 1}) around(parentOf({Level::Local, local.x + dx, local.z + dz}));
    return sources;
}

} // namespace derivean::sim
