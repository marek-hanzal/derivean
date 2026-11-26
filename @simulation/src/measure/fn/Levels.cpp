#include "measure/fn/Levels.hpp"

#include <cmath>

namespace derivean::sim {

Mosaic gatherParents(World& world, const Mosaic& child) {
    const TileKey low = parentOf(child.first);
    const TileKey high = parentOf({child.level, child.first.x + child.tiles - 1, child.first.z + child.tiles - 1});
    return gatherMosaic(world, low, std::max(high.x - low.x, high.z - low.z) + 1);
}

LevelReport levelsOf(const Mosaic& parent, const Mosaic& child, double river) {
    LevelReport report;
    // Global cell index of the child's first cell, in parent cells and relative to the parent mosaic.
    const int64_t childX = int64_t(child.first.x) * TileCells, childZ = int64_t(child.first.z) * TileCells;
    const int offsetX = int(childX / Refinement - int64_t(parent.first.x) * TileCells);
    const int offsetZ = int(childZ / Refinement - int64_t(parent.first.z) * TileCells);
    const int count = child.side() / Refinement;
    std::vector<float> drift;
    double squares = 0, steps = 0;
    int stepCount = 0, nearFound = 0, wideFound = 0, agree = 0;

    const auto peak = [&](int cx, int cz, int reach) {
        float most = 0;
        for (int z = std::max(0, (cz - reach) * Refinement); z < std::min(child.side(), (cz + reach + 1) * Refinement); ++z)
            for (int x = std::max(0, (cx - reach) * Refinement); x < std::min(child.side(), (cx + reach + 1) * Refinement); ++x)
                most = std::max(most, child.discharge.at(x, z));
        return most;
    };

    for (int cz = 0; cz < count; ++cz) {
        for (int cx = 0; cx < count; ++cx) {
            const int px = offsetX + cx, pz = offsetZ + cz;
            if (!parent.ground.contains(px, pz)) continue;
            double sum = 0;
            int flooded = 0;
            for (int z = 0; z < Refinement; ++z)
                for (int x = 0; x < Refinement; ++x) {
                    sum += child.ground.at(cx * Refinement + x, cz * Refinement + z);
                    flooded += child.flooded(cx * Refinement + x, cz * Refinement + z);
                }
            const double difference = sum / (Refinement * Refinement) - parent.ground.at(px, pz);
            drift.push_back(float(difference));
            squares += difference * difference;
            agree += parent.flooded(px, pz) == (flooded * 2 > Refinement * Refinement);
            if (parent.ground.contains(px + 1, pz)) steps += std::abs(parent.ground.at(px + 1, pz) - parent.ground.at(px, pz)), ++stepCount;
            if (parent.ground.contains(px, pz + 1)) steps += std::abs(parent.ground.at(px, pz + 1) - parent.ground.at(px, pz)), ++stepCount;

            // A river two parent cells from the edge of the child mosaic can be looked for all around.
            if (parent.flooded(px, pz) || parent.discharge.at(px, pz) < river) continue;
            if (cx < 2 || cz < 2 || cx >= count - 2 || cz >= count - 2) continue;
            ++report.rivers;
            const float wanted = 0.5f * parent.discharge.at(px, pz);
            const bool near = peak(cx, cz, 1) >= wanted;
            nearFound += near;
            wideFound += near || peak(cx, cz, 2) >= wanted;
        }
    }
    report.parentCells = int(drift.size());
    if (report.parentCells > 0) {
        report.rms = std::sqrt(squares / report.parentCells);
        report.waterAgree = double(agree) / report.parentCells;
    }
    if (stepCount > 0) report.parentStep = steps / stepCount;
    if (report.rivers > 0) {
        report.foundNear = double(nearFound) / report.rivers;
        report.foundWide = double(wideFound) / report.rivers;
    }
    report.drift = distributionOf(std::move(drift));
    return report;
}

} // namespace derivean::sim
