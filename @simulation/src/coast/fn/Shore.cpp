#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

// Rounds of wave work per step. Every round raises the waves anew over what the round before left.
constexpr int Rounds[LevelCount] = {0, 3, 4, 4, 0};

float looseAt(const Domain& domain, size_t i) { return domain.ground.sediment[0][i] + domain.ground.sediment[1][i] + domain.ground.sediment[2][i]; }

// What the step's flood left standing: the sea, and lakes their rivers still reach.
bool readStanding(const Domain& domain, Shore& shore) {
    bool any = false;
    const float sea = float(domain.seaLevel) + SeaBand;
    for (size_t i = 0; i < domain.cells(); ++i) {
        shore.top[i] = domain.ground.rock[i] + looseAt(domain, i);
        const bool stands = domain.level[i] > shore.top[i] + 1e-3f && (domain.level[i] <= sea || domain.discharge[i] > 0);
        shore.stood[i] = stands ? domain.level[i] : NoWater;
        any = any || stands;
    }
    return any;
}

// Entries in the tile's ledger and its per-cell totals for what the waves did this step, and the
// surface a finer level is held to moved by the same: a bench or a spit is relief of the level's
// own, which its parent cannot have and its found uplift must not take back.
void book(Domain& domain, Shore& shore, const std::vector<float>& cutBefore) {
    const float dt = float(domain.step);
    const double area = domain.cell * domain.cell;
    for (int z = shore.minZ; z <= shore.maxZ; ++z) {
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free) continue;
            const float broken = shore.cut[i] - cutBefore[i], gained = looseAt(domain, i) - shore.before[i];
            if (broken == 0 && gained == 0) continue;
            const float lost = broken + std::max(0.0f, -gained), won = std::max(0.0f, gained);
            shore.laid[i] += gained;
            domain.trend[i] += gained - broken;
            domain.eroded[i] += lost;
            domain.deposited[i] += won;
            domain.erosionRate[i] += 0.4f * lost / dt;
            domain.depositionRate[i] += 0.4f * won / dt;
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
    shore.gale.resize(cells * WaveDirections);
    for (size_t i = 0; i < cells; ++i)
        for (int k = 0; k < WaveDirections; ++k) shore.gale[i * WaveDirections + size_t(k)] = stormWind(domain, i, k) * stormWind(domain, i, k);
    for (auto* field : {&shore.top, &shore.notch, &shore.cut, &shore.laid, &shore.before, &shore.rate, &shore.pending[0], &shore.pending[1], &shore.pending[2], &shore.load[0],
                        &shore.load[1], &shore.load[2]})
        field->assign(cells, 0.0f);
    shore.stood.assign(cells, NoWater);
    shore.still.assign(cells, NoWater);
    return shore;
}

// Still water reaches every cell it covers at high water, however far from where it stands at its
// mean level: across a bench that dries at low tide, behind a bar the tide overtops. The land
// cells next to it are given its level too: they are what its waves strike.
bool spreadWater(const Domain& domain, Shore& shore) {
    const int size = domain.size;
    std::vector<int32_t> reached;
    std::fill(shore.still.begin(), shore.still.end(), NoWater);
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
        const float water = shore.still[size_t(cell)];
        if (water + swingOf(domain, water) <= shore.top[size_t(cell)]) continue;   // land: struck, not crossed
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            if (shore.still[other] != NoWater) continue;
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
    if (!domain.shore) {
        // Most ground stands dry: nothing is set up until water is found.
        bool any = false;
        const float sea = float(domain.seaLevel) + SeaBand;
        for (size_t i = 0; i < domain.cells() && !any; ++i)
            any = domain.level[i] > domain.ground.rock[i] + looseAt(domain, i) + 1e-3f && (domain.level[i] <= sea || domain.discharge[i] > 0);
        if (!any) return;
    }
    Shore& shore = shoreOf(domain);
    if (!readStanding(domain, shore) || !spreadWater(domain, shore)) return;
    for (size_t i = 0; i < domain.cells(); ++i) shore.before[i] = looseAt(domain, i);
    const std::vector<float> cutBefore = shore.cut;
    const int rounds = Rounds[size_t(domain.key.level)];
    double years = std::min(domain.step, StandTime);
    for (int round = 0; round < rounds; ++round) {
        if (round > 0) spreadWater(domain, shore);
        raiseWaves(domain, shore);
        years -= cutCliffs(domain, shore, years / (rounds - round));
        moveSand(domain, shore, round);
    }
    // Broken rock still in the surf when the replay ends is laid where it waits.
    if (domain.time + 1.5 * domain.step > domain.endTime)
        for (size_t i = 0; i < domain.cells(); ++i)
            for (int c = 0; c < SedimentClasses; ++c)
                if (shore.pending[c][i] > 0 && domain.hold[i] == Free) domain.ground.sediment[c][i] += shore.pending[c][i], shore.pending[c][i] = 0;
    book(domain, shore, cutBefore);
}

} // namespace derivean::sim
