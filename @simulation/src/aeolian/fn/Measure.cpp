#include "aeolian/fn/Measure.hpp"

#include <algorithm>
#include <cstdint>

namespace derivean::sim {
namespace {

constexpr float Sandy = 0.1f;        // m of sand from which a cell counts as sand-covered
constexpr double SlipBand = 0.9;     // share of repose from which a slope counts as a slip face
constexpr double LeastDune = 0.25;   // cells: a crest must stand this much of a cell above the troughs beside it

double middle(std::vector<double>& values) {
    if (values.empty()) return 0;
    std::nth_element(values.begin(), values.begin() + long(values.size() / 2), values.end());
    return values[values.size() / 2];
}

// Crests and troughs along one line of heights `stride` metres apart: turning points that differ
// from the one before by at least `least`.
void walkLine(const std::vector<float>& line, double stride, double least, std::vector<double>& heights, std::vector<double>& spacings) {
    if (line.size() < 3) return;
    double low = line[0], high = line[0], lastTrough = line[0], lastCrestAt = -1;
    size_t highAt = 0;
    bool rising = true;
    for (size_t k = 1; k < line.size(); ++k) {
        const double h = line[k];
        if (rising) {
            if (h > high) high = h, highAt = k;
            if (h < high - least) {
                // A crest stands between the trough before it and the one that follows.
                if (lastCrestAt >= 0) spacings.push_back((double(highAt) - lastCrestAt) * stride);
                heights.push_back(high - lastTrough);
                lastCrestAt = double(highAt);
                rising = false, low = h;
            }
        } else {
            low = std::min(low, h);
            if (h > low + least) rising = true, lastTrough = low, high = h, highAt = k;
        }
    }
}

} // namespace

DuneStats measureDunes(int size, double cell, const std::vector<float>& top, const std::vector<float>& sand, Vec2 wind, float reposeTan) {
    DuneStats stats;
    const auto at = [&](int x, int z) { return size_t(z) * size_t(size) + size_t(x); };
    const Vec2 heading = normalized(wind);
    double sandy = 0, slip = 0, slipLee = 0, up = 0, upCount = 0, down = 0, downCount = 0, gxx = 0, gzz = 0, gxz = 0;
    for (int z = 1; z + 1 < size; ++z) {
        for (int x = 1; x + 1 < size; ++x) {
            const size_t i = at(x, z);
            stats.meanSand += sand[i];
            if (sand[i] < Sandy) continue;
            const double dx = (top[at(x + 1, z)] - top[at(x - 1, z)]) / (2 * cell), dz = (top[at(x, z + 1)] - top[at(x, z - 1)]) / (2 * cell);
            const double slope = std::hypot(dx, dz), along = dx * heading.x + dz * heading.z;
            sandy += 1;
            stats.steepest = std::max(stats.steepest, slope);
            gxx += dx * dx, gzz += dz * dz, gxz += dx * dz;
            if (slope >= SlipBand * reposeTan) slip += 1, slipLee += along < 0;
            if (slope < 0.03) continue;
            if (along > 0) up += slope, upCount += 1;
            else down += slope, downCount += 1;
        }
    }
    const double inner = double(size - 2) * (size - 2);
    stats.meanSand /= inner, stats.sandShare = sandy / inner;
    if (sandy < 1) return stats;
    stats.slipShare = slip / sandy, stats.leeSlipShare = slip > 0 ? slipLee / slip : 0.0;
    stats.windward = upCount > 0 ? up / upCount : 0.0, stats.lee = downCount > 0 ? down / downCount : 0.0;
    // The axis most slopes fall along lies across the crests.
    const double axis = 0.5 * std::atan2(2 * gxz, gxx - gzz);
    stats.order = gxx + gzz > 0 ? std::hypot(gxx - gzz, 2 * gxz) / (gxx + gzz) : 0.0;
    const Vec2 across{std::cos(axis), std::sin(axis)};
    stats.crestAngle = 90.0 - std::acos(std::min(1.0, std::abs(dot(across, heading)))) * 57.29577951;

    // Lines across the crests, four cells apart, read at the nearest cell. They read the sand's
    // thickness, not the top: on a real tile the ground under a field rises and falls, and a dune's
    // height is what it stands above its own floor.
    std::vector<double> heights, spacings;
    std::vector<float> line;
    const Vec2 side = perpendicular(across);
    for (int offset = -size; offset <= size; offset += 4) {
        line.clear();
        for (int step = -size; step <= size; ++step) {
            const int x = int(std::lround(size / 2 + across.x * step + side.x * offset)), z = int(std::lround(size / 2 + across.z * step + side.z * offset));
            if (x < 0 || z < 0 || x >= size || z >= size) {
                if (!line.empty()) break;
                continue;
            }
            line.push_back(sand[at(x, z)]);
        }
        walkLine(line, cell, LeastDune * cell, heights, spacings);
    }
    stats.crests = int(heights.size());
    stats.tallest = heights.empty() ? 0.0 : *std::max_element(heights.begin(), heights.end());
    stats.height = middle(heights), stats.spacing = middle(spacings);

    // Bodies of sand that touch along a cell's side are one.
    std::vector<uint8_t> seen(top.size(), 0);
    std::vector<int32_t> stack;
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            if (seen[at(x, z)] || sand[at(x, z)] < Sandy) continue;
            ++stats.patches;
            seen[at(x, z)] = 1;
            stack.assign(1, int32_t(at(x, z)));
            while (!stack.empty()) {
                const int cx = stack.back() % size, cz = stack.back() / size;
                stack.pop_back();
                const int nextX[4] = {cx + 1, cx - 1, cx, cx}, nextZ[4] = {cz, cz, cz + 1, cz - 1};
                for (int k = 0; k < 4; ++k) {
                    if (nextX[k] < 0 || nextZ[k] < 0 || nextX[k] >= size || nextZ[k] >= size) continue;
                    const size_t next = at(nextX[k], nextZ[k]);
                    if (seen[next] || sand[next] < Sandy) continue;
                    seen[next] = 1;
                    stack.push_back(int32_t(next));
                }
            }
        }
    }
    return stats;
}

} // namespace derivean::sim
