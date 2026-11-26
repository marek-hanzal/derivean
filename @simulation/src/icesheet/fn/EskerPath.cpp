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
// Cells the stream advances per step of the trace.
constexpr double Step = 0.5;
// m of head an open channel the stream runs into draws the water around it down by, falling to
// none at twice the corridor: a channel at low pressure takes in the water beside it.
constexpr double Drawdown = 50.0;
// Share of each step the stream at least advances along its route: where the head rises it still
// passes on, through a pond under the ice.
constexpr double Onward = 0.3;
// m of head below the highest point still ahead on its path at which the stream stands in a pond
// under the ice: it runs full and slow there and leaves no ridge.
constexpr double Ponded = 2.0;

// A chain of the parent's stretches: the cell centres it runs through and the arc length to each.
struct Route {
    std::vector<size_t> chain;
    std::vector<Vec2> points;
    std::vector<double> at;

    // Arc length to the point of the route nearest p, that point, and the route's direction there.
    double project(Vec2 p, Vec2& nearest, Vec2& ahead) const {
        double distance = 1e30, best = 0;
        for (size_t k = 0; k + 1 < points.size(); ++k) {
            const Vec2 ab = points[k + 1] - points[k];
            const double span = dot(ab, ab), t = span > 0 ? std::clamp(dot(p - points[k], ab) / span, 0.0, 1.0) : 0.0;
            const Vec2 q = points[k] + ab * t;
            if (const double d = length(p - q); d < distance) distance = d, best = at[k] + t * (at[k + 1] - at[k]), nearest = q, ahead = span > 0 ? ab * (1.0 / std::sqrt(span)) : Vec2{};
        }
        return best;
    }
};

// The head of the water under the last maximum's ice, and its slope, between cell centres.
struct Head {
    const Domain& domain;
    std::vector<float> head, slopeX, slopeZ;

    void corner(Vec2 p, int& x, int& z, double& tx, double& tz) const {
        const double fx = std::clamp((p.x - domain.origin.x) / domain.cell - 0.5, 0.0, domain.size - 1.0), fz = std::clamp((p.z - domain.origin.z) / domain.cell - 0.5, 0.0, domain.size - 1.0);
        x = std::min(int(fx), domain.size - 2), z = std::min(int(fz), domain.size - 2), tx = fx - x, tz = fz - z;
    }
    double sample(const std::vector<float>& field, Vec2 p) const {
        int x, z;
        double tx, tz;
        corner(p, x, z, tx, tz);
        const auto at = [&](int dx, int dz) { return double(field[domain.index(x + dx, z + dz)]); };
        return (at(0, 0) * (1 - tx) + at(1, 0) * tx) * (1 - tz) + (at(0, 1) * (1 - tx) + at(1, 1) * tx) * tz;
    }
};

Head headOf(const Domain& domain, const ParentSheet& parent) {
    Head h{domain, {}, {}, {}};
    const int size = domain.size;
    h.head.resize(domain.cells()), h.slopeX.resize(domain.cells()), h.slopeZ.resize(domain.cells());
    for (size_t i = 0; i < domain.cells(); ++i) {
        const float ground = domain.ground.rock[i] + domain.sedimentAt(i);
        h.head[i] = ground + float(IceToWater) * std::max(0.0f, parent.topThen[i] - ground);
    }
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const int w = std::max(0, x - 1), e = std::min(size - 1, x + 1), n = std::max(0, z - 1), s = std::min(size - 1, z + 1);
            h.slopeX[domain.index(x, z)] = float((h.head[domain.index(e, z)] - h.head[domain.index(w, z)]) / ((e - w) * domain.cell));
            h.slopeZ[domain.index(x, z)] = float((h.head[domain.index(x, s)] - h.head[domain.index(x, n)]) / ((s - n) * domain.cell));
        }
    return h;
}

