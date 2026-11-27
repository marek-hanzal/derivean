#include "coast/fn/Coast.hpp"

#include "landscape/fn/Process.hpp"

// About 270 lines: one step of the coast and the booking of what it did, which reads the state the
// step began with; apart, the booking would need that state handed to it field by field.

namespace derivean::sim {
namespace {

// Years one pass of the eight trains over the sand and gravel of a shore stands for: a step takes
// as many as its years hold, so a shore follows its years, not how they are cut into steps. A
// region's steps of thousands of years take MostRounds, each standing for more.
constexpr double SweepYears = 25.0;
constexpr int MostRounds = 8;
constexpr float RateWeight = 0.4f;
// m before a lake's sill over which the path of its water under the lake is kept open: the
// shallows where the swash of lake waves builds a bar. A calibration on the lake scene, whose
// waves barred the approach within 60 m of its sill.
constexpr double ApproachReach = 64.0;

float looseAt(const Domain& domain, size_t i) { return domain.ground.sediment[0][i] + domain.ground.sediment[1][i] + domain.ground.sediment[2][i]; }


// Entries in the tile's ledger and its per-cell totals for what the waves did this step, and the
// surface a finer level is held to moved by the same: a bench or a spit is relief of the level's
// own, which its parent cannot have and its found uplift must not take back.
void book(Domain& domain, Shore& shore, const std::vector<float>& cutBefore) {
    const float dt = float(domain.step);
    const double area = domain.cell * domain.cell;
    // Over the whole domain: mud sinks wherever its cloud ends, which may lie outside the shore's box.
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free) continue;
            const float broken = shore.cut[i] - cutBefore[i], gained = looseAt(domain, i) - shore.before[i];
            if (broken == 0 && gained == 0) continue;
            const float lost = broken + std::max(0.0f, -gained), won = std::max(0.0f, gained);
            shore.laid[i] += gained;
            // Invariant: `trend` follows every metre the waves cut or lay, or the uplift a finer
            // level finds for itself (landscape/fn/Uplift.cpp) would raise the bench again.
            domain.trend[i] += gained - broken;
            domain.eroded[i] += lost;
            domain.deposited[i] += won;
            // The rates are moving averages (landscape/fn/Fluvial.cpp keeps them with the same share
            // per step): the waves' work of one step enters them as that share of its rate.
            domain.erosionRate[i] += RateWeight * lost / dt;
            domain.depositionRate[i] += RateWeight * won / dt;
            if (!domain.owned(x, z)) continue;
            domain.ledger.eroded += double(lost) * area;
            domain.ledger.deposited += double(won) * area;
        }
    }
}

// m³ of broken rock waiting in the surf of the tile's own cells, which are all Free.
double pendingOwned(const Domain& domain, const Shore& shore) {
    double sum = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x)
            if (domain.owned(x, z))
                for (const auto& kind : shore.pending) sum += kind[domain.index(x, z)];
    return sum * domain.cell * domain.cell;
}

} // namespace

Shore& shoreOf(Domain& domain) {
    if (!domain.shore) domain.shore = std::make_shared<Shore>();
    Shore& shore = *domain.shore;
    if (shore.top.size() == domain.cells()) return shore;
    const size_t cells = domain.cells();
    shore.wave.assign(cells * WaveDirections, 0.0f);
    shore.arrive.assign(cells * WaveDirections, 0.0f);
    shore.crest.assign(cells * WaveDirections, 0.0f);
    shore.gale.resize(cells * WaveDirections);
    for (size_t i = 0; i < cells; ++i)
        for (int k = 0; k < WaveDirections; ++k) shore.gale[trainIndex(cells, i, k)] = stormWind(domain, i, k) * stormWind(domain, i, k);
    for (auto* field : {&shore.top, &shore.notch, &shore.cut, &shore.laid, &shore.before, &shore.pending[0], &shore.pending[1], &shore.pending[2], &shore.load[0],
                        &shore.load[1], &shore.load[2]})
        field->assign(cells, 0.0f);
    shore.stood.assign(cells, NoWater);
    shore.stream.assign(cells, 0);
    shore.still.assign(cells, NoWater);
    shore.surf.assign(cells, 0);
    return shore;
}

// Still water is what waves ride on. It reaches every cell that it covers at high water or that
// the swash of the highest waves can run over, however far that is from where it stands at its
// mean level: across a bench that dries at low tide, over a bar. The ground next to such cells is
// given its level too: that is what its waves strike.
bool readStanding(const Domain& domain, Shore& shore) {
    bool any = false;
    for (size_t i = 0; i < domain.cells(); ++i) {
        shore.top[i] = domain.ground.rock[i] + looseAt(domain, i);
        const bool stands = standsOver(domain, i, shore.top[i]);
        shore.stood[i] = stands ? domain.level[i] : NoWater;
        any = any || stands;
    }
    return any;
}

