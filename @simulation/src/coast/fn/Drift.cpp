// About 255 lines, one owner: the pass of a wave train over loose bed (lift, drop, undertow, drift
// along the shore, swash). Its parts share the sweep's state and are not used apart.
#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

// Rise over run each class holds in the swash. Below the waterline a bed holds less the deeper it
// lies, as SwashSlope/√(1 + depth/ProfileDepth): the concave shoreface of an equilibrium profile.
constexpr float SwashSlope[SedimentClasses] = {0.27f, 0.08f, 0.004f};
constexpr float ProfileDepth = 0.5f;
constexpr float Closure = 2.5f;      // depth over wave height beyond which a bed is out of the waves' reach
constexpr float Breaking = 2.0f;     // depth over wave height at which they move it most
constexpr float GravelLift = 0.25f;  // how readily waves lift gravel against sand from a bed under water
// Share of what a train can carry that it lifts from one cell's bed: a strong sea fills up over
// ten cells of sand and so does a weak one, so the drift of a shore follows the stronger of them.
constexpr float Lift = 0.1f;
// m of sand a train of one metre carries through a cell in one sweep, and the most m of bed any
// sweep lifts. With Lift this makes a drift fill up over some ten cells of sandy shore, so that
// beyond them a shore gains and loses only where its waves change. It is about a tenth of what
// such a sea moves in the years a sweep stands for: spits here grow slowly.
constexpr float Carry[LevelCount] = {0, 8.0f, 5.0f, 3.0f, 0};
constexpr float Stir[LevelCount] = {0, 3.0f, 1.5f, 0.3f, 0};
constexpr size_t Outside = ~size_t(0);
constexpr float ActiveBed = 1.0f;    // m of loose bed from which a shore beyond the domain counts as sandy

float holds(int kind, float depth) { return SwashSlope[kind] / std::sqrt(1.0f + std::max(0.0f, depth) / ProfileDepth); }

// One train's pass over the shore, upwind cells first: each cell takes what was handed to it,
// drops or lifts by what its waves can carry, and hands the rest on.
struct Sweep {
    Domain& domain;
    Shore& shore;
    int train;
    float run, carry, stir;
    double area;

    // A cell the train crosses: the water covers it, or the train's swash does.
    bool covered(size_t i) const { return i != Outside && shore.wave[i * WaveDirections + size_t(train)] > 0; }
    size_t toward(size_t i, int k) const {
        const int x = int(i % size_t(domain.size)) + NeighborX[k], z = int(i / size_t(domain.size)) + NeighborZ[k];
        return x < 0 || z < 0 || x >= domain.size || z >= domain.size ? Outside : domain.index(x, z);
    }

    void hand(size_t to, int kind, float amount) {
        shore.load[kind][to] += amount;
        shore.surf[to] |= 2;
    }

    void lay(size_t i, int kind, float amount) {
        domain.ground.sediment[kind][i] += amount;
        shore.top[i] += amount;
    }

    // Sand and gravel that waves cannot take on from a cell: laid on it up to high water, the
    // rest left in its surf. A cell that does not evolve gives it back to where it came from.
    void strand(size_t i, int kind, float amount) {
        if (domain.hold[i] != Free) {
            shore.ledger.entered -= double(amount) * area;
            return;
        }
        const float water = shore.still[i], laid = std::clamp(water + swingOf(domain, water) - shore.top[i], 0.0f, amount);
        lay(i, kind, laid);
        shore.pending[kind][i] += amount - laid;
    }

    // Loose ground ahead that stands steeper over the water than its classes hold is undercut:
    // what comes down joins what the train carries, with what waited in the surf at its foot.
    // A bluff of sand feeds a shore this way.
    void undercut(size_t i, size_t ahead, float reach, float moving[SedimentClasses]) {
        if (ahead == Outside || domain.hold[ahead] != Free || reach <= 0) return;
        for (const int kind : {Sand, Coarse}) {
            const float taken = std::min(shore.pending[kind][ahead], reach);
            shore.pending[kind][ahead] -= taken, moving[kind] += taken;
        }
        for (const int kind : {Fines, Sand, Coarse}) {
            const float over = shore.top[ahead] - shore.top[i] - SwashSlope[kind] * run;
            const float fallen = std::min({domain.ground.sediment[kind][ahead], 0.5f * over, reach});
            if (fallen <= 0) continue;
            lay(ahead, kind, -fallen);
            if (kind != Fines) moving[kind] += fallen;
            else if (domain.hold[i] == Free) lay(i, Fines, fallen);
            else shore.ledger.left += double(fallen) * area;
        }
    }