// Where a finished neighbour's recorded ridge meets the tile's own cells near a route, on the side
// the route comes from (`entering`) or goes to: the middle of the ridge cells within two cells of
// the highest one, so a second ridge nearby is not averaged in.
bool crossing(const Domain& domain, const Route& route, double corridor, double edge, bool entering, Vec2& where) {
    std::vector<std::pair<Vec2, float>> found;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            size_t at = 0;
            const SheetRecord* record = domain.hold[domain.index(x, z)] == Settled ? finishedSheet(domain, x, z, at) : nullptr;
            if (!record || record->esker[at] <= 1.0f) continue;
            bool beside = false;
            for (int k = 0; k < 8 && !beside; ++k) beside = domain.owned(x + NeighborX[k], z + NeighborZ[k]);
            if (!beside) continue;
            Vec2 nearest, ahead;
            const double along = route.project(domain.center(x, z), nearest, ahead);
            if (length(domain.center(x, z) - nearest) > corridor || (entering ? along > edge + 0.5 * corridor : along < edge - 0.5 * corridor)) continue;
            found.push_back({domain.center(x, z), record->esker[at]});
        }
    if (found.empty()) return false;
    size_t top = 0;
    for (size_t k = 1; k < found.size(); ++k)
        if (found[k].second > found[top].second) top = k;
    Vec2 sum{};
    int count = 0;
    for (const auto& [p, height] : found)
        if (length(p - found[top].first) <= 2.0 * domain.cell) sum = sum + p, ++count;
    where = sum * (1.0 / count);
    return true;
}

} // namespace

