#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"
#include "landscape/fn/Process.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

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
    // its foot than gravel stands; the rest stays spread in the cover. Its section is set by its own
    // fill over its own length, so it stands the same in every domain that holds it; narrower than
    // a cell, it is drawn over the cells its path crosses with the same section.
    const size_t count = tunnels.size();
    std::vector<double> section(count, 0.0), drawn(count, 0.0), fits(count, 0.0);
    for (size_t s = 0; s < count; ++s) {
        const Tunnel& tunnel = tunnels[s];
        const double reach = length(tunnel.to - tunnel.from);
        if (reach <= 0 || tunnel.fill <= 0) continue;
        const double half = std::clamp(EskerSpread * std::pow(std::max(0.0, double(tunnel.discharge)) / 1e8, 0.4), double(EskerNarrowest), double(EskerWidest));
        const double crest = std::min(double(EskerTallest), 0.5 * ReposeTan[Coarse] * half);
        const double volume = std::min(tunnel.fill, 4.0 / 3.0 * crest * half * reach);
        section[s] = volume / reach, drawn[s] = std::max(half, domain.cell), fits[s] = volume / tunnel.fill;
    }
    std::vector<int32_t> owner(cells, -1);
    for (const EskerTrack& track : traceEskers(domain, parent, tunnels)) {
        Vec2 low{1e30, 1e30}, high{-1e30, -1e30};
        double widest = 0;
        for (size_t k = 0; k < track.points.size(); ++k) {
            const Vec2 p = track.points[k];
            low = {std::min(low.x, p.x), std::min(low.z, p.z)}, high = {std::max(high.x, p.x), std::max(high.z, p.z)};
            widest = std::max(widest, drawn[size_t(track.stretch[k])]);
        }
        const int x0 = std::max(0, int(std::floor((low.x - widest - domain.origin.x) / domain.cell))), z0 = std::max(0, int(std::floor((low.z - widest - domain.origin.z) / domain.cell)));
        const int x1 = std::min(domain.size - 1, int(std::floor((high.x + widest - domain.origin.x) / domain.cell))), z1 = std::min(domain.size - 1, int(std::floor((high.z + widest - domain.origin.z) / domain.cell)));
        for (int z = z0; z <= z1; ++z)
            for (int x = x0; x <= x1; ++x) {
                const size_t i = domain.index(x, z);
                if (domain.hold[i] != Free) continue;
                double closest = 1e30, where = 0;
                size_t piece = 0;
                for (size_t k = 0; k + 1 < track.points.size(); ++k) {
                    double t = 0;
                    if (const double d = distanceToSegment(domain.center(x, z), track.points[k], track.points[k + 1], t); d < closest) closest = d, piece = k, where = t;
                }
                const size_t s = size_t(track.stretch[where < 0.5 ? piece : piece + 1]);
                if (section[s] <= 0 || closest >= drawn[s]) continue;
                const float height = float(section[s] / (4.0 / 3.0 * drawn[s]) * (1.0 - (closest / drawn[s]) * (closest / drawn[s])));
                if (height > fill.ridge[i]) fill.ridge[i] = height, owner[i] = int32_t(s);
            }
    }
    // The gravel and sand come out of the cover the parent spread them in: from each cell the share
    // of what the parent spread there that the nearest stretch's ridge holds.
    std::vector<int32_t> nearest(cells, -1);
    std::vector<float> offered(cells, 0.0f);
    std::vector<double> wanted(count, 0.0), available(count, 0.0);
    for (size_t i = 0; i < cells; ++i)
        if (owner[i] >= 0) wanted[size_t(owner[i])] += fill.ridge[i];
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free || parent.esker[i] <= 0) continue;
            const Vec2 centre = domain.center(x, z);
            double closest = 1e30;
            for (size_t s = 0; s < count; ++s)
                if (const double d = length((tunnels[s].from + tunnels[s].to) * 0.5 - centre); d < closest) closest = d, nearest[i] = int32_t(s);
            const size_t s = size_t(nearest[i]);
            if (fits[s] <= 0) continue;
            offered[i] = std::min(domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i], parent.esker[i] * float(fits[s]));
            available[s] += offered[i];
        }
    // Where the cover holds less than a stretch's ridge, its ridge is lower; otherwise it takes only what its ridge holds.
    for (size_t i = 0; i < cells; ++i) {
        if (owner[i] < 0) continue;
        const size_t s = size_t(owner[i]);
        fill.ridge[i] = available[s] > 0 ? std::min(fill.ridge[i] * float(std::min(1.0, available[s] / wanted[s])), EskerTallest) : 0.0f;
    }
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
