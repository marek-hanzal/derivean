// About 330 lines, one owner: the pass of a wave train over loose bed (lift, drop, undertow, drift
// along the shore, swash, the bank it strikes). Its parts share the sweep's state and are not used apart.
#include "coast/fn/Coast.hpp"

#include "chronology/type/Calendar.hpp"

#include <utility>

namespace derivean::sim {
namespace {

constexpr float Closure = 2.5f;      // depth over wave height beyond which a bed is out of the waves' reach
constexpr float Breaking = 2.0f;     // depth over wave height at which they move it most
constexpr float GravelLift = 0.25f;  // how readily waves lift gravel against sand from a bed under water
// Share of what a train can carry that it lifts from one cell's bed: a strong sea fills up over
// ten cells of sand and so does a weak one, so the drift of a shore follows the stronger of them.
constexpr float Lift = 0.1f;
// The trains are the storm seas of each quarter. Their work grows as H^2.5, so with the wind as
// U^2.5 (fetch-limited) to U^5 (fully developed): with a Rayleigh wind and storms StormGain times
// the mean, a year moves what 6–16 % of it at the storm height would, and the wind blows from one
// of the eight quarters at a time. Each train works this share of the year.
constexpr float StormShare = 0.01f;
// m³ a year of sand a train breaking at a metre moves past a shore it strikes at half a right angle:
// CERC (USACE 1984), Q = K·√g/(16·√κ·(s − 1)·(1 − n))·H^2.5·sin 2θ, K = 0.39 for the significant
// height, κ = 0.78, s = 2.65, n = 0.4, sin 2θ = 1, i.e. 0.0873 m³/s at H = 1 m all year.
constexpr float DriftRate = 0.0873f * float(YearSeconds) * StormShare;
// m³ a year per metre of scarp the swash of a train takes from loose ground it runs `d` m up:
// 4·Cs·d²/T per wave (Larson, Erikson and Hanson 2004, dune erosion by wave impact), Cs = 1.7e-4.
constexpr float ImpactRate = 4.0f * 1.7e-4f * float(YearSeconds) * StormShare;
constexpr size_t Outside = ~size_t(0);
// m of ground cut down to the foot of a bank that the swash of a sweep crosses to reach it, and the
// most the bank goes back in a sweep. Past a sweep of about 40 years at the Site level this, not the
// rate of impact, bounds how far a bank of sand goes back.
constexpr float ScarpReach = 32.0f;

// One train's pass over the shore, upwind cells first: each cell takes what was handed to it,
// drops or lifts by what its waves can carry, and hands the rest on.
struct Sweep {
    Domain& domain;
    Shore& shore;
    int train;
    float run;
    float years;   // of the trains' work this sweep stands for
    double area;
    // The train at the cell being visited: its breaker height, and the beach face each class takes under it (Beach.cpp).
    float surge{}, face[SedimentClasses]{};

    // Rise over run a class holds `depth` m below still water (above it, the face).
    float holds(int kind, float depth) const { return profileSlope(kind, depth, surge, face[kind]); }

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
    // rest left in its surf. On a cell that does not evolve it is left to the shore beyond.
    void strand(size_t i, int kind, float amount) {
        if (domain.hold[i] != Free) {
            shore.ledger.left += double(amount) * area;
            return;
        }
        const float laid = std::clamp(layingLevel(domain, shore, i, shore.still[i]) - shore.top[i], 0.0f, amount);
        lay(i, kind, laid);
        shore.pending[kind][i] += amount - laid;
    }

    // m of bed over a cell a train of height √`energy` m carries through it in this sweep: the
    // drift such a train moves past the shore in the sweep's years, shared among the cells of the
    // surf zone of its breakers, out to where the shoreface lies at closure depth under them.
    float capacityOf(float energy) const {
        const float wide = std::max(float(domain.cell), shorefaceReach(Sand, Closure * surge));
        return DriftRate * std::pow(energy, 1.25f) * years / (wide * float(domain.cell));
    }