std::vector<EskerTrack> traceEskers(const Domain& domain, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const double cell = domain.cell, corridor = Corridor * Refinement * cell, step = Step * cell;
    const Head head = headOf(domain, parent);
    // Chains: a stretch carries on into the one starting where it ends; where two meet, the one
    // with more water carries on and the other joins it.
    const auto key = [&](Vec2 p) { return std::pair<int64_t, int64_t>{std::llround(p.x / (Refinement * cell) - 0.5), std::llround(p.z / (Refinement * cell) - 0.5)}; };
    std::map<std::pair<int64_t, int64_t>, size_t> startingAt;
    for (size_t s = 0; s < tunnels.size(); ++s) startingAt[key(tunnels[s].from)] = s;
    std::vector<int64_t> main(tunnels.size(), -1), next(tunnels.size(), -1);
    for (size_t s = 0; s < tunnels.size(); ++s)
        if (auto found = startingAt.find(key(tunnels[s].to)); found != startingAt.end()) {
            next[s] = int64_t(found->second);
            int64_t& carried = main[found->second];
            if (carried < 0 || tunnels[s].discharge > tunnels[size_t(carried)].discharge) carried = int64_t(s);
        }
    std::vector<Route> routes;
    for (size_t first = 0; first < tunnels.size(); ++first) {
        if (main[first] >= 0) continue;
        Route route;
        for (int64_t s = int64_t(first); s >= 0;) {
            route.chain.push_back(size_t(s));
            const int64_t onward = next[size_t(s)];
            s = onward >= 0 && main[size_t(onward)] == s ? onward : -1;
        }
        for (size_t s : route.chain) route.points.push_back(tunnels[s].from);
        route.points.push_back(tunnels[route.chain.back()].to);
        route.at.assign(route.points.size(), 0.0);
        for (size_t k = 1; k < route.points.size(); ++k) route.at[k] = route.at[k - 1] + length(route.points[k] - route.points[k - 1]);
        routes.push_back(std::move(route));
    }
    // Trunks before the streams that join them.
    const auto most = [&](const Route& route) {
        float flow = 0;
        for (size_t s : route.chain) flow = std::max(flow, tunnels[s].discharge);
        return flow;
    };
    std::stable_sort(routes.begin(), routes.end(), [&](const Route& a, const Route& b) { return most(a) > most(b); });

    const Vec2 low = domain.origin, high = domain.origin + Vec2{domain.size * cell, domain.size * cell};
    const auto within = [&](Vec2 p) { return p.x >= low.x && p.z >= low.z && p.x < high.x && p.z < high.z; };
    std::vector<EskerTrack> tracks;
    std::vector<int32_t> trackOf(tunnels.size(), -1);
    for (const Route& route : routes) {
        // Where the route first and last lies over the tile's own cells.
        double inside = -1, outside = -1;
        for (double a = 0; a <= route.at.back(); a += step) {
            size_t k = 0;
            while (k + 2 < route.at.size() && a > route.at[k + 1]) ++k;
            const double span = route.at[k + 1] - route.at[k], t = span > 0 ? (a - route.at[k]) / span : 0.0;
            const Vec2 p = route.points[k] + (route.points[k + 1] - route.points[k]) * t;
            const int x = int(std::floor((p.x - low.x) / cell)), z = int(std::floor((p.z - low.z) / cell));
            if (domain.owned(x, z)) outside = a, inside = inside < 0 ? a : inside;
        }
        // It starts where the route enters the domain, or on a finished neighbour's ridge where that
        // comes over the edge; it ends where the route does, on the ridge of a finished neighbour it
        // runs into, or on the trunk it joins.
        Vec2 start{}, target{};
        bool started = inside >= 0 && crossing(domain, route, corridor, inside, true, start);
        for (size_t k = 0; k < route.points.size() && !started; ++k) started = within(route.points[k]), start = route.points[k];
        if (!started) continue;
        bool aimed = outside >= 0 && crossing(domain, route, corridor, outside, false, target);
        if (const int64_t onward = next[route.chain.back()]; !aimed && onward >= 0 && trackOf[size_t(onward)] >= 0) {
            double closest = 1e30;
            for (const Vec2& p : tracks[size_t(trackOf[size_t(onward)])].points)
                if (const double d = length(p - route.points.back()); d < closest) closest = d, target = p, aimed = true;
        }
        // Down the head, never back up the route, drawn towards where it ends; turned back towards
        // the route near the edge of the cells it crossed.
        const auto headAt = [&](Vec2 p) {
            const double d = aimed ? length(p - target) : 1e30;
            return head.sample(head.head, p) - (d < 2 * corridor ? Drawdown * (1 - d / (2 * corridor)) : 0.0);
        };
        EskerTrack track;
        Vec2 p = start, nearest, ahead;
        // Past the target, or at the route's end, an aimed stream joins its target directly.
        double along = route.project(p, nearest, ahead);
        // A target level with the start or behind it is no end to run to.
        if (aimed && route.project(target, nearest, ahead) <= along) aimed = false;
        const double reachesTarget = std::min(aimed ? route.project(target, nearest, ahead) + cell : 1e30, route.at.back() - 0.5 * cell);
        along = route.project(p, nearest, ahead);
        track.points.push_back(p);
        std::vector<double> heads{headAt(p)};
        // A join longer than two cells is not a course the head gave: it is not drawn.
        std::vector<bool> traced;
        const int most = int(std::ceil(route.at.back() / (Onward * step))) + 4 * domain.size;
        for (int steps = 0; steps < most; ++steps) {
            const bool arrived = aimed ? length(target - p) < cell || along >= reachesTarget : along >= route.at.back() - 0.5 * cell;
            if (arrived) {
                if (aimed) track.points.push_back(target), heads.push_back(headAt(target)), traced.push_back(length(target - p) <= 2 * cell);
                break;
            }
            Vec2 down = Vec2{-head.sample(head.slopeX, p), -head.sample(head.slopeZ, p)};
            if (const double d = aimed ? length(target - p) : 1e30; d < 2 * corridor) down = down + (target - p) * (Drawdown / (2 * corridor) / d);
            const double fall = length(down);
            const Vec2 u = fall > 1e-9 ? down * (1.0 / fall) : ahead;
            const double forward = dot(u, ahead);
            Vec2 way = u - ahead * forward + ahead * std::max(forward, Onward);
            way = way * (1.0 / std::max(1e-9, length(way)));
            if (const double off = length(p - nearest); off > 0.5 * corridor) {
                way = way + (nearest - p) * ((off - 0.5 * corridor) / (0.5 * corridor) / off);
                way = way * (1.0 / std::max(1e-9, length(way)));
            }
            const Vec2 q = p + way * step;
            if (!within(q)) break;
            track.points.push_back(p = q), heads.push_back(headAt(q)), traced.push_back(true);
            along = route.project(p, nearest, ahead);
        }
        if (track.points.size() < 2) continue;
        // A stretch below the highest head still ahead of it is a pond.
        std::vector<bool> ponded(heads.size());
        double barrier = -1e30;
        for (size_t k = heads.size(); k-- > 0;) barrier = std::max(barrier, heads[k]), ponded[k] = barrier - heads[k] > Ponded;
        for (size_t k = 0; k + 1 < heads.size(); ++k) track.open.push_back(traced[k] && !ponded[k] && !ponded[k + 1]);
        for (const Vec2& point : track.points) {
            const double here = route.project(point, nearest, ahead);
            size_t k = 0;
            while (k + 1 < route.chain.size() && here >= route.at[k + 1]) ++k;
            track.stretch.push_back(int32_t(route.chain[k]));
        }
        for (size_t s : route.chain) trackOf[s] = int32_t(tracks.size());
        tracks.push_back(std::move(track));
    }
    return tracks;
}

} // namespace derivean::sim
