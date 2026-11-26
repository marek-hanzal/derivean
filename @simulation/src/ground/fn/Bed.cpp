#include "ground/fn/Stones.hpp"

#include "ground/constant/Ground.hpp"

namespace derivean::sim {
namespace {

constexpr float SteepBed = 0.035f;     // fall above which a bed of boulders forms steps
constexpr float LeastKey = 0.3f;       // m, smallest stone that holds a step
constexpr float RockFall = 0.5f;       // m of drop over bare rock that makes a fall
constexpr float SoftRock = 6e-6f;      // erodibility above which falling water digs its pool into the rock itself
constexpr float DeepestPool = 3.0f;    // m

struct Reach {
    std::vector<int32_t> cells;   // down the channel
    std::vector<float> along;     // m from its head
};

int mainReceiver(const Domain& domain, const FlowScratch& scratch, size_t i) {
    const float* share = &scratch.share[i * 8];
    int main = -1;
    for (int k = 0; k < 8; ++k)
        if (share[k] > 0 && (main < 0 || share[k] > share[main])) main = k;
    return main < 0 ? -1 : int(domain.index(int(i % size_t(domain.size)) + NeighborX[main], int(i / size_t(domain.size)) + NeighborZ[main]));
}

// Lowers the loose cover within `reach` cells of a cell by up to `depth`, shallower towards the
// sides, and digs on into soft rock. What it takes joins `carry` by class; `dug` counts the m³ of
// coarse waste that came out of the rock.
void scour(Domain& domain, size_t at, int reach, float depth, float carry[SedimentClasses], double& dug) {
    const int cx = int(at % size_t(domain.size)), cz = int(at / size_t(domain.size));
    for (int z = cz - reach; z <= cz + reach; ++z)
        for (int x = cx - reach; x <= cx + reach; ++x) {
            const int far = (x - cx) * (x - cx) + (z - cz) * (z - cz);
            if (!domain.owned(x, z) || far > reach * reach) continue;
            const size_t i = domain.index(x, z);
            float want = depth * (1.0f - float(far) / float(reach * reach + 1));
            const float loose = domain.sedimentAt(i), taken = std::min(want, loose);
            if (loose > 0)
                for (int c = 0; c < SedimentClasses; ++c) {
                    const float part = std::min(domain.ground.sediment[c][i], taken * domain.ground.sediment[c][i] / loose);
                    domain.ground.sediment[c][i] -= part;
                    carry[c] += part;
                }
            want -= taken;
            const RockTraits& rock = domain.unit[i].traits;
            if (want <= 0 || rock.erodibility < SoftRock) continue;
            const float deeper = 0.5f * want;
            domain.ground.rock[i] -= deeper;
            carry[Coarse] += deeper * rock.coarse, carry[Sand] += deeper * rock.sand, carry[Fines] += deeper * (rock.fines + rock.solute);
            dug += double(deeper * rock.coarse) * domain.cell * domain.cell;
        }
}

// Lays what the water carries on the bed around a cell, no higher than `crest`.
void fill(Domain& domain, size_t at, int reach, float crest, float carry[SedimentClasses]) {
    const int cx = int(at % size_t(domain.size)), cz = int(at / size_t(domain.size));
    for (int z = cz - reach; z <= cz + reach; ++z)
        for (int x = cx - reach; x <= cx + reach; ++x) {
            if (!domain.owned(x, z) || (x - cx) * (x - cx) + (z - cz) * (z - cz) > reach * reach) continue;
            const size_t i = domain.index(x, z);
            const float total = carry[0] + carry[1] + carry[2];
            const float room = std::min(total, crest - domain.ground.rock[i] - domain.sedimentAt(i));
            if (room <= 0 || total <= 0) continue;
            for (int c = 0; c < SedimentClasses; ++c) {
                const float part = room * carry[c] / total;
                domain.ground.sediment[c][i] += part;
                carry[c] -= part;
            }
        }
}

} // namespace

void organiseBeds(StoneBed& bed, const FlowScratch& scratch) {
    Domain& domain = bed.domain;
    const float cell = float(domain.cell);
    const auto ground = [&](size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); };
    const auto channel = [&](size_t i) {
        return domain.owned(int(i % size_t(domain.size)), int(i / size_t(domain.size))) && domain.discharge[i] >= StepFlow && domain.level[i] < domain.surface[i] + 0.3f;
    };
    // The stone centred on each cell, and how high each stands over the bed it lies on.
    std::vector<int32_t> stoneAt(domain.cells(), -1);
    std::vector<float> lift(bed.stones.size());
    std::vector<uint8_t> jammed(bed.stones.size(), 0);   // a stone comes to rest against one step only
    const Vec2 corner = bed.tileCorner();
    const auto cellOf = [&](const Clast& stone) { return bed.cellAt({corner.x + stone.x, corner.z + stone.z}); };
    for (size_t s = 0; s < bed.stones.size(); ++s) {
        const size_t at = cellOf(bed.stones[s]);
        stoneAt[at] = int32_t(s);
        lift[s] = bed.stones[s].y - ground(at);
    }

