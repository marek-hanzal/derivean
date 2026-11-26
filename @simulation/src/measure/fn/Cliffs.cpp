#include "measure/fn/ReliefStats.hpp"

namespace derivean::sim {
namespace {

constexpr float CliffFall = 1.7320508f;   // rise over run of 60°

} // namespace

CliffStats cliffInventory(const Mosaic& mosaic, const Field<float>& fall) {
    CliffStats stats;
    const int side = mosaic.side();
    std::vector<int32_t> face(mosaic.ground.size(), -1), stack;
    std::vector<float> heights, lengths;
    size_t steep = 0, dry = 0;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) dry += mosaic.dry(x, z);
    for (int sz = 1; sz + 1 < side; ++sz) {
        for (int sx = 1; sx + 1 < side; ++sx) {
            if (fall.at(sx, sz) < CliffFall || face[fall.index(sx, sz)] >= 0) continue;
            // One face: every steep cell reachable from this one through steep neighbours.
            const int32_t id = int32_t(heights.size());
            float top = -1e30f, foot = 1e30f;
            double sumX = 0, sumZ = 0, sumXX = 0, sumZZ = 0, sumXZ = 0;
            size_t count = 0;
            stack.assign(1, int32_t(fall.index(sx, sz)));
            face[fall.index(sx, sz)] = id;
            while (!stack.empty()) {
                const int32_t cell = stack.back();
                stack.pop_back();
                const int x = cell % side, z = cell / side;
                ++count, sumX += x, sumZ += z, sumXX += double(x) * x, sumZZ += double(z) * z, sumXZ += double(x) * z;
                top = std::max(top, mosaic.ground.at(x, z));
                for (int k = 0; k < 8; ++k) {
                    const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                    foot = std::min(foot, mosaic.top(nx, nz));
                    if (nx < 1 || nz < 1 || nx >= side - 1 || nz >= side - 1) continue;
                    if (fall.at(nx, nz) < CliffFall || face[fall.index(nx, nz)] >= 0) continue;
                    face[fall.index(nx, nz)] = id;
                    stack.push_back(int32_t(fall.index(nx, nz)));
                }
            }
            steep += count;
            heights.push_back(top - foot);
            // Length along the face from the spread of its cells: a line of length L has variance L²/12.
            const double n = double(count), varX = sumXX / n - (sumX / n) * (sumX / n), varZ = sumZZ / n - (sumZ / n) * (sumZ / n), cov = sumXZ / n - (sumX / n) * (sumZ / n);
            const double major = 0.5 * (varX + varZ) + std::sqrt(0.25 * (varX - varZ) * (varX - varZ) + cov * cov);
            lengths.push_back(float((std::sqrt(12 * std::max(0.0, major)) + 1) * mosaic.cell));
        }
    }
    stats.faces = int(heights.size());
    const double area = double(dry) * mosaic.cell * mosaic.cell / 1e6;
    stats.perSquareKm = area > 0 ? stats.faces / area : 0.0;
    stats.steepShare = dry ? double(steep) / double(dry) : 0.0;
    stats.height = distributionOf(std::move(heights));
    stats.length = distributionOf(std::move(lengths));
    return stats;
}

} // namespace derivean::sim
