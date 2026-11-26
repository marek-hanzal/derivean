#include "volcanic/fn/Proving.hpp"

#include "landscape/type/Terrain.hpp"
#include "measure/constant/Angle.hpp"

namespace derivean::sim {

HeightGrid gridOf(const Domain& domain) {
    HeightGrid grid{domain.size, domain.size, domain.cell, domain.origin, std::vector<float>(domain.cells()), std::vector<float>(domain.cells(), 0.0f)};
    const VolcanicDomain& volcanic = domain.volcanic;
    for (size_t i = 0; i < domain.cells(); ++i) {
        grid.top[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        if (!volcanic.floor.empty()) grid.pile[i] = std::max(0.0f, domain.ground.rock[i] - (domain.ground.datum[i] - volcanic.floor[i]));
    }
    return grid;
}

EdificeShape measureEdifice(const HeightGrid& grid, Vec2 site, double reach) {
    EdificeShape shape;
    const int rings = std::max(4, int(reach / grid.cell));
    std::vector<double> sum(size_t(rings) + 1, 0.0), highest(size_t(rings) + 1, -1e30);
    std::vector<int> count(size_t(rings) + 1, 0);
    const int x0 = std::max(0, int((site.x - reach - grid.origin.x) / grid.cell) - 1), x1 = std::min(grid.width - 1, int((site.x + reach - grid.origin.x) / grid.cell) + 1);
    const int z0 = std::max(0, int((site.z - reach - grid.origin.z) / grid.cell) - 1), z1 = std::min(grid.height - 1, int((site.z + reach - grid.origin.z) / grid.cell) + 1);
    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            const size_t i = size_t(z) * size_t(grid.width) + size_t(x);
            const int ring = int(length(Vec2{grid.origin.x + (x + 0.5) * grid.cell, grid.origin.z + (z + 0.5) * grid.cell} - site) / grid.cell);
            if (ring > rings) continue;
            sum[size_t(ring)] += grid.top[i], ++count[size_t(ring)];
            highest[size_t(ring)] = std::max(highest[size_t(ring)], double(grid.top[i]));
            shape.pile += grid.pile[i] * grid.cell * grid.cell;
        }
    }
    std::vector<double> mean(size_t(rings) + 1, 0.0);
    for (int ring = 0; ring <= rings; ++ring) mean[size_t(ring)] = count[size_t(ring)] ? sum[size_t(ring)] / count[size_t(ring)] : (ring ? mean[size_t(ring) - 1] : 0.0);
    shape.base = mean[size_t(rings)];
    int crest = 0;
    for (int ring = 0; ring <= rings; ++ring)
        if (mean[size_t(ring)] > mean[size_t(crest)]) crest = ring;
    shape.summit = *std::max_element(highest.begin(), highest.end());
    shape.height = mean[size_t(crest)] - shape.base;
    shape.pit = mean[size_t(crest)] - mean[0];
    shape.pitRadius = crest * grid.cell;
    if (shape.height <= 0) return shape;
    const auto slopeAt = [&](int ring) { return std::atan((mean[size_t(ring)] - mean[size_t(ring) + 1]) / grid.cell) * DegreesPerRadian; };
    const auto ringAt = [&](double share) {
        int ring = crest;
        while (ring < rings - 1 && mean[size_t(ring)] - shape.base > share * shape.height) ++ring;
        return std::max(crest, ring - 1);
    };
    for (int ring = crest; ring < rings && mean[size_t(ring)] - shape.base > 0.66 * shape.height; ++ring) shape.topSlope = std::max(shape.topSlope, slopeAt(ring));
    shape.midSlope = slopeAt(ringAt(0.5));
    shape.footSlope = slopeAt(ringAt(0.1));
    shape.radius = (ringAt(0.05) + 1) * grid.cell;
    return shape;
}

} // namespace derivean::sim
