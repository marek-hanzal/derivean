#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"

namespace derivean::sim {
namespace {

// Behind high ground the wind is slack below a line that falls from it at LeeTan, and before high
// ground it stalls below a line that falls back from it at RampTan: sand that reaches either stays
// until it has filled the room up to that line. Over kilometres both are far gentler than the
// separation behind a single dune.
constexpr float LeeTan = 0.105f;       // 6°
constexpr float RampTan = 0.07f;       // 4°
constexpr float RiverHalf = 3e7f;      // m³/yr of a stream that, running all year, catches half the sand blown across it
constexpr float Bombard = 0.02f;       // m of fines raised per m of sand that passes over them
constexpr float DustReach = 1.0f;      // m of fines the passing sand can raise from a cell in one step
constexpr float BarSilt = 0.2f;        // m of silt the dry bars of a river that lays down its load give the wind in one step
constexpr float SandShield = 0.3f;     // m of wind-laid sand under which the fines of a cell are out of reach

Vec2 driftAt(const Domain& domain, size_t i) {
    const WindField& wind = domain.aeolian;
    return wind.driftX.empty() ? wind.regime.resultant : Vec2{wind.driftX[i], wind.driftZ[i]};
}

} // namespace

WindGraph windGraph(const Domain& domain) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    WindGraph graph;
    graph.to.assign(cells * 2, -1);
    graph.share.assign(cells * 2, 0.0f);
    std::vector<int32_t> waiting(cells, 0);
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const Vec2 drift = driftAt(domain, i);
            const double along = std::abs(drift.x) + std::abs(drift.z);
            if (along < 1e-6) continue;
            const int nx = x + (drift.x > 0 ? 1 : -1), nz = z + (drift.z > 0 ? 1 : -1);
            graph.share[i * 2] = float(std::abs(drift.x) / along), graph.share[i * 2 + 1] = float(std::abs(drift.z) / along);
            if (graph.share[i * 2] > 0 && nx >= 0 && nx < size) graph.to[i * 2] = int32_t(domain.index(nx, z));
            if (graph.share[i * 2 + 1] > 0 && nz >= 0 && nz < size) graph.to[i * 2 + 1] = int32_t(domain.index(x, nz));
            for (int k = 0; k < 2; ++k)
                if (graph.to[i * 2 + size_t(k)] >= 0) ++waiting[size_t(graph.to[i * 2 + size_t(k)])];
        }
    }
    graph.order.reserve(cells);
    for (size_t i = 0; i < cells; ++i)
        if (waiting[i] == 0) graph.order.push_back(int32_t(i));
    for (size_t done = 0; done < graph.order.size(); ++done)
        for (int k = 0; k < 2; ++k) {
            const int32_t next = graph.to[size_t(graph.order[done]) * 2 + size_t(k)];
            if (next >= 0 && --waiting[size_t(next)] == 0) graph.order.push_back(next);
        }
    // Where drifts of neighbouring cells turn against each other the graph closes on itself; those
    // cells come last, in index order, and pass on what has reached them by then.
    for (size_t i = 0; i < cells; ++i)
        if (waiting[i] > 0) graph.order.push_back(int32_t(i));
    return graph;
}

