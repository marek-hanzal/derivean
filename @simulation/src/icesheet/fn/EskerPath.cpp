#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace derivean::sim {
namespace {

// Of a parent cell, how far either side of the parent's route (through the cell centres) its
// stream may have run: anywhere in the cells the route crossed.
constexpr double Corridor = 0.6;
// m of head a cell on a finished neighbour's recorded ridge counts as lower than its own: the
// stream keeps to the path the neighbour already gave it.
constexpr float Recorded = 1e4f;

// Arc length along a polyline to the point nearest p, and the distance to it.
double project(const std::vector<Vec2>& line, const std::vector<double>& at, Vec2 p, double& distance) {
    distance = 1e30;
    double best = 0;
    for (size_t k = 0; k + 1 < line.size(); ++k) {
        const Vec2 ab = line[k + 1] - line[k];
        const double span = dot(ab, ab), t = span > 0 ? std::clamp(dot(p - line[k], ab) / span, 0.0, 1.0) : 0.0;
        const double d = length(p - (line[k] + ab * t));
        if (d < distance) distance = d, best = at[k] + t * (at[k + 1] - at[k]);
    }
    return best;
}

// Corners cut twice (Chaikin): the stream's course without the 45° steps of the cells it was traced on.
std::vector<Vec2> rounded(std::vector<Vec2> line) {
    for (int pass = 0; pass < 2 && line.size() > 2; ++pass) {
        std::vector<Vec2> next{line.front()};
        for (size_t k = 0; k + 1 < line.size(); ++k) {
            next.push_back(line[k] * 0.75 + line[k + 1] * 0.25);
            next.push_back(line[k] * 0.25 + line[k + 1] * 0.75);
        }
        next.push_back(line.back());
        line = std::move(next);
    }
    return line;
}

} // namespace

std::vector<EskerTrack> traceEskers(const Domain& domain, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    // The head of the water under the last maximum's ice; on a finished neighbour's ridge, lower still.
    std::vector<float> head(cells);
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const float ground = domain.ground.rock[i] + domain.sedimentAt(i);
            head[i] = ground + float(IceToWater) * std::max(0.0f, parent.topThen[i] - ground);
            size_t at = 0;
            if (domain.hold[i] == Settled)
                if (const SheetRecord* record = finishedSheet(domain, x, z, at); record && record->esker[at] > 0) head[i] -= Recorded;
        }
    // Chains: a stretch continues the one whose end is its start.
    std::map<std::pair<double, double>, size_t> startingAt;
    for (size_t s = 0; s < tunnels.size(); ++s) startingAt[{tunnels[s].from.x, tunnels[s].from.z}] = s;
    std::vector<bool> continues(tunnels.size(), false);
    for (const Tunnel& tunnel : tunnels)
        if (auto next = startingAt.find({tunnel.to.x, tunnel.to.z}); next != startingAt.end()) continues[next->second] = true;
    const double corridor = Corridor * Refinement * domain.cell;
    std::vector<EskerTrack> tracks;
    std::vector<bool> taken(tunnels.size(), false);
    for (size_t first = 0; first < tunnels.size(); ++first) {
        if (continues[first] || taken[first]) continue;
        // The parent's route: stretch starts, then the last end, with arc length along it.
        std::vector<size_t> chain;
        for (size_t s = first; !taken[s];) {
            taken[s] = true, chain.push_back(s);
            auto next = startingAt.find({tunnels[s].to.x, tunnels[s].to.z});
            if (next == startingAt.end()) break;
            s = next->second;
        }
        std::vector<Vec2> route;
        for (size_t s : chain) route.push_back(tunnels[s].from);
        route.push_back(tunnels[chain.back()].to);
        std::vector<double> at(route.size(), 0.0);
        for (size_t k = 1; k < route.size(); ++k) at[k] = at[k - 1] + length(route[k] - route[k - 1]);
        // Down the head from where the route enters the domain, never back up the route by more
        // than half a cell and never through a cell twice, until the route ends or leaves.
        int cx = -1, cz = -1;
        for (size_t k = 0; k < route.size() && cx < 0; ++k) {
            const int x = int(std::floor((route[k].x - domain.origin.x) / domain.cell)), z = int(std::floor((route[k].z - domain.origin.z) / domain.cell));
            if (x >= 0 && z >= 0 && x < size && z < size) cx = x, cz = z;
        }
        if (cx < 0) continue;
        std::vector<bool> visited(cells, false);
        std::vector<Vec2> path;
        double distance = 0, along = project(route, at, domain.center(cx, cz), distance);
        for (int steps = 0; steps < 4 * size; ++steps) {
            visited[domain.index(cx, cz)] = true;
            path.push_back(domain.center(cx, cz));
            if (along >= at.back() - 0.5 * domain.cell) break;
            // The lowest head; between equal heads, the cell nearer the parent's route.
            int best = -1;
            double bestAlong = 0, bestOff = 0;
            float lowest = 0;
            for (int k = 0; k < 8; ++k) {
                const int nx = cx + NeighborX[k], nz = cz + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size || visited[domain.index(nx, nz)]) continue;
                double off = 0;
                const double there = project(route, at, domain.center(nx, nz), off);
                if (off > corridor || there < along - 0.5 * domain.cell) continue;
                const float h = head[domain.index(nx, nz)];
                if (best < 0 || h < lowest || (h == lowest && off < bestOff)) best = k, bestAlong = there, bestOff = off, lowest = h;
            }
            if (best < 0) break;
            cx += NeighborX[best], cz += NeighborZ[best], along = bestAlong;
        }
        if (path.size() < 2) continue;
        EskerTrack track;
        track.points = rounded(path);
        for (const Vec2& point : track.points) {
            const double here = project(route, at, point, distance);
            size_t k = 0;
            while (k + 1 < chain.size() && here >= at[k + 1]) ++k;
            track.stretch.push_back(int32_t(chain[k]));
        }
        tracks.push_back(std::move(track));
    }
    return tracks;
}

} // namespace derivean::sim