bool spreadWater(const Domain& domain, Shore& shore) {
    const int size = domain.size;
    std::vector<int32_t> reached;
    // Trains, surf and loads belong to the water of this step only: a cell outside its box must not
    // keep what an earlier, larger water left there.
    std::fill(shore.still.begin(), shore.still.end(), NoWater);
    for (auto* field : {&shore.wave, &shore.arrive, &shore.crest}) std::fill(field->begin(), field->end(), 0.0f);
    shore.raised.valid = false;
    std::fill(shore.surf.begin(), shore.surf.end(), uint8_t(0));
    for (auto& load : shore.load) std::fill(load.begin(), load.end(), 0.0f);
    for (size_t i = 0; i < domain.cells(); ++i)
        if (shore.stood[i] != NoWater && shore.stood[i] > shore.top[i]) shore.still[i] = shore.stood[i], reached.push_back(int32_t(i));
    if (reached.empty()) {
        shore.minX = shore.minZ = 0, shore.maxX = shore.maxZ = -1;
        return false;
    }
    int minX = size, maxX = 0, minZ = size, maxZ = 0;
    for (size_t next = 0; next < reached.size(); ++next) {
        const int32_t cell = reached[next];
        const int x = cell % size, z = cell / size;
        minX = std::min(minX, x), maxX = std::max(maxX, x), minZ = std::min(minZ, z), maxZ = std::max(maxZ, z);
        const float water = shore.still[size_t(cell)], swing = swingOf(domain, water);
        if (water + swing + HighestBerm <= shore.top[size_t(cell)]) continue;   // struck, never crossed
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            if (shore.still[other] != NoWater) continue;
            // Dry ground deeper under the water than its low stand lies beyond the water's sill,
            // down the river it spills into: the water does not stand there.
            if (shore.stood[other] == NoWater && shore.top[other] < water - swing) continue;
            shore.still[other] = water;
            reached.push_back(int32_t(other));
        }
    }
    shore.minX = std::max(0, minX - 1), shore.maxX = std::min(size - 1, maxX + 1);
    shore.minZ = std::max(0, minZ - 1), shore.maxZ = std::min(size - 1, maxZ + 1);
    return true;
}

namespace {

// Where running water keeps the bed open against the waves: a river the coarser level draws
// (landscape/fn/Process.hpp: the water a cell gathers beyond the sheet of its slope, against that
// level's channel head; a Site keeps such rivers on their course, Fluvial.cpp) on dry ground; the
// sill a lake spills over and the channel below it (dry ground the flood left at or below the
// lake's level that passes a channel of the level), whatever the size of the lake's basin; and
// from a sill, the path its water takes to it under the lake within `ApproachReach`, where the
// waves would bar the approach and the lake would rise with it. Only a lake's outlet raises its
// water when barred: a river mouth barred by the waves raises no sea, and behind such a bar a
// lagoon is the right outcome. The path is followed while it carries half of what the sill
// passes, which keeps to the outlet's main branch; a heuristic, not a law. The level's own
// channels elsewhere are rills a cell or two wide that the swash buries.
// `gathered` is filled by routeWater at every level the coast works (Replay.cpp).
void findStreams(const Domain& domain, Shore& shore, const FlowScratch& scratch) {
    std::fill(shore.stream.begin(), shore.stream.end(), uint8_t(0));
    if (scratch.gathered.size() != domain.cells()) return;
    const Level level = domain.key.level;
    const float river = channelHeadOf(level == Level::Region ? level : Level(uint8_t(level) - 1)), channel = channelHeadOf(level);
    std::vector<int32_t> open;
    std::vector<float> least;   // m³/yr the path to each open cell must carry
    std::vector<int> away;      // cells from the sill
    for (size_t i = 0; i < domain.cells(); ++i) {
        const float still = shore.still[i], gathered = scratch.gathered[i];
        const bool dry = shore.stood[i] == NoWater;
        const bool sill = dry && still != NoWater && swingOf(domain, still) == LakeSwing && domain.surface[i] <= still + Film && gathered >= channel;
        shore.stream[i] = sill || (dry && gathered >= river);
        if (sill) open.push_back(int32_t(i)), least.push_back(0.5f * gathered), away.push_back(0);
    }
    // A path one cell wide moves as the bed changes: the approach is bounded by distance, not depth.
    const int reach = std::max(1, int(ApproachReach / domain.cell));
    for (size_t next = 0; next < open.size(); ++next) {
        if (away[next] >= reach) continue;
        const int x = open[next] % domain.size, z = open[next] / domain.size;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= domain.size || nz >= domain.size) continue;
            const size_t other = domain.index(nx, nz);
            if (shore.stream[other] || shore.stood[other] == NoWater || scratch.gathered[other] < least[next]) continue;
            shore.stream[other] = 1;
            open.push_back(int32_t(other)), least.push_back(least[next]), away.push_back(away[next] + 1);
        }
    }
}

