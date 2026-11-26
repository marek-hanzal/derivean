#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"
#include "landscape/fn/Process.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// Point at `t` (0..1) of the Catmull-Rom curve between b and c with a before and d after: the
// stream's path through the parent's cell centres without the corners of their grid.
Vec2 along(Vec2 a, Vec2 b, Vec2 c, Vec2 d, double t) {
    const double t2 = t * t, t3 = t2 * t;
    return (b * 2.0 + (c - a) * t + (a * 2.0 - b * 5.0 + c * 4.0 - d) * t2 + (b * 3.0 - a - c * 3.0 + d) * t3) * 0.5;
}

// Distance from p to the segment ab, and where along it (unclamped) p lies.
double distanceToSegment(Vec2 p, Vec2 a, Vec2 b, double& along) {
    const Vec2 ab = b - a;
    const double span = dot(ab, ab);
    along = span > 0 ? dot(p - a, ab) / span : 0.0;
    return length(p - (a + ab * std::clamp(along, 0.0, 1.0)));
}

} // namespace

EskerFill setAsideEskers(Domain& domain, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const size_t cells = domain.cells();
    const double area = domain.cell * domain.cell;
    EskerFill fill;
    fill.ridge.assign(cells, 0.0f);
    if (tunnels.empty()) return fill;
    // Each stretch holds as much of its fill as a ridge as wide as its tunnel takes, no steeper at
    // its foot than gravel stands; the rest stays spread in the cover. Its gravel and sand come from
    // the cells nearest to it, all of which lie in every domain whose tile it reaches (the halo is
    // wider than a parent cell), so a stretch's ridge stands the same in each of them.
    const size_t count = tunnels.size();
    std::vector<double> volume(count, 0.0), wanted(count, 0.0), available(count, 0.0);
    for (size_t s = 0; s < count; ++s) {
        const Tunnel& tunnel = tunnels[s];
        const double reach = length(tunnel.to - tunnel.from);
        if (reach <= 0 || tunnel.fill <= 0) continue;
        const double half = std::clamp(EskerSpread * std::pow(std::max(0.0, double(tunnel.discharge)) / 1e8, 0.4), double(EskerNarrowest), double(EskerWidest));
        const double crest = std::min(double(EskerTallest), 0.5 * ReposeTan[Coarse] * half);
        volume[s] = std::min(tunnel.fill, 4.0 / 3.0 * crest * half * reach);
        wanted[s] = volume[s] / area;
    }
    std::vector<int32_t> nearest(cells, -1);
    std::vector<float> offered(cells, 0.0f);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free || parent.esker[i] <= 0) continue;
            const Vec2 centre = domain.center(x, z);
            double closest = 1e30;
            for (size_t s = 0; s < count; ++s)
                if (const double d = length((tunnels[s].from + tunnels[s].to) * 0.5 - centre); d < closest) closest = d, nearest[i] = int32_t(s);
            const size_t s = size_t(nearest[i]);
            if (volume[s] <= 0) continue;
            offered[i] = std::min(domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i], parent.esker[i] * float(volume[s] / tunnels[s].fill));
            available[s] += offered[i];
        }
    // The ridge of each stretch over the cells nearest to its own span (its ends belong to the
    // stretches before and after it), drawn at least a cell wide and with its volume kept; the sum
    // runs over cells beyond the domain's edge too. Where the cover holds less, the ridge is lower.
    for (size_t s = 0; s < count; ++s) {
        if (volume[s] <= 0 || available[s] <= 0) continue;
        const Tunnel& tunnel = tunnels[s];
        const double reach = length(tunnel.to - tunnel.from);
        const double half = std::clamp(EskerSpread * std::pow(std::max(0.0, double(tunnel.discharge)) / 1e8, 0.4), double(EskerNarrowest), double(EskerWidest));
        const double drawn = std::max(half, domain.cell);
        const double laid = volume[s] * std::min(1.0, available[s] / wanted[s]);
        const int pieces = std::max(2, int(std::ceil(reach / (0.5 * domain.cell))));
        std::vector<Vec2> line(size_t(pieces) + 1);
        Vec2 low{1e30, 1e30}, high{-1e30, -1e30};
        for (int k = 0; k <= pieces; ++k) {
            const Vec2 p = along(tunnel.before, tunnel.from, tunnel.to, tunnel.after, double(k) / pieces);
            line[size_t(k)] = p;
            low = {std::min(low.x, p.x), std::min(low.z, p.z)}, high = {std::max(high.x, p.x), std::max(high.z, p.z)};
        }
        const int x0 = int(std::floor((low.x - drawn - domain.origin.x) / domain.cell)), z0 = int(std::floor((low.z - drawn - domain.origin.z) / domain.cell));
        const int x1 = int(std::floor((high.x + drawn - domain.origin.x) / domain.cell)), z1 = int(std::floor((high.z + drawn - domain.origin.z) / domain.cell));
        const size_t width = size_t(x1 - x0 + 1);
        std::vector<double> weight(width * size_t(z1 - z0 + 1), 0.0);
        double weights = 0;
        for (int z = z0; z <= z1; ++z)
            for (int x = x0; x <= x1; ++x) {
                const Vec2 centre{domain.origin.x + (x + 0.5) * domain.cell, domain.origin.z + (z + 0.5) * domain.cell};
                double closest = 1e30, where = 0;
                int piece = 0;
                for (int k = 0; k < pieces; ++k) {
                    double t = 0;
                    if (const double d = distanceToSegment(centre, line[size_t(k)], line[size_t(k) + 1], t); d < closest) closest = d, where = t, piece = k;
                }
                // Half-open: a cell beyond this stretch's start, or at or beyond its end, belongs to its neighbour.
                if ((piece == 0 && where < 0) || (piece == pieces - 1 && where >= 1)) continue;
                const double w = closest < drawn ? 1.0 - (closest / drawn) * (closest / drawn) : 0.0;
                weight[size_t(z - z0) * width + size_t(x - x0)] = w;
                weights += w;
            }
        if (weights <= 0) continue;
        for (int z = std::max(0, z0); z <= std::min(domain.size - 1, z1); ++z)
            for (int x = std::max(0, x0); x <= std::min(domain.size - 1, x1); ++x) {
                const size_t i = domain.index(x, z);
                const double w = weight[size_t(z - z0) * width + size_t(x - x0)];
                if (domain.hold[i] == Free && w > 0) fill.ridge[i] += float(laid * w / (weights * area));
            }
    }
    for (float& height : fill.ridge) height = std::min(height, EskerTallest);
    // Set aside, from each cell, its stretch's share of what the ridges hold.
    double coarse = 0, sand = 0, owned = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (offered[i] <= 0) continue;
            const size_t s = size_t(nearest[i]);
            float& gravel = domain.ground.sediment[Coarse][i];
            float& grit = domain.ground.sediment[Sand][i];
            const float taken = offered[i] * float(std::min(1.0, wanted[s] / available[s])), part = taken / (gravel + grit);
            coarse += gravel * part, sand += grit * part;
            gravel -= gravel * part, grit -= grit * part;
            domain.eroded[i] += taken;
            if (domain.owned(x, z)) owned += taken;
        }
    domain.ledger.eroded += owned * area;
    fill.ownedTaken = owned;
    const double found = coarse + sand;
    // A stretch whose ridge reaches past the domain's edge lays part of it there; what crosses the
    // edge is booked as such when the ridges are laid.
    fill.coarseShare = found > 0 ? coarse / found : 0.0;
    return fill;
}

void layEskers(Domain& domain, SheetWork& work, const EskerFill& fill) {
    const double area = domain.cell * domain.cell;
    double owned = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const float height = fill.ridge[i];
            if (height <= 0) continue;
            domain.ground.sediment[Coarse][i] += height * float(fill.coarseShare), domain.ground.sediment[Sand][i] += height * float(1 - fill.coarseShare);
            domain.deposited[i] += height;
            work.left.esker[i] += height;
            if (domain.owned(x, z)) owned += height;
        }
    // Set aside and laid match over the domain; what moved between the tile's cells and its halo crossed its edge.
    domain.ledger.deposited += owned * area;
    if (owned > fill.ownedTaken) domain.ledger.sedimentIn += (owned - fill.ownedTaken) * area;
    else domain.ledger.sedimentOut += (fill.ownedTaken - owned) * area;
}

} // namespace derivean::sim
