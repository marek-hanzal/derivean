#include "volcanic/fn/Proving.hpp"

#include "landscape/type/Terrain.hpp"
#include "measure/constant/Angle.hpp"

namespace derivean::sim {

EdificeShape measureEdifice(const Domain& domain, Vec2 site, double reach) {
    EdificeShape shape;
    const int rings = std::max(4, int(reach / domain.cell));
    std::vector<double> sum(size_t(rings) + 1, 0.0), highest(size_t(rings) + 1, -1e30);
    std::vector<int> count(size_t(rings) + 1, 0);
    const VolcanicDomain& volcanic = domain.volcanic;
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const double distance = length(domain.center(x, z) - site);
            const int ring = int(distance / domain.cell);
            if (ring > rings) continue;
            const double top = domain.ground.rock[i] + domain.sedimentAt(i);
            sum[size_t(ring)] += top, ++count[size_t(ring)];
            highest[size_t(ring)] = std::max(highest[size_t(ring)], top);
            if (!volcanic.floor.empty()) shape.pile += std::max(0.0f, domain.ground.rock[i] - (domain.ground.datum[i] - volcanic.floor[i])) * domain.cell * domain.cell;
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
    shape.pitRadius = crest * domain.cell;
    if (shape.height <= 0) return shape;
    const auto slopeAt = [&](int ring) { return std::atan((mean[size_t(ring)] - mean[size_t(ring) + 1]) / domain.cell) * DegreesPerRadian; };
    const auto ringAt = [&](double share) {
        int ring = crest;
        while (ring < rings - 1 && mean[size_t(ring)] - shape.base > share * shape.height) ++ring;
        return std::max(crest, ring - 1);
    };
    for (int ring = crest; ring < rings && mean[size_t(ring)] - shape.base > 0.66 * shape.height; ++ring) shape.topSlope = std::max(shape.topSlope, slopeAt(ring));
    shape.midSlope = slopeAt(ringAt(0.5));
    shape.footSlope = slopeAt(ringAt(0.1));
    shape.radius = (ringAt(0.05) + 1) * domain.cell;
    return shape;
}

} // namespace derivean::sim