    // Loose ground ahead that stands over the reach of the swash is undercut at the rate of wave
    // impact: the swash runs up to its foot, from high water or from the beach before it where
    // that stands higher, and takes what stands above the foot; the bank above comes down with it
    // (Larson, Erikson and Hanson 2004). A cell taken down to the foot lets the swash on to the
    // next, with the time it has left, as far as ScarpReach. What comes down joins what the train
    // carries (at most `room`, after what waited in the surf at the foot); the rest waits in the
    // surf of the cell before the bank. A bluff of sand feeds a shore this way.
    void undercut(size_t i, size_t ahead, float room, float moving[SedimentClasses]) {
        if (ahead == Outside || domain.hold[ahead] != Free || domain.hold[i] != Free) return;
        for (const int kind : {Sand, Coarse}) {
            const float taken = std::min(shore.pending[kind][ahead], room);
            shore.pending[kind][ahead] -= taken, moving[kind] += taken, room -= taken;
        }
        if (surge <= 0) return;
        const float water = shore.still[i], high = water + swingOf(domain, water), reach = high + runUp(surge, face[Sand]);
        const float period = wavePeriod(surge);
        float left = 1;   // share of the sweep's time the swash still has
        size_t foot = i;
        const int cells = std::max(1, int(ScarpReach / float(domain.cell)));
        for (int cell = 0; cell < cells && left > 0; ++cell) {
            if (ahead == Outside || domain.hold[ahead] != Free) return;
            const float toe = std::max(shore.top[foot], high), struck = reach - toe, above = shore.top[ahead] - toe;
            if (struck <= 0) return;
            if (above <= 0) {
                // Ground an earlier sweep of this step took down to the foot: the swash crosses it
                // to the bank behind, as the waves of the next step will.
                foot = ahead, ahead = toward(ahead, train);
                continue;
            }
            const float loose = domain.ground.sediment[Coarse][ahead] + domain.ground.sediment[Sand][ahead] + domain.ground.sediment[Fines][ahead];
            if (loose <= 0) return;
            const float could = ImpactRate * struck * struck / period * years / float(domain.cell);   // m over the cell, the whole sweep
            const float taken = std::min({above, loose, left * could});
            const float share = taken / loose;
            for (const int kind : {Fines, Sand, Coarse}) {
                const float fallen = share * domain.ground.sediment[kind][ahead];
                if (fallen <= 0) continue;
                lay(ahead, kind, -fallen);
                if (kind == Fines) {
                    lay(i, Fines, fallen);
                    continue;
                }
                const float carried = std::clamp(room, 0.0f, fallen);
                moving[kind] += carried, room -= carried;
                shore.pending[kind][i] += fallen - carried;
            }
            left -= taken / could;
            if (taken < above) return;   // the bank stands, or what is under it is rock
            foot = ahead, ahead = toward(ahead, train);
        }
    }

