#include "Coast.hpp"

#include "Measure.hpp"

#include "measure/fn/Format.hpp"
#include "measure/fn/Scout.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr double Disc = 6000.0;        // m around a Region shore cell that its kind of coast is read over
constexpr double Steep = 2000.0;       // m from the shore within which the land must rise for a cliff to be possible
constexpr float CliffLand = 60.0f;     // m above the sea it must rise to there
constexpr float LowLand = 20.0f;       // m above the sea up to which land is lowland
constexpr double LeastLake = 2e6;      // m² of a Region lake worth looking at: a beach needs kilometres of open water
constexpr float FrozenLake = 1.0f;     // °C of annual mean at the ground under which a lake is ice much of the year
// m a Region lake must be deep somewhere to stand at the Local level too: shallower hollows are
// levelled as interpolation artefacts when a Local domain is built (landscape/fn/Build.cpp, levelHollows).
constexpr float KeptLake = 50.0f;
// Region cells from a Region tile edge within which a lake is not trusted: the seam blend between
// tiles built dams there (consistency stream), and a lake that touches the band may be one.
constexpr int EdgeBand = 8;

bool sea(const Mosaic& m, int x, int z) { return m.contains(x, z) && !m.dry(x, z) && !m.lake(x, z); }

struct Candidate {
    Vec2 at;
    double score{};
    std::string reason;
};

// Shore cells of the Region's sea, ranked by how much of a showcase coast lies within Disc of
// them: a shoreline that winds (bays and headlands), lowland and land that rises steeply from the water.
std::vector<Candidate> seaCoasts(const Mosaic& m, bool steep) {
    std::vector<Candidate> found;
    const int reach = cellsIn(m, Disc), near = cellsIn(m, Steep), side = m.side();
    for (int z = reach; z < side - reach; z += 2)
        for (int x = reach; x < side - reach; x += 2) {
            if (!m.dry(x, z) || !(sea(m, x + 1, z) || sea(m, x - 1, z) || sea(m, x, z + 1) || sea(m, x, z - 1))) continue;
            int cells = 0, wet = 0, shore = 0, low = 0;
            float highest = 0;
            for (int dz = -reach; dz <= reach; ++dz)
                for (int dx = -reach; dx <= reach; ++dx) {
                    if (dx * dx + dz * dz > reach * reach) continue;
                    const int nx = x + dx, nz = z + dz;
                    ++cells;
                    if (sea(m, nx, nz)) {
                        ++wet;
                        continue;
                    }
                    if (!m.dry(nx, nz)) continue;
                    const float above = float(m.ground.at(nx, nz) - m.seaLevel);
                    low += above < LowLand;
                    if (dx * dx + dz * dz <= near * near) highest = std::max(highest, above);
                    shore += sea(m, nx + 1, nz) || sea(m, nx - 1, nz) || sea(m, nx, nz + 1) || sea(m, nx, nz - 1);
                }
            const double share = double(wet) / cells;
            if (share < 0.25 || share > 0.75) continue;
            // Shore cells over the diameter of the disc: 1 for a straight shore, more where it winds.
            const double winding = double(shore) / (2 * reach + 1), lowland = double(low) / std::max(1, cells - wet);
            const bool cliff = highest >= CliffLand;
            Candidate c;
            c.at = cellCentre(m, x, z);
            // Lowland coasts by how they wind and how low they lie; steep ones by how high the land
            // rises from the water, where it winds.
            if (steep && !cliff) continue;
            c.score = steep ? winding * highest / CliffLand : winding * (0.5 + lowland) * (cliff ? 1.5 : 1.0);
            c.reason = formatText("sea coast: shoreline %.1f times the disc's width, lowland %.0f %% of the land, land %.0f m above the sea within 2 km", winding, 100 * lowland,
                                  highest);
            found.push_back(c);
        }
    return found;
}

// Area of the Region's lakes by what keeps them from being a candidate, against its land (dry and lake).
struct LakeShare {
    double land{}, small{}, frozen{}, shallow{}, edge{}, kept{};
};

// Region cells from cell (x, z) of a mosaic to the nearest edge of the Region tile holding it.
int edgeDistance(int x, int z) {
    const int ax = x % TileCells, az = z % TileCells;
    return std::min({ax, TileCells - 1 - ax, az, TileCells - 1 - az});
}

