#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

// Rise over run each class holds in the swash. Below the waterline a bed holds less the deeper it
// lies, as SwashSlope/√(1 + depth/ProfileDepth): the concave shoreface of an equilibrium profile.
constexpr float SwashSlope[SedimentClasses] = {0.27f, 0.08f, 0.004f};
constexpr float ProfileDepth = 0.5f;
constexpr float Closure = 2.5f;      // depth over wave height beyond which a bed is out of the waves' reach
constexpr float Breaking = 2.0f;     // depth over wave height at which they move it most
constexpr float GravelLift = 0.25f;  // how readily waves lift gravel against sand
// m of sand a train of one metre carries through a cell in one sweep, and m of bed a sweep can
// lift. A sweep stands for a few years of a finer level's shore and a few centuries of a region's:
// the drift of a metre-high sea, some 10^5 m³ a year, is then tens of metres over a cell of either,
// and far more than a four-metre cell's own bed can supply, so a site's sand goes wherever its
// waves let it lie.
constexpr float Carry[LevelCount] = {0, 16.0f, 30.0f, 20.0f, 0};
constexpr float Stir[LevelCount] = {0, 2.0f, 1.0f, 0.25f, 0};

float holds(int kind, float depth) { return SwashSlope[kind] / std::sqrt(1.0f + std::max(0.0f, depth) / ProfileDepth); }

struct Sweep {
    Domain& domain;
    Shore& shore;
    int train;
    float run, carry, stir;
    double area;

    // A cell the train crosses: the water covers it, or the train's swash does.
    bool covered(size_t i) const { return shore.wave[i * WaveDirections + size_t(train)] > 0; }

    void hand(size_t to, int kind, float amount) {
        shore.load[kind][to] += amount;
        shore.surf[to] |= 2;
    }

    void strandLost(size_t i, const float moving[SedimentClasses]) {
        for (const int kind : {Coarse, Sand})
            if (moving[kind] > 0) shore.pending[kind][i] += moving[kind];
    }

    void lay(size_t i, int kind, float amount) {
        domain.ground.sediment[kind][i] += amount;
        shore.top[i] += amount;
    }

    // Sand and gravel that waves cannot take on from a cell: laid on it up to high water, the
    // rest left in its surf.
    void strand(size_t i, int kind, float amount) {
        const float water = shore.still[i], laid = std::clamp(water + swingOf(domain, water) - shore.top[i], 0.0f, amount);
        lay(i, kind, laid);
        shore.pending[kind][i] += amount - laid;
    }

    // Loose ground ahead that stands steeper over the water than its classes hold is undercut:
    // what comes down joins what the train carries. A bluff of sand feeds a shore this way.
    void undercut(size_t i, size_t ahead, float reach, float moving[SedimentClasses]) {
        if (domain.hold[ahead] != Free) return;
        // What waits in the surf at the foot of the shore is taken along first.
        for (const int kind : {Sand, Coarse}) {
            const float taken = std::min(shore.pending[kind][ahead], reach);
            shore.pending[kind][ahead] -= taken, moving[kind] += taken;
        }
        for (const int kind : {Fines, Sand, Coarse}) {
            const float over = shore.top[ahead] - shore.top[i] - SwashSlope[kind] * run;
            const float fallen = std::min({domain.ground.sediment[kind][ahead], 0.5f * over, reach});
            if (fallen <= 0) continue;
            lay(ahead, kind, -fallen);
            if (kind == Fines) lay(i, Fines, fallen);
            else moving[kind] += fallen;
        }
    }