    // The swash runs up the last cell the train crosses and the land ahead of it and leaves what
    // it carries, neither above the run-up of the waves: gravel first, which the backwash cannot
    // take back down, then sand as high as its flatter face lies. On the last cell this builds the
    // berm, on its face over the cell before it; on a mixed beach the berm is the gravel's.
    void swash(size_t i, float moving[SedimentClasses]) {
        // The swash runs up the face of what it mostly carries.
        const float water = shore.still[i], limit = water + swingOf(domain, water) + runUp(surge, face[moving[Coarse] > moving[Sand] ? Coarse : Sand]);
        const int reach = std::clamp(int(HighestBerm / (face[Sand] * run)) + 1, 1, 12);
        const size_t back = toward(i, (train + 4) & 7);
        if (domain.hold[i] == Free && back != Outside)
            for (const int kind : {Coarse, Sand}) {
                const float laid = std::min(moving[kind], std::min(shore.stream[i] ? water : limit, shore.top[back] + face[kind] * run) - shore.top[i]);
                if (laid <= 0) continue;
                lay(i, kind, laid);
                moving[kind] -= laid;
            }
        float below = shore.top[i];
        size_t n = i;
        for (int step = 0; step < reach && moving[Coarse] + moving[Sand] > 0; ++step) {
            n = toward(n, train);
            if (n == Outside || domain.hold[n] != Free || shore.top[n] >= limit) return;
            for (const int kind : {Coarse, Sand}) {
                const float laid = std::min(moving[kind], std::min(shore.stream[n] ? water : limit, below + face[kind] * run) - shore.top[n]);
                if (laid <= 0) continue;
                lay(n, kind, laid);
                moving[kind] -= laid;
            }
            if (shore.top[n] > below + face[Coarse] * run) return;
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
        surge = std::sqrt(shore.crest[at]);
        for (int kind = 0; kind < SedimentClasses; ++kind) face[kind] = faceSlope(kind, surge);
        // A cell is as shallow as its bed gets on the way to the cell ahead: the surf of a shore
        // lies inside the last cell of water, however deep that cell is on average.
        const float shoal = straight == Outside ? depth : std::min(depth, water - 0.5f * (shore.top[i] + shore.top[straight]));
        const float within = height > 1e-3f ? std::clamp((Closure * height - shoal) / ((Closure - Breaking) * height), 0.0f, 1.0f) : 0.0f;
        // Under water the surf zone carries the drift of the breakers (CERC's total, spread evenly
        // over it); over ground the swash crosses, what is left of the broken train.
        const float capacity = within > 0 ? capacityOf(depth > 0 ? shore.crest[at] : shore.wave[at]) * within : 0.0f;
        if (domain.hold[i] != Free) {
            // A cell that does not evolve passes on what its own shore would: the train lifts from
            // its bed what it lifts from any bed and drops what it cannot carry, and its ground
            // stays as it is. A load fills up across a strip of such cells as it does inside the domain.
            const float total = moving[Coarse] + moving[Sand];
            if (total > capacity) {
                const float dropped = total - capacity, gravel = std::min(moving[Coarse], dropped);
                moving[Coarse] -= gravel, moving[Sand] -= dropped - gravel;
                shore.ledger.left += double(dropped) * area;
            } else {
                const auto [sand, gravel] = bedLift(i, straight, depth, std::min(capacity - total, Lift * capacity));
                moving[Sand] += sand, moving[Coarse] += gravel;
                shore.ledger.entered += double(sand + gravel) * area;
            }
        } else {
            work(i, straight, depth, capacity, moving);
        }
        pass(i, straight, depth, height, capacity, moving);
    }

    // m of sand and gravel a sweep lifts from the bed of a cell, at most `want`. A bed is not dug
    // deeper under the cell ahead than sand holds: the waves there work on the shore above it,
    // which then feeds it. Where the train ends on a shore its swash throws gravel up as readily
    // as sand; only the sand comes back down.
    std::pair<float, float> bedLift(size_t i, size_t straight, float depth, float want) {
        if (straight != Outside) want = std::min(want, shore.top[i] - shore.top[straight] + holds(Sand, depth) * run);
        const float sand = domain.ground.sediment[Sand][i], gravel = (covered(straight) ? GravelLift : 1.0f) * domain.ground.sediment[Coarse][i];
        if (want <= 0 || sand + gravel <= 0) return {0.0f, 0.0f};
        const float share = std::min(1.0f, want / (sand + gravel));
        return {share * sand, share * gravel};
    }

    void work(size_t i, size_t straight, float depth, float capacity, float moving[SedimentClasses]) {
        const float water = shore.still[i], total = moving[Coarse] + moving[Sand];
        if (total > capacity) {
            // Waves that can carry less than they bring drop the difference, gravel first: on the
            // bed up to high water, and what has no room there waits in the surf. Into water out of
            // their reach it goes as a tongue, half ahead and a quarter to either side.
            float excess = total - capacity, room = std::max(0.0f, layingLevel(domain, shore, i, water) - shore.top[i]);
            const size_t beside[2] = {toward(i, (train + 2) & 7), toward(i, (train + 6) & 7)};
            for (const int kind : {Coarse, Sand}) {
                float dropped = std::min(moving[kind], excess);
                moving[kind] -= dropped, excess -= dropped;
                for (const size_t side : beside) {
                    if (capacity > 0 || side == Outside || domain.hold[side] != Free || !covered(side) || shore.top[side] > shore.top[i]) continue;
                    const float aside = std::min(0.25f * dropped, std::max(0.0f, layingLevel(domain, shore, side, shore.still[side]) - shore.top[side]));
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
            const auto [sand, gravel] = bedLift(i, straight, depth, std::min(want, Lift * capacity));
            if (sand + gravel > 0) {
                lay(i, Sand, -sand), moving[Sand] += sand;
                lay(i, Coarse, -gravel), moving[Coarse] += gravel;
            }
        }
        // Undertow: a class that lies steeper than it holds towards the cell the train came from
        // slides back to it. Mud holds almost no slope, so it leaves for water too deep to stir.
        const size_t back = toward(i, (train + 4) & 7);
        if (capacity <= 0 || !covered(back)) return;
        for (const int kind : {Fines, Sand, Coarse}) {
            const float over = shore.top[i] - shore.top[back] - holds(kind, depth) * run;
            const float slid = std::min({domain.ground.sediment[kind][i], 0.5f * over, Lift * capacity});
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
        const auto open = [&](size_t n, int k, int kind) { return covered(n) && shore.top[n] - shore.top[i] <= holds(kind, depth) * float(domain.cell) * NeighborReach[k]; };
        if (!covered(straight)) {
            // What the surf takes from the bluff is bounded like what it lifts from the bed: the
            // train never leaves a cell carrying more than it can.
            const float room = std::max(0.0f, capacity - moving[Coarse] - moving[Sand]);
            undercut(i, straight, room, moving);
            swash(i, moving);
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

void moveSand(Domain& domain, Shore& shore, int round, double years) {
    Sweep sweep{domain, shore, 0, 0, float(years), domain.cell * domain.cell};
    for (int order = 0; order < WaveDirections; ++order) {
        // The trains take turns to go first, so that none of them always works on what the others left.
        sweep.train = (order + 3 * round) & 7;
        sweep.run = float(domain.cell) * NeighborReach[sweep.train];
        sweepDownwind(shore, sweep.train, [&](int x, int z) { sweep.visit(x, z); });
    }
}

} // namespace derivean::sim