// Lakes of the Region large enough to raise waves that sort a beach, warm enough to be open water,
// and clear of the tile edges.
std::vector<Candidate> lakes(const Mosaic& m, LakeShare& share) {
    std::vector<Candidate> found;
    const int side = m.side();
    std::vector<uint8_t> seen(size_t(side) * size_t(side), 0);
    std::vector<int> queue;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            if (seen[size_t(z) * size_t(side) + size_t(x)] || !m.lake(x, z)) continue;
            queue.assign(1, z * side + x);
            seen[size_t(z) * size_t(side) + size_t(x)] = 1;
            int minX = x, maxX = x, minZ = z, maxZ = z, edge = TileCells;
            double sumX = 0, sumZ = 0, warmth = 0;
            float deepest = 0;
            for (size_t next = 0; next < queue.size(); ++next) {
                const int cx = queue[next] % side, cz = queue[next] / side;
                minX = std::min(minX, cx), maxX = std::max(maxX, cx), minZ = std::min(minZ, cz), maxZ = std::max(maxZ, cz);
                sumX += cx, sumZ += cz, warmth += m.warmth.at(cx, cz), edge = std::min(edge, edgeDistance(cx, cz));
                deepest = std::max(deepest, m.water.at(cx, cz) - m.ground.at(cx, cz));
                for (int k = 0; k < 8; k += 2) {
                    const int nx = cx + NeighborX[k], nz = cz + NeighborZ[k];
                    if (!m.contains(nx, nz) || seen[size_t(nz) * size_t(side) + size_t(nx)] || !m.lake(nx, nz)) continue;
                    seen[size_t(nz) * size_t(side) + size_t(nx)] = 1;
                    queue.push_back(nz * side + nx);
                }
            }
            const double area = double(queue.size()) * m.cell * m.cell, across = std::hypot(maxX - minX + 1, maxZ - minZ + 1) * m.cell;
            warmth /= double(queue.size());
            if (minX == 0 || minZ == 0 || maxX == side - 1 || maxZ == side - 1) continue;
            double& kind = area < LeastLake ? share.small : warmth < FrozenLake ? share.frozen : deepest < KeptLake ? share.shallow : edge < EdgeBand ? share.edge : share.kept;
            kind += area;
            if (&kind != &share.kept) {
                if (&kind == &share.edge)
                    std::printf("  lake near a tile edge (%d cells), set aside: X %.0f Z %.0f  %.1f km2, %.0f m deep\n", edge, cellCentre(m, x, z).x, cellCentre(m, x, z).z, area / 1e6, deepest);
                continue;
            }
            // The lake cell nearest its middle, so the Local tiles around it hold the lake.
            const int mx = int(sumX / double(queue.size())), mz = int(sumZ / double(queue.size()));
            int best = queue.front();
            for (const int cell : queue)
                if (std::hypot(cell % side - mx, cell / side - mz) < std::hypot(best % side - mx, best / side - mz)) best = cell;
            Candidate c;
            c.at = cellCentre(m, best % side, best / side);
            // Kilometres of open water make the waves; a lake much larger than a Local tile is seen only in part.
            c.score = std::min(across, 20e3) / 1000.0 - 0.005 * std::max(0.0, double(m.water.at(best % side, best / side)) - m.seaLevel);
            c.reason = formatText("lake %.1f km2, %.1f km across, %.0f m deep, surface %.0f m, ground %.1f C, %d cells from a tile edge", area / 1e6, across / 1000, deepest,
                                  m.water.at(best % side, best / side), warmth, edge);
            found.push_back(c);
        }
    return found;
}