    // The swash runs up the land ahead of the last cell the train crosses and leaves what it
    // carries: sand as high as sand lies, gravel above it, neither above the run-up of the waves.
    void swash(size_t i, float height, float moving[SedimentClasses]) {
        const float water = shore.still[i], limit = water + swingOf(domain, water) + std::min(HighestBerm, Runup * height);
        const int reach = std::clamp(int(HighestBerm / (SwashSlope[Sand] * run)) + 1, 1, 12);
        float below = shore.top[i];
        size_t n = i;
        for (int step = 0; step < reach && moving[Coarse] + moving[Sand] > 0; ++step) {
            n = toward(n, train);
            if (n == Outside || domain.hold[n] != Free || shore.top[n] >= limit) return;
            for (const int kind : {Sand, Coarse}) {
                const float laid = std::min(moving[kind], std::min(limit, below + SwashSlope[kind] * run) - shore.top[n]);
                if (laid <= 0) continue;
                lay(n, kind, laid);
                moving[kind] -= laid;
            }
            if (shore.top[n] > below + SwashSlope[Coarse] * run) return;
            below = shore.top[n];
        }
    }

    void visit(int x, int z) {
        const size_t i = domain.index(x, z);
        // Most water is too deep for any wave to touch the bed, and nothing is on its way through it.
        if (!shore.surf[i]) return;
        float moving[SedimentClasses] = {shore.load[Coarse][i], shore.load[Sand][i], 0};
        shore.load[Coarse][i] = shore.load[Sand][i] = 0;
        shore.surf[i] &= 1;
        if (!covered(i)) return;
        const size_t at = i * WaveDirections + size_t(train), straight = toward(i, train);
        const float water = shore.still[i], depth = water - shore.top[i], height = std::sqrt(shore.arrive[at]);
        // A cell is as shallow as its bed gets on the way to the cell ahead: the surf of a shore
        // lies inside the last cell of water, however deep that cell is on average.
        const float shoal = straight == Outside ? depth : std::min(depth, water - 0.5f * (shore.top[i] + shore.top[straight]));
        const float within = height > 1e-3f ? std::clamp((Closure * height - shoal) / ((Closure - Breaking) * height), 0.0f, 1.0f) : 0.0f;
        const float capacity = within > 0 ? carry * std::pow(shore.wave[at], 1.25f) * within : 0.0f;
        if (domain.hold[i] != Free) {
            // A cell that does not evolve takes what reaches it and hands on what its surf carries
            // over its own bed: the shore goes on beyond the domain as it is there. A sandy shore
            // beyond gives all its waves can carry; a film of sand on rock gives in proportion.
            shore.ledger.left += double(moving[Coarse] + moving[Sand]) * area;
            const float loose = domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i], given = capacity * std::min(1.0f, loose / ActiveBed);
            moving[Coarse] = loose > 0 ? given * domain.ground.sediment[Coarse][i] / loose : 0.0f;
            moving[Sand] = given - moving[Coarse];
            shore.ledger.entered += double(given) * area;
        } else {
            work(i, straight, depth, capacity, moving);
        }
        pass(i, straight, depth, height, capacity, moving);
    }

    void work(size_t i, size_t straight, float depth, float capacity, float moving[SedimentClasses]) {
        const float water = shore.still[i], total = moving[Coarse] + moving[Sand];
        if (total > capacity) {
            // Waves that can carry less than they bring drop the difference, gravel first: on the
            // bed up to high water, and what has no room there waits in the surf. Into water out of
            // their reach it goes as a tongue, half ahead and a quarter to either side.
            float excess = total - capacity, room = std::max(0.0f, water + swingOf(domain, water) - shore.top[i]);
            const size_t beside[2] = {toward(i, (train + 2) & 7), toward(i, (train + 6) & 7)};
            for (const int kind : {Coarse, Sand}) {
                float dropped = std::min(moving[kind], excess);
                moving[kind] -= dropped, excess -= dropped;
                for (const size_t side : beside) {
                    if (capacity > 0 || side == Outside || domain.hold[side] != Free || !covered(side) || shore.top[side] > shore.top[i]) continue;
                    const float aside = std::min(0.25f * dropped, std::max(0.0f, shore.still[side] + swingOf(domain, shore.still[side]) - shore.top[side]));
                    lay(side, kind, aside);
                    dropped -= aside;
                }
                const float laid = std::min(dropped, room);
                lay(i, kind, laid);
                shore.pending[kind][i] += dropped - laid;
                room -= laid;
            }
        } else {
            // Broken rock in the surf goes first; of the bed a sweep lifts only so much.
            float want = capacity - total;
            for (const int kind : {Sand, Coarse}) {
                const float taken = std::min(shore.pending[kind][i], want);
                shore.pending[kind][i] -= taken, moving[kind] += taken, want -= taken;
            }
            want = std::min({want, Lift * capacity, stir});
            // A bed that already lies deeper under the cell ahead than sand holds is not dug
            // further: the waves there work on the shore above it, which then feeds it.
            const bool deep = straight != Outside && shore.top[straight] - shore.top[i] > holds(Sand, depth) * run;
            // Where the train ends on a shore its swash throws gravel up as readily as sand; only the sand comes back down.
            const float sand = domain.ground.sediment[Sand][i], gravel = (covered(straight) ? GravelLift : 1.0f) * domain.ground.sediment[Coarse][i];
            if (want > 0 && sand + gravel > 0 && !deep) {
                const float share = std::min(1.0f, want / (sand + gravel));
                lay(i, Sand, -share * sand), moving[Sand] += share * sand;
                lay(i, Coarse, -share * gravel), moving[Coarse] += share * gravel;
            }
        }
        // Undertow: a class that lies steeper than it holds towards the cell the train came from
        // slides back to it. Mud holds almost no slope, so it leaves for water too deep to stir.
        const size_t back = toward(i, (train + 4) & 7);
        if (capacity <= 0 || !covered(back)) return;
        for (const int kind : {Fines, Sand, Coarse}) {
            const float over = shore.top[i] - shore.top[back] - holds(kind, depth) * run;
            const float slid = std::min({domain.ground.sediment[kind][i], 0.5f * over, Lift * capacity, stir});
            if (slid <= 0) continue;
            lay(i, kind, -slid);
            if (domain.hold[back] == Free) lay(back, kind, slid);
            else shore.ledger.left += double(slid) * area;
        }
    }

    // Onward with the train. A class goes straight on while the bed ahead does not rise more
    // steeply than it holds; at the shore the swash takes it. What cannot go on is turned along
    // the shore, to the cells half a right angle to either side: the drift of oblique waves.
    void pass(size_t i, size_t straight, float depth, float height, float capacity, float moving[SedimentClasses]) {
        // Cells that do not evolve hand nothing to one another: what they give goes into the domain.
        const bool evolving = domain.hold[i] == Free;
        const auto open = [&](size_t n, int k, int kind) {
            return covered(n) && (evolving || domain.hold[n] == Free) && shore.top[n] - shore.top[i] <= holds(kind, depth) * float(domain.cell) * NeighborReach[k];
        };
        if (!covered(straight)) {
            // What the surf takes from the bluff is bounded like what it lifts from the bed: the
            // train never leaves a cell carrying more than it can.
            const float room = std::max(0.0f, capacity - moving[Coarse] - moving[Sand]);
            undercut(i, straight, std::min({stir, room, Lift * carry * std::pow(shore.arrive[i * WaveDirections + size_t(train)], 1.25f)}), moving);
            swash(i, height, moving);
        }
        const int sides[2] = {(train + 1) & 7, (train + 7) & 7};
        const size_t beside[2] = {toward(i, sides[0]), toward(i, sides[1])};
        for (const int kind : {Coarse, Sand}) {
            if (moving[kind] <= 0) continue;
            if (open(straight, train, kind)) {
                hand(straight, kind, moving[kind]);
                continue;
            }
            const bool first = open(beside[0], sides[0], kind), second = open(beside[1], sides[1], kind);
            if (first) hand(beside[0], kind, second ? 0.5f * moving[kind] : moving[kind]);
            if (second) hand(beside[1], kind, first ? 0.5f * moving[kind] : moving[kind]);
            if (!first && !second) strand(i, kind, moving[kind]);
        }
    }
};

} // namespace

