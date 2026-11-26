#include "measure/fn/ReliefStats.hpp"

#include "measure/constant/Angle.hpp"

namespace derivean::sim {
namespace {

constexpr int Flank = 4;   // cells to either side of a crest over which its flanks are read
constexpr int AxisX[4] = {1, 0, 1, 1}, AxisZ[4] = {0, 1, 1, -1};
constexpr float AxisReach[4] = {1, 1, 1.41421356f, 1.41421356f};

} // namespace

CrestStats crestSharpness(const Mosaic& mosaic) {
    CrestStats stats;
    const int side = mosaic.side();
    std::vector<uint8_t> fed(mosaic.ground.size(), 0);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const uint8_t to = mosaic.flowTo.at(x, z);
            if (to < 8 && mosaic.contains(x + NeighborX[to], z + NeighborZ[to])) fed[mosaic.ground.index(x + NeighborX[to], z + NeighborZ[to])] = 1;
        }
    std::vector<float> angles, sharpness, curvature;
    for (int z = Flank; z < side - Flank; ++z) {
        for (int x = Flank; x < side - Flank; ++x) {
            if (fed[mosaic.ground.index(x, z)] || !mosaic.dry(x, z)) continue;
            const float here = mosaic.ground.at(x, z);
            // Across the crest is the axis along which the ground falls most on both sides.
            int best = -1;
            float steepest = 0;
            for (int axis = 0; axis < 4; ++axis) {
                const float before = mosaic.ground.at(x - AxisX[axis], z - AxisZ[axis]), after = mosaic.ground.at(x + AxisX[axis], z + AxisZ[axis]);
                if (before >= here || after >= here) continue;
                const float fall = (here - 0.5f * (before + after)) / AxisReach[axis];
                if (fall > steepest) steepest = fall, best = axis;
            }
            if (best < 0) continue;
            const double reach = AxisReach[best] * mosaic.cell;
            bool land = true;
            double below = 0;
            for (const int sign : {-1, 1}) {
                for (int k = 1; k <= Flank; ++k) land = land && mosaic.dry(x + sign * k * AxisX[best], z + sign * k * AxisZ[best]);
                below += 0.5 * (mosaic.ground.at(x + sign * AxisX[best], z + sign * AxisZ[best]) - mosaic.ground.at(x + sign * Flank * AxisX[best], z + sign * Flank * AxisZ[best])) / ((Flank - 1) * reach);
            }
            if (!land || below <= 1e-4) continue;
            const double top = double(steepest) * AxisReach[best] / reach;
            angles.push_back(float(std::atan(top) * DegreesPerRadian));
            sharpness.push_back(float(std::min(2.0, top / below)));
            curvature.push_back(float(2 * top / reach));
        }
    }
    size_t dry = 0, peaks = 0, pits = 0;
    std::vector<float> rough, standing;
    for (int z = 1; z < side - 1; ++z) {
        for (int x = 1; x < side - 1; ++x) {
            if (!mosaic.dry(x, z)) continue;
            const float here = mosaic.ground.at(x, z);
            float low = 1e30f, high = -1e30f, sum = 0;
            bool land = true;
            for (int k = 0; k < 8; ++k) {
                const float other = mosaic.ground.at(x + NeighborX[k], z + NeighborZ[k]);
                land = land && mosaic.dry(x + NeighborX[k], z + NeighborZ[k]);
                low = std::min(low, other), high = std::max(high, other), sum += other;
            }
            if (!land) continue;
            ++dry, peaks += here > high, pits += here < low;
            if (here > high) standing.push_back((here - high) / std::max(1e-3f, here - low));
            if (high - low > 1e-3f) rough.push_back(std::abs(here - sum / 8) / (high - low));
        }
    }
    stats.peakShare = dry ? double(peaks) / double(dry) : 0.0;
    stats.pitShare = dry ? double(pits) / double(dry) : 0.0;
    stats.roughness = medianOf(rough);
    stats.peakStanding = medianOf(standing);
    // Relief that sits at the scale of one cell shows as an rms against the 3 × 3 mean that is
    // most of the rms against the 17 × 17 mean; in a ridge-and-valley surface it is a small part.
    for (const int reach : {1, 8}) {
        double sum = 0;
        size_t count = 0;
        for (int z = 8; z < side - 8; z += 2) {
            for (int x = 8; x < side - 8; x += 2) {
                if (!mosaic.dry(x, z)) continue;
                double mean = 0;
                for (int dz = -reach; dz <= reach; ++dz)
                    for (int dx = -reach; dx <= reach; ++dx) mean += mosaic.top(x + dx, z + dz);
                mean /= double((2 * reach + 1) * (2 * reach + 1));
                sum += (mosaic.ground.at(x, z) - mean) * (mosaic.ground.at(x, z) - mean), ++count;
            }
        }
        (reach == 1 ? stats.rmsNear : stats.rmsWide) = count ? std::sqrt(sum / double(count)) : 0.0;
    }
    stats.crests = int(angles.size());
    stats.flankAngle = distributionOf(std::move(angles));
    stats.sharpness = distributionOf(std::move(sharpness));
    stats.curvature = distributionOf(std::move(curvature));
    return stats;
}

} // namespace derivean::sim
