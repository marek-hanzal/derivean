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
    // fill over its own length, lowered by the share of that fill the parent's cover still holds:
    // parent fields alone, so every domain that holds the stretch draws the same ridge. Narrower
    // than a cell, it is drawn over the cells its path crosses with the same section.
    const size_t count = tunnels.size();
    std::vector<double> section(count, 0.0), drawn(count, 0.0);
    for (size_t s = 0; s < count; ++s) {
        const Tunnel& tunnel = tunnels[s];
        const double reach = length(tunnel.to - tunnel.from);
        if (reach <= 0 || tunnel.fill <= 0) continue;
        const double half = std::clamp(EskerSpread * std::pow(std::max(0.0, double(tunnel.discharge)) / 1e8, 0.4), double(EskerNarrowest), double(EskerWidest));
        const double crest = std::min(double(EskerTallest), 0.5 * ReposeTan[Coarse] * half);
        const double volume = std::min(tunnel.fill, 4.0 / 3.0 * crest * half * reach);
        section[s] = volume / reach * std::min(1.0, tunnel.supply / tunnel.fill), drawn[s] = std::max(half, domain.cell);
    }
    // Each track's ridge over the cells within its width of an open step, the nearest step's
    // stretch setting its section; where tracks meet, the higher ridge. It is drawn over cells a
    // finished neighbour holds too, for what it takes, and laid only on the domain's own.
    std::vector<float> closest(cells, 1e30f);
    std::vector<int32_t> nearestStretch(cells, -1), owner(cells, -1);
    std::vector<size_t> touched;
    for (const EskerTrack& track : traceEskers(domain, parent, tunnels)) {
        touched.clear();
        for (size_t k = 0; k + 1 < track.points.size(); ++k) {
            const size_t s = size_t(track.stretch[k]);
            if (!track.open[k] || section[s] <= 0) continue;
            const Vec2 a = track.points[k], b = track.points[k + 1];
            const double reach = drawn[s];
            const int x0 = std::max(0, int(std::floor((std::min(a.x, b.x) - reach - domain.origin.x) / domain.cell)));
            const int z0 = std::max(0, int(std::floor((std::min(a.z, b.z) - reach - domain.origin.z) / domain.cell)));
            const int x1 = std::min(domain.size - 1, int(std::floor((std::max(a.x, b.x) + reach - domain.origin.x) / domain.cell)));
            const int z1 = std::min(domain.size - 1, int(std::floor((std::max(a.z, b.z) + reach - domain.origin.z) / domain.cell)));
            for (int z = z0; z <= z1; ++z)
                for (int x = x0; x <= x1; ++x) {
                    const size_t i = domain.index(x, z);
                    if (domain.hold[i] == Rim) continue;
                    double t = 0;
                    const float d = float(distanceToSegment(domain.center(x, z), a, b, t));
                    if (d >= float(reach) || d >= closest[i]) continue;
                    if (closest[i] >= 1e30f) touched.push_back(i);
                    closest[i] = d, nearestStretch[i] = int32_t(s);
                }
        }
        for (const size_t i : touched) {
            const size_t s = size_t(nearestStretch[i]);
            const double off = closest[i] / drawn[s];
            const float height = float(section[s] / (4.0 / 3.0 * drawn[s]) * (1.0 - off * off));
            if (height > fill.ridge[i]) fill.ridge[i] = height, owner[i] = int32_t(s);
            closest[i] = 1e30f;
        }
    }
    // The gravel and sand of each stretch's ridge come out of the cover of the parent cell it starts
    // in, where the parent spread them: in proportion to what the parent spread over each of this
    // domain's cells there, as far as the cell's cover holds it, the cells with cover to spare
    // giving what the bare ones cannot. The tile that owns that cell takes for the whole ridge,
    // also where a finished neighbour laid it, and books what lies off the tile as leaving; a ridge
    // laid here from a stretch whose cell a finished neighbour owns was taken there. What no cover
    // can give is laid all the same and booked as arriving along the tunnel, unmatched.
    std::vector<double> wanted(count, 0.0), spread(count, 0.0), owed(count, 0.0), spare(count, 0.0);
    for (size_t i = 0; i < cells; ++i) {
        if (owner[i] >= 0) wanted[size_t(owner[i])] += fill.ridge[i];
        if (domain.hold[i] != Free) fill.ridge[i] = 0;
    }
    const double parentCell = domain.cell * Refinement;
    const int64_t baseX = int64_t(std::floor(domain.origin.x / parentCell)), baseZ = int64_t(std::floor(domain.origin.z / parentCell));
    const int span = int(std::ceil(domain.size / double(Refinement))) + 2;
    std::vector<int32_t> stretchOf(size_t(span) * size_t(span), -1);
    for (size_t s = 0; s < count; ++s) {
        const int64_t px = int64_t(std::floor(tunnels[s].from.x / parentCell)) - baseX, pz = int64_t(std::floor(tunnels[s].from.z / parentCell)) - baseZ;
        if (px >= 0 && pz >= 0 && px < span && pz < span) stretchOf[size_t(pz) * size_t(span) + size_t(px)] = int32_t(s);
    }
    std::vector<int32_t> source(cells, -1);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free || parent.esker[i] <= 0) continue;
            const Vec2 centre = domain.center(x, z);
            const int64_t px = int64_t(std::floor(centre.x / parentCell)) - baseX, pz = int64_t(std::floor(centre.z / parentCell)) - baseZ;
            source[i] = stretchOf[size_t(pz) * size_t(span) + size_t(px)];
            if (source[i] >= 0) spread[size_t(source[i])] += parent.esker[i];
        }
    std::vector<float> taken(cells, 0.0f);
    for (size_t i = 0; i < cells; ++i) {
        if (source[i] < 0) continue;
        const size_t s = size_t(source[i]);
        const float cover = std::max(0.0f, domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i]);
        const float share = float(wanted[s] * parent.esker[i] / spread[s]);
        taken[i] = std::min(cover, share);
        owed[s] += share - taken[i], spare[s] += cover - taken[i];
    }
    double coarse = 0, sand = 0, owned = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (source[i] < 0) continue;
            const size_t s = size_t(source[i]);
            float& gravel = domain.ground.sediment[Coarse][i];
            float& grit = domain.ground.sediment[Sand][i];
            const float cover = std::max(0.0f, gravel + grit);
            if (spare[s] > 0) taken[i] += (cover - taken[i]) * float(std::min(1.0, owed[s] / spare[s]));
            if (taken[i] <= 0 || cover <= 0) continue;
            const float part = std::min(1.0f, taken[i] / cover), removed = cover * part;
            coarse += gravel * part, sand += grit * part;
            gravel -= gravel * part, grit -= grit * part;
            domain.eroded[i] += removed;
            if (domain.owned(x, z)) owned += removed;
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
    // Laid on the tile's cells and set aside from them differ by what the tunnel brought across its edge.
    domain.ledger.deposited += owned * area;
    if (owned > fill.ownedTaken) domain.ledger.sedimentIn += (owned - fill.ownedTaken) * area;
    else domain.ledger.sedimentOut += (fill.ownedTaken - owned) * area;
}

} // namespace derivean::sim