void findSurf(const Domain& domain, Shore& shore) {
    for (int z = shore.minZ; z <= shore.maxZ; ++z) {
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            shore.surf[i] = 0;
            if (shore.still[i] == NoWater) continue;
            float highest = 0;
            for (int k = 0; k < WaveDirections; ++k) highest = std::max(highest, shore.arrive[i * WaveDirections + size_t(k)]);
            float shoal = shore.still[i] - shore.top[i];
            if (x > 0 && z > 0 && x < domain.size - 1 && z < domain.size - 1)
                for (int k = 0; k < 8; ++k) shoal = std::min(shoal, shore.still[i] - 0.5f * (shore.top[i] + shore.top[domain.index(x + NeighborX[k], z + NeighborZ[k])]));
            shore.surf[i] = highest > 0 && (shoal <= 0 || shoal * shoal < Closure * Closure * highest);
        }
    }
}

void moveSand(Domain& domain, Shore& shore, int round) {
    Sweep sweep{domain, shore, 0, 0, Carry[size_t(domain.key.level)], Stir[size_t(domain.key.level)], domain.cell * domain.cell};
    for (int order = 0; order < WaveDirections; ++order) {
        // The trains take turns to go first, so that none of them always works on what the others left.
        sweep.train = (order + 3 * round) & 7;
        sweep.run = float(domain.cell) * NeighborReach[sweep.train];
        sweepDownwind(shore, sweep.train, [&](int x, int z) { sweep.visit(x, z); });
    }
}

} // namespace derivean::sim