std::vector<Candidate> apart(std::vector<Candidate> all, int limit, double distance) {
    std::sort(all.begin(), all.end(), [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
    std::vector<Candidate> kept;
    for (const Candidate& c : all) {
        if (int(kept.size()) >= limit) break;
        bool near = false;
        for (const Candidate& k : kept) near = near || length(k.at - c.at) < distance;
        if (!near) kept.push_back(c);
    }
    return kept;
}

} // namespace

// terra coast --find 1 [--x --z] [--radius m] [--tries n] [--confirm 0] [--pick lake2,sea1] [--site 0] [--out directory]:
// sea coasts and lakes around a start (the landing unless given), ranked on the Region by what a
// showcase shore needs, then surveyed on 2 × 2 Local tiles with a map each; with Site tiles at the
// best sand beach (and for the sea the highest cliff) of each, and two pictures there.
int coastFind(const Arguments& arguments) {
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 start{arguments.number("x", 0), arguments.number("z", 0)};
    if (!arguments.values.count("x")) start = findLanding(world, start);
    const double radius = arguments.number("radius", 300e3);
    const int tries = int(arguments.number("tries", 3));
    const std::string out = arguments.text("out", "");
    const Mosaic region = gatherAround(world, Level::Region, start, radius);
    std::printf("coast find  seed %llu  from X %.0f Z %.0f  radius %.0f km  region %d x %d tiles  sea level %.1f m  (%.0f s)\n", static_cast<unsigned long long>(arguments.seed()), start.x,
                start.z, radius / 1000, region.tiles, region.tiles, region.seaLevel, lap());
    LakeShare share;
    const std::vector<Candidate> coasts = apart(seaCoasts(region, false), tries, 30e3), steeps = apart(seaCoasts(region, true), tries, 30e3), waters = apart(lakes(region, share), tries, 10e3);
    for (int z = 0; z < region.side(); ++z)
        for (int x = 0; x < region.side(); ++x) share.land += (region.dry(x, z) || region.lake(x, z)) ? region.cell * region.cell : 0.0;
    const double lakeArea = share.small + share.frozen + share.shallow + share.edge + share.kept;
    share.land = std::max(share.land, 1.0);
    std::printf("  lakes %.2f %% of the land (%.0f km2 of %.0f km2): under %.0f km2 %.2f %%, frozen %.2f %%, under %.0f m deep %.2f %%, within %d cells of a tile edge %.2f %%, candidates %.2f %%\n",
                100 * lakeArea / share.land, lakeArea / 1e6, share.land / 1e6, LeastLake / 1e6, 100 * share.small / share.land, 100 * share.frozen / share.land, KeptLake, 100 * share.shallow / share.land,
                EdgeBand, 100 * share.edge / share.land, 100 * share.kept / share.land);
    for (size_t k = 0; k < coasts.size(); ++k) std::printf("  sea %zu  X %.0f Z %.0f  score %.2f  %s\n", k + 1, coasts[k].at.x, coasts[k].at.z, coasts[k].score, coasts[k].reason.c_str());
    for (size_t k = 0; k < steeps.size(); ++k) std::printf("  cliff %zu  X %.0f Z %.0f  score %.2f  %s\n", k + 1, steeps[k].at.x, steeps[k].at.z, steeps[k].score, steeps[k].reason.c_str());
    for (size_t k = 0; k < waters.size(); ++k) std::printf("  lake %zu  X %.0f Z %.0f  score %.2f  %s\n", k + 1, waters[k].at.x, waters[k].at.z, waters[k].score, waters[k].reason.c_str());
    std::fflush(stdout);
    if (arguments.number("confirm", 1) <= 0) return 0;
    // --pick lake2,sea1 examines only those; by default every candidate listed.
    const std::string picked = arguments.text("pick", "");
    const bool site = arguments.number("site", 1) > 0;
    const auto chosen = [&](const std::string& tag) { return picked.empty() || ("," + picked + ",").find("," + tag + ",") != std::string::npos; };
    for (size_t k = 0; k < waters.size(); ++k)
        if (chosen("lake" + std::to_string(k + 1))) examineShore(world, waters[k].at, {ShoreKind::Sand}, site, out, "lake" + std::to_string(k + 1));
    for (size_t k = 0; k < coasts.size(); ++k)
        if (chosen("sea" + std::to_string(k + 1))) examineShore(world, coasts[k].at, {ShoreKind::Sand}, site, out, "sea" + std::to_string(k + 1));
    for (size_t k = 0; k < steeps.size(); ++k)
        if (chosen("cliff" + std::to_string(k + 1))) examineShore(world, steeps[k].at, {ShoreKind::Cliff, ShoreKind::Sand}, site, out, "cliff" + std::to_string(k + 1));
    return 0;
}