// A stream that runs out of standing water or into it keeps its channel at the level the water
// spills at: the sand and gravel the waves laid on it above that this step wait in its surf
// instead, and the mud goes with the current. Without it a lake's outlet is barred a little
// higher every step and the lake rises with it.
void keepChannels(Domain& domain, Shore& shore) {
    for (int z = shore.minZ; z <= shore.maxZ; ++z)
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            const float water = shore.still[i];
            if (water == NoWater || domain.hold[i] != Free || !shore.stream[i]) continue;
            const float loose = looseAt(domain, i), laid = std::min({shore.top[i] - water, loose - shore.before[i], loose});
            if (laid <= 0) continue;
            for (const int kind : {Fines, Sand, Coarse}) {
                const float share = laid * domain.ground.sediment[kind][i] / loose;
                domain.ground.sediment[kind][i] -= share;
                if (kind == Fines) shore.ledger.left += double(share) * domain.cell * domain.cell;
                else shore.pending[kind][i] += share;
            }
            shore.top[i] -= laid;
        }
}

} // namespace

void workShores(Domain& domain, FlowScratch& scratch) {
    if (domain.key.level == Level::Ground) return;
    const bool last = domain.time + 1.5 * domain.step > domain.endTime;
    if (!domain.shore) {
        // Most ground stands dry: nothing is set up until water is found.
        bool any = false;
        for (size_t i = 0; i < domain.cells() && !any; ++i) any = standsOver(domain, i, domain.ground.rock[i] + looseAt(domain, i));
        if (!any) return;
    }
    Shore& shore = shoreOf(domain);
    readStanding(domain, shore);
    const bool water = spreadWater(domain, shore);
    // With the water gone a shore has nothing to do but lay down, in the last step, what still waits in its surf.
    if (!water && !last) return;
    for (size_t i = 0; i < domain.cells(); ++i) shore.before[i] = looseAt(domain, i);
    const std::vector<float> cutBefore = shore.cut;
    const TileLedger ledgerBefore = domain.ledger;
    const double waitingBefore = pendingOwned(domain, shore);
    if (water) {
        const bool dated = domain.key.level == Level::Region;
        const double years = dated ? std::min(domain.step, StandTime) : StandTime * domain.step / std::max(domain.step, domain.endTime - domain.startTime);
        const int rounds = std::clamp(int(std::ceil(years / SweepYears - 1e-9)), 1, MostRounds);
        raiseWaves(domain, shore);
        findStreams(domain, shore, scratch);
        cutCliffs(domain, shore, years);
        for (int round = 0; round < rounds; ++round) {
            // The waves of each round run over the ground the last one left: a bank cut back lets them further in.
            if (round > 0) raiseWaves(domain, shore);
            findSurf(domain, shore);
            moveSand(domain, shore, round, years / rounds);
        }
        keepChannels(domain, shore);
    }
    // Broken rock still in the surf when the replay ends is laid where it waits, up to high water
    // (all of it where the water has gone); the rest has gone beyond the reach of this domain's waves.
    if (last) {
        const double area = domain.cell * domain.cell;
        for (size_t i = 0; i < domain.cells(); ++i)
            for (const int kind : {Sand, Coarse}) {
                float& waiting = shore.pending[kind][i];
                if (waiting <= 0) continue;
                const float laid = shore.still[i] == NoWater ? waiting : std::clamp(layingLevel(domain, shore, i, shore.still[i]) - shore.top[i], 0.0f, waiting);
                domain.ground.sediment[kind][i] += laid, shore.top[i] += laid;
                shore.ledger.left += double(waiting - laid) * area;
                waiting = 0;
            }
    }
    book(domain, shore, cutBefore);
    // Net of the step on the tile's own cells: what the waves took, less what they laid, what
    // dissolved and what the surf there holds more. The rest crossed the tile's edge, out or in:
    // drift, mud and what the last step leaves in the surf; what went out and came back cancels.
    const double crossed = (domain.ledger.eroded - ledgerBefore.eroded) - (domain.ledger.deposited - ledgerBefore.deposited) -
                           (domain.ledger.dissolved - ledgerBefore.dissolved) - (pendingOwned(domain, shore) - waitingBefore);
    (crossed > 0 ? domain.ledger.sedimentOut : domain.ledger.sedimentIn) += std::abs(crossed);
}

float layingLevel(const Domain& domain, const Shore& shore, size_t i, float water) { return shore.stream[i] ? water : water + swingOf(domain, water); }

} // namespace derivean::sim