    // Reaches from every channel head down; a reach ends where it joins one already walked.
    std::vector<uint8_t> walked(domain.cells(), 0);
    std::vector<Reach> reaches;
    for (size_t step = domain.cells(); step-- > 0;) {
        size_t i = size_t(domain.order[step]);
        if (!channel(i) || walked[i]) continue;
        Reach reach;
        float along = 0;
        while (channel(i) && !walked[i]) {
            walked[i] = 1;
            reach.cells.push_back(int32_t(i));
            reach.along.push_back(along);
            const int next = mainReceiver(domain, scratch, i);
            if (next < 0) break;
            along += cell * (std::abs(int(next % domain.size) - int(i % size_t(domain.size))) + std::abs(next / domain.size - int(i / size_t(domain.size))) == 2 ? 1.41421356f : 1.0f);
            i = size_t(next);
        }
        if (reach.cells.size() >= 8) reaches.push_back(std::move(reach));
    }

    for (const Reach& reach : reaches) {
        const int count = int(reach.cells.size());
        float carry[SedimentClasses] = {};
        float lastStep = -1;
        int skipTo = 0;
        for (int p = 0; p < count; ++p) {
            const size_t at = size_t(reach.cells[size_t(p)]);
            const int low = std::max(0, p - 6), high = std::min(count - 1, p + 6);
            const float fall = (domain.level[size_t(reach.cells[size_t(low)])] - domain.level[size_t(reach.cells[size_t(high)])]) /
                               std::max(cell, reach.along[size_t(high)] - reach.along[size_t(low)]);
            const float flow = domain.discharge[at];
            const int half = std::clamp(int(0.5f * streamWidth(flow) / cell) + 1, 2, 12);
            if (p < skipTo) continue;
            // A fall over bare rock, or the largest stone of the bed here if the water cannot move it.
            float height = 0;
            const size_t below = size_t(reach.cells[size_t(std::min(count - 1, p + 2))]);
            if (domain.sedimentAt(at) < 0.2f && domain.ground.rock[at] - ground(below) >= RockFall) height = domain.ground.rock[at] - ground(below);
            int key = -1;
            if (height <= 0 && fall >= SteepBed) {
                // Only a stone the water cannot move holds: the few largest of a bed, wherever they came to lie.
                const float least = std::max(LeastKey, competence(flow, fall));
                const int cx = int(at % size_t(domain.size)), cz = int(at / size_t(domain.size));
                for (int z = std::max(0, cz - half); z <= std::min(domain.size - 1, cz + half); ++z)
                    for (int x = std::max(0, cx - half); x <= std::min(domain.size - 1, cx + half); ++x) {
                        const int32_t s = stoneAt[domain.index(x, z)];
                        if (s >= 0 && 2 * bed.stones[size_t(s)].b >= least && (key < 0 || bed.stones[size_t(s)].b > bed.stones[size_t(key)].b)) key = s;
                    }
                if (key >= 0) height = std::max(0.15f, lift[size_t(key)] + bed.stones[size_t(key)].c);
            }
            if (height <= 0) continue;
            if (key >= 0) {
                jammed[size_t(key)] = 1;
                // Smaller stones from just upstream come to rest against it, across the channel.
                Clast& keystone = bed.stones[size_t(key)];
                const size_t before = size_t(reach.cells[size_t(std::max(0, p - 1))]);
                const Vec2 flowDir = normalized(Vec2{double(int(at % size_t(domain.size)) - int(before % size_t(domain.size))), double(int(at / size_t(domain.size)) - int(before / size_t(domain.size)))});
                const Vec2 across = perpendicular(flowDir);
                float left = keystone.a, right = keystone.a;
                for (int back = 1; back <= 3 * half && p - back >= 0; ++back) {
                    const size_t from = size_t(reach.cells[size_t(p - back)]);
                    const int fx = int(from % size_t(domain.size)), fz = int(from / size_t(domain.size));
                    for (int z = std::max(0, fz - half); z <= std::min(domain.size - 1, fz + half); ++z)
                        for (int x = std::max(0, fx - half); x <= std::min(domain.size - 1, fx + half); ++x) {
                            const int32_t s = stoneAt[domain.index(x, z)];
                            if (s < 0 || s == key || jammed[size_t(s)] || bed.stones[size_t(s)].b >= keystone.b || bed.stones[size_t(s)].origin != ClastOrigin::Stream) continue;
                            Clast& stone = bed.stones[size_t(s)];
                            float& side = left <= right ? left : right;
                            if (side > half * cell) continue;
                            const double offset = (left <= right ? -1.0 : 1.0) * (side + 0.8 * stone.a);
                            const Vec2 to{corner.x + keystone.x + across.x * offset - flowDir.x * 0.3 * stone.b, corner.z + keystone.z + across.z * offset - flowDir.z * 0.3 * stone.b};
                            if (!bed.owns(to) || stoneAt[bed.cellAt(to)] >= 0) continue;
                            stoneAt[domain.index(x, z)] = -1;
                            stoneAt[bed.cellAt(to)] = s;
                            stone.x = float(to.x - corner.x), stone.z = float(to.z - corner.z);
                            jammed[size_t(s)] = 1;
                            side += 1.6f * stone.a;
                        }
                }
            }
            // The pool below, and what was dug from the pool above laid behind this step.
            const float crest = ground(at) + (key >= 0 ? 0.6f * height : 0.0f);
            for (int back = 1; back <= 2 * half && p - back >= 0 && carry[0] + carry[1] + carry[2] > 1e-4f; ++back)
                fill(domain, size_t(reach.cells[size_t(p - back)]), half, crest - 0.5f * fall * (reach.along[size_t(p)] - reach.along[size_t(p - back)]), carry);
            const float deepest = std::min(DeepestPool, 0.7f * height * (1.0f + 0.4f * std::log10(flow / StepFlow)));
            const int length = std::clamp(int(2.5f * height / cell) + 2, 3, 40);
            for (int j = 1; j <= length && p + j < count; ++j)
                scour(domain, size_t(reach.cells[size_t(p + j)]), half, deepest * std::sin(3.14159f * float(j) / float(length + 1)), carry, bed.ledger.dug);
            if (lastStep >= 0) bed.stepGaps.push_back(reach.along[size_t(p)] - lastStep);
            lastStep = reach.along[size_t(p)];
            skipTo = p + length;
        }
        // What the last pool gave lies below it as a bar.
        for (int p = std::max(0, count - 12); p < count && carry[0] + carry[1] + carry[2] > 1e-4f; ++p)
            fill(domain, size_t(reach.cells[size_t(p)]), 3, ground(size_t(reach.cells[size_t(p)])) + 0.15f, carry);
        // A bar cannot hold everything on a short reach: the rest lies thin along the whole of it.
        for (int c = 0; c < SedimentClasses; ++c)
            for (const int32_t at : reach.cells) domain.ground.sediment[c][size_t(at)] += carry[c] / float(count);
    }
    for (size_t s = 0; s < bed.stones.size(); ++s) bed.stones[s].y = ground(cellOf(bed.stones[s])) + lift[s];
}

} // namespace derivean::sim
