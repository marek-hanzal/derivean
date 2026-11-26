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

// Which tile of the 3 × 3 around the domain's own holds domain column (or row) x: -1, 0 or 1.
int sideOf(int x) { return x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0; }

// Each track's ridge over the domain's own and unfinished cells within its width of an open step;
// where tracks meet, the higher ridge. A cell belongs to the stretch of the nearest step, or to the
// one before or after it where the cell lies past the bisector of the bend at the end they share:
// the track's steps fall differently in every domain, the stretches' ends do not.
void drawRidges(const Domain& domain, const ParentSheet& parent, const std::vector<Tunnel>& tunnels, const std::vector<double>& section,
                const std::vector<double>& drawn, std::vector<float>& ridge, std::vector<int32_t>& owner) {
    const size_t cells = domain.cells();
    std::vector<float> closest(cells, 1e30f);
    std::vector<int32_t> nearestStep(cells, -1);
    std::vector<size_t> touched;
    for (const EskerTrack& track : traceEskers(domain, parent, tunnels)) {
        touched.clear();
        const size_t steps = track.stretch.size();
        std::vector<int32_t> before(steps, -1), after(steps, -1);
        for (size_t k = 1; k < steps; ++k) before[k] = track.stretch[k - 1] != track.stretch[k] ? track.stretch[k - 1] : before[k - 1];
        for (size_t k = steps; k >= 2; --k) after[k - 2] = track.stretch[k - 1] != track.stretch[k - 2] ? track.stretch[k - 1] : after[k - 1];
        for (size_t k = 0; k + 1 < track.points.size(); ++k) {
            const size_t s = size_t(track.stretch[k]);
            if (!track.open[k] || section[s] <= 0) continue;
            const Vec2 a = track.points[k], b = track.points[k + 1];
            double reach = drawn[s];
            for (const int32_t other : {before[k], after[k]})
                if (other >= 0) reach = std::max(reach, drawn[size_t(other)]);
            const int x0 = std::max(0, int(std::floor((std::min(a.x, b.x) - reach - domain.origin.x) / domain.cell)));
            const int z0 = std::max(0, int(std::floor((std::min(a.z, b.z) - reach - domain.origin.z) / domain.cell)));
            const int x1 = std::min(domain.size - 1, int(std::floor((std::max(a.x, b.x) + reach - domain.origin.x) / domain.cell)));
            const int z1 = std::min(domain.size - 1, int(std::floor((std::max(a.z, b.z) + reach - domain.origin.z) / domain.cell)));
            for (int z = z0; z <= z1; ++z)
                for (int x = x0; x <= x1; ++x) {
                    const size_t i = domain.index(x, z);
                    if (domain.hold[i] != Free) continue;
                    double t = 0;
                    const float d = float(distanceToSegment(domain.center(x, z), a, b, t));
                    if (d >= float(reach) || d >= closest[i]) continue;
                    if (closest[i] >= 1e30f) touched.push_back(i);
                    closest[i] = d, nearestStep[i] = int32_t(k);
                }
        }
        for (const size_t i : touched) {
            const size_t k = size_t(nearestStep[i]);
            size_t s = size_t(track.stretch[k]);
            const Vec2 centre = domain.center(int(i % size_t(domain.size)), int(i / size_t(domain.size)));
            const auto way = [&](size_t t) { return (tunnels[t].to - tunnels[t].from) * (1.0 / std::max(1e-9, length(tunnels[t].to - tunnels[t].from))); };
            if (const int32_t next = after[k]; next >= 0 && section[size_t(next)] > 0 && dot(centre - tunnels[s].to, way(s) + way(size_t(next))) > 0) s = size_t(next);
            else if (const int32_t last = before[k]; last >= 0 && section[size_t(last)] > 0 && dot(centre - tunnels[s].from, way(size_t(last)) + way(s)) < 0) s = size_t(last);
            const double off = closest[i] / drawn[s];
            if (off >= 1) {
                closest[i] = 1e30f;
                continue;
            }
            const float height = float(section[s] / (4.0 / 3.0 * drawn[s]) * (1.0 - off * off));
            if (height > ridge[i]) ridge[i] = height, owner[i] = int32_t(s);
            closest[i] = 1e30f;
        }
    }
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
    // parent fields alone. Narrower than a cell, it is drawn over the cells its path crosses.
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
    std::vector<int32_t> owner(cells, -1);
    drawRidges(domain, parent, tunnels, section, drawn, fill.ridge, owner);
    // The gravel and sand of each stretch's ridge come out of the cover of the parent cell it starts
    // in, where the parent spread them, and the ridge stands only as high as that cover gives.
    // Where the start cell is not a finished neighbour's, this domain lowers the stretch by one
    // share: the cover there against what finished neighbours recorded laying of the ridge and what
    // this domain draws of it. The tile that owns the start takes for all of it and records what it
    // took for each unfinished neighbour's part; the neighbour, finished later, lays that much along
    // its own drawing. A tile that lays part of a ridge starting on an unfinished neighbour records
    // it, for that neighbour to take.
    const double parentCell = domain.cell * Refinement;
    const int64_t baseX = int64_t(std::floor(domain.origin.x / parentCell)), baseZ = int64_t(std::floor(domain.origin.z / parentCell));
    const int span = int(std::ceil(domain.size / double(Refinement))) + 2;
    std::vector<int32_t> stretchOf(size_t(span) * size_t(span), -1);
    for (size_t s = 0; s < count; ++s) {
        const int64_t px = int64_t(std::floor(tunnels[s].from.x / parentCell)) - baseX, pz = int64_t(std::floor(tunnels[s].from.z / parentCell)) - baseZ;
        if (px >= 0 && pz >= 0 && px < span && pz < span) stretchOf[size_t(pz) * size_t(span) + size_t(px)] = int32_t(s);
    }
    const auto stretchAt = [&](int64_t x, int64_t z) {
        const int64_t px = x - baseX, pz = z - baseZ;
        return px >= 0 && pz >= 0 && px < span && pz < span ? stretchOf[size_t(pz) * size_t(span) + size_t(px)] : -1;
    };
    // m·cells finished neighbours laid of stretches starting off them, and took for this tile's part.
    std::vector<double> laid(count, 0.0), sent(count, 0.0);
    for (int k = 0; k < 9; ++k) {
        const Tile* neighbour = domain.finished[k];
        if (!neighbour || !neighbour->sheet) continue;
        for (const EskerCrossing& crossing : neighbour->sheet->eskerArrived)
            if (const int32_t s = stretchAt(crossing.x, crossing.z); s >= 0) laid[size_t(s)] += crossing.volume / area;
        for (const EskerCrossing& crossing : neighbour->sheet->eskerSent)
            if (const int32_t s = stretchAt(crossing.x, crossing.z); s >= 0 && crossing.tx == 1 - k % 3 && crossing.tz == 1 - k / 3) sent[size_t(s)] += crossing.volume / area;
    }
    std::vector<double> laying(count, 0.0), own(count, 0.0), cover(count, 0.0), spread(count, 0.0), share(count, 0.0);
    std::vector<int32_t> source(cells, -1);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (owner[i] >= 0) laying[size_t(owner[i])] += fill.ridge[i];
            if (owner[i] >= 0 && domain.owned(x, z)) own[size_t(owner[i])] += fill.ridge[i];
            if (domain.hold[i] != Free || parent.esker[i] <= 0) continue;
            const Vec2 centre = domain.center(x, z);
            source[i] = stretchAt(int64_t(std::floor(centre.x / parentCell)), int64_t(std::floor(centre.z / parentCell)));
            if (source[i] < 0) continue;
            spread[size_t(source[i])] += parent.esker[i];
            cover[size_t(source[i])] += std::max(0.0f, domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i]);
        }
    std::vector<uint8_t> start(count, Rim);
    std::vector<bool> startsHere(count, false);
    for (size_t s = 0; s < count; ++s) {
        const int x = int(std::floor((tunnels[s].from.x - domain.origin.x) / domain.cell)), z = int(std::floor((tunnels[s].from.z - domain.origin.z) / domain.cell));
        if (x < 0 || z < 0 || x >= domain.size || z >= domain.size) continue;
        start[s] = domain.hold[domain.index(x, z)], startsHere[s] = domain.owned(x, z);
        // A finished owner took for this tile's part already, no more than it laid along its own
        // drawing of it; what its drawing holds beyond this tile's stays taken and is laid nowhere,
        // booked out by the owner with nobody booking it in. Where the start is this tile's and a
        // finished neighbour laid its part from its halo's view of the start's cover, the cover
        // here may hold less: that difference is laid without a source.
        if (start[s] == Settled) share[s] = own[s] > 0 ? std::min(1.0, sent[s] / own[s]) : 0.0;
        if (start[s] == Free) share[s] = laying[s] > 0 ? std::clamp((cover[s] - laid[s]) / laying[s], 0.0, 1.0) : 0.0;
    }
    std::vector<double> arrived(count, 0.0), toward(count * 9, 0.0);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (owner[i] < 0) continue;
            const size_t s = size_t(owner[i]);
            fill.ridge[i] *= float(share[s]);
            if (domain.owned(x, z) && start[s] == Free && !startsHere[s]) arrived[s] += fill.ridge[i] * area;
            if (!domain.owned(x, z) && startsHere[s]) toward[s * 9 + size_t((sideOf(z) + 1) * 3 + sideOf(x) + 1)] += fill.ridge[i] * area;
        }
    for (size_t s = 0; s < count; ++s) {
        const int64_t px = int64_t(std::floor(tunnels[s].from.x / parentCell)), pz = int64_t(std::floor(tunnels[s].from.z / parentCell));
        if (arrived[s] > 0) fill.arrived.push_back({px, pz, 0, 0, arrived[s]});
        for (int k = 0; k < 9; ++k)
            if (toward[s * 9 + size_t(k)] > 0) fill.sent.push_back({px, pz, int8_t(k % 3 - 1), int8_t(k / 3 - 1), toward[s * 9 + size_t(k)]});
    }
    // Spread as the parent spread it, as far as each cell's cover holds it, the cells with cover to
    // spare giving what the bare ones cannot.
    std::vector<double> owed(count, 0.0), spare(count, 0.0);
    std::vector<float> taken(cells, 0.0f);
    for (size_t i = 0; i < cells; ++i) {
        if (source[i] < 0) continue;
        const size_t s = size_t(source[i]);
        const float here = std::max(0.0f, domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i]);
        const float wanted = float(std::min(cover[s], laid[s] + share[s] * laying[s]) * parent.esker[i] / spread[s]);
        taken[i] = std::min(here, wanted);
        owed[s] += wanted - taken[i], spare[s] += here - taken[i];
    }
    double coarse = 0, sand = 0, owned = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (source[i] < 0) continue;
            const size_t s = size_t(source[i]);
            float& gravel = domain.ground.sediment[Coarse][i];
            float& grit = domain.ground.sediment[Sand][i];
            const float here = std::max(0.0f, gravel + grit);
            if (spare[s] > 0) taken[i] += (here - taken[i]) * float(std::min(1.0, owed[s] / spare[s]));
            if (taken[i] <= 0 || here <= 0) continue;
            const float part = std::min(1.0f, taken[i] / here), removed = here * part;
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
    work.left.eskerArrived = fill.arrived, work.left.eskerSent = fill.sent;
    // Laid on the tile's cells and set aside from them differ by what the tunnel brought across its edge.
    domain.ledger.deposited += owned * area;
    if (owned > fill.ownedTaken) domain.ledger.sedimentIn += (owned - fill.ownedTaken) * area;
    else domain.ledger.sedimentOut += (fill.ownedTaken - owned) * area;
}

} // namespace derivean::sim
