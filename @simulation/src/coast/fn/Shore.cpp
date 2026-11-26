#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

// Passes of the eight trains over the sand and gravel of a shore per step.
constexpr int Rounds[LevelCount] = {0, 3, 4, 4, 0};
constexpr float RateWeight = 0.4f;

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
        for (int k = 0; k < WaveDirections; ++k) shore.gale[i * WaveDirections + size_t(k)] = stormWind(domain, i, k) * stormWind(domain, i, k);
    for (auto* field : {&shore.top, &shore.notch, &shore.cut, &shore.laid, &shore.before, &shore.pending[0], &shore.pending[1], &shore.pending[2], &shore.load[0],
                        &shore.load[1], &shore.load[2]})
        field->assign(cells, 0.0f);
    shore.stood.assign(cells, NoWater);
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

void workShores(Domain& domain, FlowScratch&) {
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
    if (water) {
        const int rounds = Rounds[size_t(domain.key.level)];
        const bool dated = domain.key.level == Level::Region;
        const double years = dated ? std::min(domain.step, StandTime) : StandTime * domain.step / std::max(domain.step, domain.endTime - domain.startTime);
        raiseWaves(domain, shore);
        cutCliffs(domain, shore, years);
        findSurf(domain, shore);
        for (int round = 0; round < rounds; ++round) moveSand(domain, shore, round);
    }
    // Broken rock still in the surf when the replay ends is laid where it waits, up to high water
    // (all of it where the water has gone); the rest has gone beyond the reach of this domain's waves.
    if (last) {
        const double area = domain.cell * domain.cell;
        for (size_t i = 0; i < domain.cells(); ++i)
            for (const int kind : {Sand, Coarse}) {
                float& waiting = shore.pending[kind][i];
                if (waiting <= 0) continue;
                const float laid = shore.still[i] == NoWater ? waiting : std::clamp(shore.still[i] + swingOf(domain, shore.still[i]) - shore.top[i], 0.0f, waiting);
                domain.ground.sediment[kind][i] += laid, shore.top[i] += laid;
                shore.ledger.left += double(waiting - laid) * area;
                if (domain.owned(int(i % size_t(domain.size)), int(i / size_t(domain.size)))) domain.ledger.sedimentOut += double(waiting - laid) * area;
                waiting = 0;
            }
    }
    book(domain, shore, cutBefore);
}

} // namespace derivean::sim