void routeSand(Domain& domain, const WindGraph& graph, std::vector<float>& raised) {
    WindField& wind = domain.aeolian;
    const size_t cells = domain.cells();
    const float cell = float(domain.cell), area = cell * cell, dt = float(domain.step);
    // The top the wind meets, and the room under the lee and ramp lines over it.
    std::vector<float> top(cells), lee(cells), ramp(cells), arriving(cells, 0.0f);
    std::vector<uint8_t> wet(cells);
    for (size_t i = 0; i < cells; ++i) {
        wet[i] = heldByWater(domain, i);
        const float ground = domain.ground.rock[i] + domain.sedimentAt(i);
        top[i] = lee[i] = ramp[i] = wet[i] ? std::max(domain.level[i], ground) + domain.ice[i] : ground;
    }
    for (const int32_t at : graph.order)
        for (int k = 0; k < 2; ++k)
            if (const int32_t next = graph.to[size_t(at) * 2 + size_t(k)]; next >= 0) lee[size_t(next)] = std::max(lee[size_t(next)], lee[size_t(at)] - LeeTan * cell);
    for (size_t step = graph.order.size(); step-- > 0;)
        for (int k = 0; k < 2; ++k)
            if (const int32_t next = graph.to[size_t(graph.order[step]) * 2 + size_t(k)]; next >= 0)
                ramp[size_t(graph.order[step])] = std::max(ramp[size_t(graph.order[step])], ramp[size_t(next)] - RampTan * cell);

    raised.assign(cells, 0.0f);
    std::vector<uint8_t> done(cells, 0);
    for (const int32_t at : graph.order) {
        const size_t i = size_t(at);
        done[i] = 1;
        const bool free = domain.hold[i] == Free;
        float sand = arriving[i];   // m³
        float out = 0;
        if (wet[i]) {
            // Sand blown into water joins its bed.
            if (free) domain.ground.sediment[Sand][i] += sand / area, wind.ledger.sandDrowned += sand, bookWind(domain, i, sand / area);
        } else {
            // A stream catches the sand blown across it while it runs, and in dry country it runs a small part of the year.
            const float dry = float(dryShare(domain.rain[i], domain.warmthNow[i], wind.halfRange));
            const float caught = sand * (1 - dry) * domain.discharge[i] / (domain.discharge[i] + RiverHalf);
            if (free) domain.ground.sediment[Sand][i] += caught / area, wind.ledger.sandDrowned += caught, bookWind(domain, i, caught / area);
            sand -= caught;
            const float carry = float(length(driftAt(domain, i))) * dry * dt * cell;
            const float room = std::max(0.0f, std::max(lee[i], ramp[i]) - top[i]) * area;
            const float own = mobileSand(domain, i) * area;
            // In shelter a cell keeps what arrives until its room is full and gives none of its own.
            const float kept = std::min(sand, room);
            out = room > 0 ? std::min(carry, sand - kept) : std::min(carry, sand + own);
            const float change = sand - out;   // m³ the cell gains
            if (free && change >= 0) layBlown(domain, i, change / area), wind.ledger.sandLaid += change;
            if (free && change < 0) raised[i] += takeSand(domain, i, -change / area), wind.ledger.sandLifted += -change;
            if (free) {
                // Passing sand strikes the fines it crosses into the air; the dry bars of a river
                // that is laying down its load give theirs without being struck.
                const WindLabels& labels = wind.labels;
                const bool open = labels.blown[i] < SandShield && labels.lag[i] < LagArmour;
                const float fines = std::max(0.0f, domain.ground.sediment[Fines][i] - labels.salt[i]);
                const float struck = open ? std::min(std::min(fines, DustReach), Bombard * out / area) : 0.0f;
                const bool bars = domain.depositionRate[i] > 0 && domain.discharge[i] >= ChannelHeads[size_t(domain.key.level)];
                const float given = bars ? dry * std::min(fines - struck, BarSilt) : 0.0f;
                domain.ground.sediment[Fines][i] -= struck + given;
                bookWind(domain, i, -(struck + given));
                raised[i] += struck + given;
            }
        }
        for (int k = 0; k < 2; ++k) {
            const float part = out * graph.share[i * 2 + size_t(k)];
            const int32_t next = graph.to[i * 2 + size_t(k)];
            if (part <= 0) continue;
            const bool onto = next >= 0 && domain.hold[size_t(next)] == Free;
            // Where the graph closes on itself a cell can hand sand to one already dealt with: it stays there.
            if (next >= 0 && !done[size_t(next)]) arriving[size_t(next)] += part;
            else if (onto) layBlown(domain, size_t(next), part / area), wind.ledger.sandLaid += part;
            if (free && !onto) wind.ledger.sandOut += part;
            if (!free && onto) wind.ledger.sandIn += part;
        }
    }
    for (size_t i = 0; i < cells; ++i) wind.ledger.dustRaised += double(raised[i]) * area;
}

void blowLoose(Domain& domain, FlowScratch& scratch) {
    if (domain.key.level == Level::Ground) return;
    if (domain.aeolian.labels.blown.size() != domain.cells()) allocateWind(domain);
    if (domain.key.level != Level::Region) {
        // The finer levels settle; their wind works once, on what has settled.
        if (domain.time + 1.5 * domain.step >= domain.endTime) shapeDunes(domain);
        return;
    }
    WindField& wind = domain.aeolian;
    for (size_t i = 0; i < domain.cells(); ++i) {
        domain.ground.sediment[Sand][i] += wind.parked[i], domain.ground.sediment[Fines][i] += wind.parkedLoess[i];
        wind.parked[i] = wind.parkedLoess[i] = 0;
    }
    tidyLabels(domain);
    dryBasins(domain, scratch);
    const WindGraph graph = windGraph(domain);
    std::vector<float> raised;
    routeSand(domain, graph, raised);
    settleDust(domain, graph, raised);
    // Until the last step what the wind has laid waits beside the ground for the wind's next turn.
    if (domain.time + 1.5 * domain.step >= domain.endTime) return;
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.hold[i] != Free) continue;
        wind.parked[i] = std::min(wind.labels.blown[i], std::max(0.0f, domain.ground.sediment[Sand][i]));
        wind.parkedLoess[i] = std::min(wind.labels.loess[i], std::max(0.0f, domain.ground.sediment[Fines][i] - wind.labels.salt[i]));
        domain.ground.sediment[Sand][i] -= wind.parked[i], domain.ground.sediment[Fines][i] -= wind.parkedLoess[i];
    }
}

} // namespace derivean::sim
