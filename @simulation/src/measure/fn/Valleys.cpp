#include "measure/fn/ReliefStats.hpp"

#include "measure/constant/Angle.hpp"

namespace derivean::sim {
namespace {

constexpr int SectionReach = 80;        // cells a cross-section is followed to either side
constexpr int MostSections = 600;
constexpr double TrunkShare = 0.99;     // quantile of discharge above which a stream counts as a main valley
constexpr double CrestDrop = 0.15;      // share of the rise so far the ground must fall again to end a wall
// m the ground may stand above a stream and still be its floor: a few channel depths at each level.
constexpr float FloorRise[LevelCount] = {0, 30.0f, 8.0f, 2.0f, 0.5f};

double sampleGround(const Mosaic& mosaic, double x, double z) {
    const int ix = std::clamp(int(std::floor(x)), 0, mosaic.side() - 2), iz = std::clamp(int(std::floor(z)), 0, mosaic.side() - 2);
    const double tx = std::clamp(x - ix, 0.0, 1.0), tz = std::clamp(z - iz, 0.0, 1.0);
    return mix(mix(mosaic.ground.at(ix, iz), mosaic.ground.at(ix + 1, iz), tx), mix(mosaic.ground.at(ix, iz + 1), mosaic.ground.at(ix + 1, iz + 1), tx), tz);
}

struct Side {
    double floor{};    // m from the stream to where the ground leaves the floor
    double wall{};     // m the ground rises above the stream before it falls again
    double crest{};    // m from the stream to that highest point
};

Side walk(const Mosaic& mosaic, double x, double z, Vec2 across, double base, float floorRise) {
    Side side;
    bool onFloor = true;
    double highest = base;
    side.floor = SectionReach * mosaic.cell;
    for (int step = 1; step <= SectionReach; ++step) {
        const double px = x + across.x * step, pz = z + across.z * step;
        if (px < 0 || pz < 0 || px > mosaic.side() - 1 || pz > mosaic.side() - 1) break;
        const double here = sampleGround(mosaic, px, pz);
        if (onFloor && here - base > floorRise) onFloor = false, side.floor = (step - 0.5) * mosaic.cell;
        if (here > highest) highest = here, side.crest = step * mosaic.cell;
        else if (!onFloor && here < highest - std::max(double(floorRise), CrestDrop * (highest - base))) break;
    }
    side.wall = onFloor ? 0.0 : highest - base;
    return side;
}

} // namespace

DrainageStats drainageDensity(const Mosaic& mosaic) {
    DrainageStats stats;
    const double stream = riverThreshold(mosaic.level);
    double brooks = 0, streams = 0;
    size_t land = 0;
    for (int z = 0; z < mosaic.side(); ++z) {
        for (int x = 0; x < mosaic.side(); ++x) {
            if (!mosaic.dry(x, z)) continue;
            ++land;
            const uint8_t to = mosaic.flowTo.at(x, z);
            const double reach = mosaic.cell * (to < 8 ? NeighborReach[to] : 1.0f);
            if (mosaic.discharge.at(x, z) >= BrookFlow) brooks += reach;
            if (mosaic.discharge.at(x, z) >= stream) streams += reach;
        }
    }
    const double area = double(land) * mosaic.cell * mosaic.cell;
    if (area > 0) stats.brooks = brooks / area * 1000, stats.streams = streams / area * 1000;
    return stats;
}

ValleyStats valleySections(const Mosaic& mosaic) {
    ValleyStats stats;
    const int side = mosaic.side();
    std::vector<float> flows;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x)
            if (mosaic.dry(x, z)) flows.push_back(mosaic.discharge.at(x, z));
    if (flows.size() < 100) return stats;
    std::nth_element(flows.begin(), flows.begin() + long(TrunkShare * double(flows.size())), flows.end());
    const float trunk = std::max(flows[size_t(TrunkShare * double(flows.size()))], float(riverThreshold(mosaic.level)));
    std::vector<int32_t> cells;
    for (int z = SectionReach / 4; z < side - SectionReach / 4; ++z)
        for (int x = SectionReach / 4; x < side - SectionReach / 4; ++x)
            if (mosaic.dry(x, z) && mosaic.discharge.at(x, z) >= trunk && mosaic.flowTo.at(x, z) < 8) cells.push_back(z * side + x);
    const size_t stride = std::max<size_t>(1, cells.size() / MostSections);
    const float floorRise = FloorRise[size_t(mosaic.level)];
    std::vector<float> floors, walls, angles, ratios;
    for (size_t pick = 0; pick < cells.size(); pick += stride) {
        const int x = cells[pick] % side, z = cells[pick] / side;
        // The valley's heading is that of the stream over its next few cells, not of one grid step.
        int ax = x, az = z;
        for (int step = 0; step < 4; ++step) {
            const uint8_t to = mosaic.flowTo.at(ax, az);
            if (to > 7 || !mosaic.contains(ax + NeighborX[to], az + NeighborZ[to])) break;
            ax += NeighborX[to], az += NeighborZ[to];
        }
        if (ax == x && az == z) continue;
        const Vec2 along = normalized({double(ax - x), double(az - z)}), across = perpendicular(along);
        const double base = mosaic.ground.at(x, z);
        const Side left = walk(mosaic, x, z, across, base, floorRise), right = walk(mosaic, x, z, across * -1.0, base, floorRise);
        const double wall = std::min(left.wall, right.wall);
        floors.push_back(float(left.floor + right.floor));
        walls.push_back(float(wall));
        if (wall <= 0) continue;
        angles.push_back(float(0.5 * DegreesPerRadian * (std::atan(left.wall / std::max(mosaic.cell, left.crest - left.floor)) + std::atan(right.wall / std::max(mosaic.cell, right.crest - right.floor)))));
        ratios.push_back(float(wall / std::max(mosaic.cell, left.crest + right.crest)));
    }
    stats.sections = int(floors.size());
    size_t canyons = 0;
    for (const float ratio : ratios) canyons += ratio >= 1.0f / 3.0f;
    stats.canyonShare = floors.empty() ? 0.0 : double(canyons) / double(floors.size());
    stats.floorWidth = distributionOf(std::move(floors));
    stats.wallHeight = distributionOf(std::move(walls));
    stats.wallAngle = distributionOf(std::move(angles));
    stats.depthToWidth = distributionOf(std::move(ratios));
    return stats;
}

} // namespace derivean::sim
