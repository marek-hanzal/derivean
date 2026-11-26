#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"

namespace derivean::sim {
namespace {

// Point at `t` (0..1) of the Catmull-Rom curve between b and c with a before and d after: the
// stream's path through the parent's cell centres without the corners of their grid.
Vec2 along(Vec2 a, Vec2 b, Vec2 c, Vec2 d, double t) {
    const double t2 = t * t, t3 = t2 * t;
    return (b * 2.0 + (c - a) * t + (a * 2.0 - b * 5.0 + c * 4.0 - d) * t2 + (b * 3.0 - a - c * 3.0 + d) * t3) * 0.5;
}

double distanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    const Vec2 ab = b - a;
    const double span = dot(ab, ab), t = span > 0 ? std::clamp(dot(p - a, ab) / span, 0.0, 1.0) : 0.0;
    return length(p - (a + ab * t));
}

} // namespace

EskerFill setAsideEskers(Domain& domain, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const size_t cells = domain.cells();
    EskerFill fill;
    fill.ridge.assign(cells, 0.0f);
    if (tunnels.empty()) return fill;
    // Each stretch: a fill of parabolic section along the curve, its crest set by the stretch's own
    // volume over its length, so it stands as high in every domain that holds it. Where stretches
    // meet, the higher stands.
    for (const Tunnel& tunnel : tunnels) {
        const double reach = length(tunnel.to - tunnel.from);
        if (reach <= 0) continue;
        const double half = std::clamp(EskerSpread * std::pow(double(tunnel.discharge) / 1e8, 0.4), double(EskerNarrowest), double(EskerWidest));
        const double crest = tunnel.fill / reach * 3.0 / (4.0 * half);
        const int pieces = std::max(2, int(std::ceil(reach / (0.5 * domain.cell))));
        std::vector<Vec2> line(size_t(pieces) + 1);
        Vec2 low{1e30, 1e30}, high{-1e30, -1e30};
        for (int k = 0; k <= pieces; ++k) {
            const Vec2 p = along(tunnel.before, tunnel.from, tunnel.to, tunnel.after, double(k) / pieces);
            line[size_t(k)] = p;
            low = {std::min(low.x, p.x), std::min(low.z, p.z)}, high = {std::max(high.x, p.x), std::max(high.z, p.z)};
        }
        const int x0 = std::max(0, int(std::floor((low.x - half - domain.origin.x) / domain.cell)));
        const int z0 = std::max(0, int(std::floor((low.z - half - domain.origin.z) / domain.cell)));
        const int x1 = std::min(domain.size - 1, int(std::floor((high.x + half - domain.origin.x) / domain.cell)));
        const int z1 = std::min(domain.size - 1, int(std::floor((high.z + half - domain.origin.z) / domain.cell)));
        for (int z = z0; z <= z1; ++z)
            for (int x = x0; x <= x1; ++x) {
                const size_t i = domain.index(x, z);
                if (domain.hold[i] != Free) continue;
                double nearest = 1e30;
                for (int k = 0; k < pieces; ++k) nearest = std::min(nearest, distanceToSegment(domain.center(x, z), line[size_t(k)], line[size_t(k) + 1]));
                if (nearest < half) fill.ridge[i] = std::max(fill.ridge[i], float(crest * (1.0 - (nearest / half) * (nearest / half))));
            }
    }
    double wanted = 0, spread = 0, coarse = 0, sand = 0;
    for (size_t i = 0; i < cells; ++i) {
        wanted += fill.ridge[i];
        if (domain.hold[i] == Free) spread += parent.esker[i];
    }
    if (wanted <= 0 || spread <= 0) {
        fill.ridge.assign(cells, 0.0f);
        return fill;
    }
    // The gravel and sand come out of the cover the parent spread them in, from each cell in
    // proportion to what the parent spread there. Where the cover holds less, the ridges are lower.
    const double area = domain.cell * domain.cell;
    const float share = float(wanted / spread);
    double owned = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            float& gravel = domain.ground.sediment[Coarse][i];
            float& grit = domain.ground.sediment[Sand][i];
            if (domain.hold[i] != Free || parent.esker[i] <= 0 || gravel + grit <= 0) continue;
            const float taken = std::min(gravel + grit, share * parent.esker[i]), part = taken / (gravel + grit);
            coarse += gravel * part, sand += grit * part;
            gravel -= gravel * part, grit -= grit * part;
            domain.eroded[i] += taken;
            if (domain.owned(x, z)) owned += taken;
        }
    domain.ledger.eroded += owned * area;
    fill.ownedTaken = owned;
    const double found = coarse + sand;
    const float laid = float(found / wanted);
    for (float& height : fill.ridge) height *= laid;
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