    // The swash runs up the land ahead of the last cell the water covers and leaves what it
    // carries: sand as high as sand lies, gravel above it, neither above the run-up of the waves.
    void swash(size_t i, float height, float moving[SedimentClasses]) {
        const float water = shore.still[i], limit = water + swingOf(domain, water) + std::min(HighestBerm, Runup * height);
        const int reach = std::clamp(int(HighestBerm / (SwashSlope[Sand] * run)) + 1, 1, 12);
        int x = int(i % size_t(domain.size)), z = int(i / size_t(domain.size));
        float below = shore.top[i];
        for (int step = 0; step < reach && moving[Coarse] + moving[Sand] > 0; ++step) {
            x += NeighborX[train], z += NeighborZ[train];
            if (x < 1 || z < 1 || x >= domain.size - 1 || z >= domain.size - 1) return;
            const size_t n = domain.index(x, z);
            if (domain.hold[n] != Free || shore.top[n] >= limit) return;
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
        if (!covered(i)) {
            // Nothing is handed to a cell the train does not cross; kept for the books all the same.
            strandLost(i, moving);
            return;
        }
        const size_t at = i * WaveDirections + size_t(train);
        const float water = shore.still[i], depth = water - shore.top[i], height = std::sqrt(shore.arrive[at]);
        // A cell is as shallow as its bed gets on the way to the cell ahead: the surf of a shore
        // lies inside the last cell of water, however deep that cell is on average.
        const float shoal = std::min(depth, water - 0.5f * (shore.top[i] + shore.top[size_t(ptrdiff_t(i) + ptrdiff_t(NeighborZ[train]) * domain.size + NeighborX[train])]));
        const float within = height > 1e-3f ? std::clamp((Closure * height - shoal) / ((Closure - Breaking) * height), 0.0f, 1.0f) : 0.0f;
        const float capacity = within > 0 ? carry * std::pow(shore.wave[at], 1.25f) * within : 0.0f;
        if (domain.hold[i] != Free) {
            // A cell that does not evolve takes what reaches it and hands on what its own surf would carry.
            shore.ledger.left += double(moving[Coarse] + moving[Sand]) * area;
            const float loose = domain.ground.sediment[Coarse][i] + domain.ground.sediment[Sand][i], given = loose > 0 ? std::min(capacity, stir * within) : 0.0f;
            moving[Coarse] = loose > 0 ? given * domain.ground.sediment[Coarse][i] / loose : 0.0f;
            moving[Sand] = given - moving[Coarse];
            shore.ledger.entered += double(given) * area;
            // The outermost ring has no cell ahead of it to hand to but the one straight on.
            const int nx = x + NeighborX[train], nz = z + NeighborZ[train];
            if (x == 0 || z == 0 || x == domain.size - 1 || z == domain.size - 1) {
                const bool inward = nx > 0 && nz > 0 && nx < domain.size - 1 && nz < domain.size - 1 && covered(domain.index(nx, nz));
                if (inward) hand(domain.index(nx, nz), Coarse, moving[Coarse]), hand(domain.index(nx, nz), Sand, moving[Sand]);
                else shore.ledger.entered -= double(given) * area;
                return;
            }
        } else {
            work(i, depth, within, capacity, moving);
        }
        pass(i, depth, height, moving);
    }

    void work(size_t i, float depth, float within, float capacity, float moving[SedimentClasses]) {
        const float water = shore.still[i];
        const float total = moving[Coarse] + moving[Sand];
        if (total > capacity) {
            // Waves that can carry less than they bring drop the difference, gravel first, up to high water.
            float excess = std::min(total - capacity, std::max(0.0f, water + swingOf(domain, water) - shore.top[i]));
            for (const int kind : {Coarse, Sand}) {
                const float laid = std::min(moving[kind], excess);
                lay(i, kind, laid);
                moving[kind] -= laid, excess -= laid;
            }
        } else {
            // Broken rock in the surf goes first; of the bed a sweep lifts only so much.
            float want = capacity - total;
            for (const int kind : {Sand, Coarse}) {
                const float taken = std::min(shore.pending[kind][i], want);
                shore.pending[kind][i] -= taken, moving[kind] += taken, want -= taken;
            }
            want = std::min(want, stir * within);
            const float sand = domain.ground.sediment[Sand][i], gravel = GravelLift * domain.ground.sediment[Coarse][i];
            if (want > 0 && sand + gravel > 0) {
                const float share = std::min(1.0f, want / (sand + gravel));
                lay(i, Sand, -share * sand), moving[Sand] += share * sand;
                lay(i, Coarse, -share * gravel), moving[Coarse] += share * gravel;
            }
        }
        // Undertow: a class that lies steeper than it holds towards the cell the train came from
        // slides back to it. Mud holds almost no slope, so it leaves for water too deep to stir.
        if (within <= 0) return;
        const size_t back = size_t(ptrdiff_t(i) - (ptrdiff_t(NeighborZ[train]) * domain.size + NeighborX[train]));
        if (!covered(back)) return;
        for (const int kind : {Fines, Sand, Coarse}) {
            const float over = shore.top[i] - shore.top[back] - holds(kind, depth) * run;
            const float slid = std::min({domain.ground.sediment[kind][i], 0.5f * over, stir * within});
            if (slid <= 0) continue;
            lay(i, kind, -slid);
            if (domain.hold[back] == Free) lay(back, kind, slid);
            else shore.ledger.left += double(slid) * area;
        }
    }

    // Onward with the train. A class goes straight on while the bed ahead does not rise more
    // steeply than it holds; at the shore the swash takes it. What cannot go on is turned along
    // the shore, to the cells half a right angle to either side: the drift of oblique waves.
    void pass(size_t i, float depth, float height, float moving[SedimentClasses]) {
        const auto ahead = [&](int k) { return size_t(ptrdiff_t(i) + ptrdiff_t(NeighborZ[k]) * domain.size + NeighborX[k]); };
        const auto open = [&](size_t n, int k, int kind) { return covered(n) && shore.top[n] - shore.top[i] <= holds(kind, depth) * float(domain.cell) * NeighborReach[k]; };
        const size_t straight = ahead(train);
        if (!covered(straight)) {
            undercut(i, straight, std::min(stir, carry * std::pow(shore.arrive[i * WaveDirections + size_t(train)], 1.25f)), moving);
            swash(i, height, moving);
        }
        const int sides[2] = {(train + 1) & 7, (train + 7) & 7};
        for (const int kind : {Coarse, Sand}) {
            if (moving[kind] <= 0) continue;
            if (open(straight, train, kind)) {
                hand(straight, kind, moving[kind]);
                continue;
            }
            const bool first = open(ahead(sides[0]), sides[0], kind), second = open(ahead(sides[1]), sides[1], kind);
            if (first) hand(ahead(sides[0]), kind, second ? 0.5f * moving[kind] : moving[kind]);
            if (second) hand(ahead(sides[1]), kind, first ? 0.5f * moving[kind] : moving[kind]);
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
