#include "landscape/fn/Process.hpp"

namespace derivean::sim {
namespace {

constexpr float RegolithDepth = 0.6f;     // m, e-folding of weathering under its own waste
constexpr float CreepRate = 0.02f;        // m²/yr, soil diffusivity under a temperate climate
// Without roots and burrowers soil moves by rain splash, wetting and frost alone: an order of
// magnitude less than under plants, more where the ground freezes and thaws through the year.
constexpr float BareCreepRate = 0.02f;    // m²/yr, the Region, Local and Site levels
constexpr float FrostCreep = 12.0f;       // gain of that rate per unit share of the year spent cracking
// A ground cell hides nothing: what moves soil across a quarter of a metre is splash, wetting and
// frost alone, a small part of what a wider cell sums up.
constexpr float GroundCreepShare = 0.05f;
constexpr float FrostShatter = 10.0f;     // gain of regolith production per unit share of the year spent cracking
constexpr int FailureRounds = 3;
// A cell can be the high end of eight pairs at once; this share keeps all of them together from
// moving more than the excess that set them off, which would make neighbours trade places.
constexpr float PairShare = 0.1f;

// Each unordered pair of neighbours once: east, south and the two southern diagonals.
constexpr int PairX[4] = {1, 0, 1, -1};
constexpr int PairZ[4] = {0, 1, 1, 1};
constexpr float PairReach[4] = {1, 1, 1.41421356f, 1.41421356f};

bool underWater(const Domain& domain, size_t i) { return domain.level[i] > domain.surface[i] + 1e-3f; }

// Rock breaks down under a mantle that slows the attack as it thickens; the soluble share leaves
// with the runoff, and soluble rock is also lowered directly by the water passing over it.
void weather(Domain& domain) {
    const float dt = float(domain.step);
    const float area = float(domain.cell * domain.cell);
    const int size = domain.size;
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.hold[i] != Free || underWater(domain, i)) continue;
        const RockTraits& rock = domain.unit[i].traits;
        const float wet = (0.3f + 0.7f * std::min(1.0f, domain.rain[i])) * (1.0f + FrostShatter * frostShare(domain, i));
        const float mantle = domain.sedimentAt(i);
        // Under more than a few metres of cover the rock is out of reach of the weather.
        const float grown = mantle > 9.0f ? 0.0f : RegolithDepth * std::log(std::exp(mantle / RegolithDepth) + rock.weathering * wet * dt / RegolithDepth) - mantle;
        const float dissolved = rock.solubility * domain.runoff[i] * dt + grown * rock.solute;
        domain.ground.rock[i] -= grown + rock.solubility * domain.runoff[i] * dt;
        domain.ground.sediment[Coarse][i] += grown * rock.coarse;
        domain.ground.sediment[Sand][i] += grown * rock.sand;
        domain.ground.sediment[Fines][i] += grown * rock.fines;
        if (domain.owned(int(i % size_t(size)), int(i / size_t(size)))) {
            domain.ledger.weathered += double(grown) * area;
            domain.ledger.dissolved += double(dissolved) * area;
        }
    }
}

void applyMoves(Domain& domain, FlowScratch& scratch) {
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.hold[i] != Free) continue;
        for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] = std::max(0.0f, domain.ground.sediment[c][i] + scratch.moved[c][i]);
        domain.ground.rock[i] += scratch.moved[SedimentClasses][i];
    }
}

// One implicit pass of diffusion along every row or every column (Thomas algorithm). Cells that do
// not evolve keep their height and act as fixed ends.
void diffuseLines(const Domain& domain, const std::vector<float>& mobility, std::vector<float>& height, bool alongX, float number) {
    const int size = domain.size;
    const size_t count = size_t(size);
    // A line reads and writes its own cells only: lines may be solved side by side.
    forRows(domain, size, [&](int begin, int end) {
    std::vector<float> lower(count, 0.0f), diagonal(count, 0.0f), upper(count, 0.0f), right(count, 0.0f);
    for (int line = begin; line < end; ++line) {
        const auto at = [&](int k) { return alongX ? domain.index(k, line) : domain.index(line, k); };
        for (int k = 0; k < size; ++k) {
            const size_t i = at(k);
            const bool fixed = domain.hold[i] != Free || k == 0 || k == size - 1;
            const float back = fixed ? 0.0f : number * 0.5f * (mobility[i] + mobility[at(k - 1)]);
            const float ahead = fixed ? 0.0f : number * 0.5f * (mobility[i] + mobility[at(k + 1)]);
            lower[size_t(k)] = -back;
            upper[size_t(k)] = -ahead;
            diagonal[size_t(k)] = 1 + back + ahead;
            right[size_t(k)] = height[i];
        }
        for (int k = 1; k < size; ++k) {
            const float factor = lower[size_t(k)] / diagonal[size_t(k - 1)];
            diagonal[size_t(k)] -= factor * upper[size_t(k - 1)];
            right[size_t(k)] -= factor * right[size_t(k - 1)];
        }
        height[at(size - 1)] = right[size_t(size - 1)] / diagonal[size_t(size - 1)];
        for (int k = size - 2; k >= 0; --k) height[at(k)] = (right[size_t(k)] - upper[size_t(k)] * height[at(k + 1)]) / diagonal[size_t(k)];
    }
    });
}

// Soil creeps downhill in proportion to slope. A step of the finer levels spans far more time than
// an explicit exchange between neighbours could carry, so the diffusion is solved implicitly; the
// result is then paid for cell by cell: a cell can only lose the soil it has plus what its rock
// weathers in the step, and what all cells lose is what the gaining cells share.
void creep(Domain& domain, FlowScratch& scratch) {
    const size_t cells = domain.cells();
    const float dt = float(domain.step);
    // A coarse cell hides many small slopes that mix soil among themselves, so its effective
    // diffusivity grows with its width.
    const bool ground = domain.key.level == Level::Ground;
    const float rate = ground ? CreepRate * GroundCreepShare : BareCreepRate * std::max(1.0f, float(domain.cell) / 64.0f);
    const float number = rate * dt / float(domain.cell * domain.cell);
    std::vector<float>& mobility = scratch.moved[0];
    std::vector<float>& height = scratch.moved[1];
    std::vector<float>& before = scratch.moved[2];
    mobility.assign(cells, 0.0f);
    height.resize(cells);
    before.resize(cells);
    for (size_t i = 0; i < cells; ++i) {
        before[i] = height[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        // Bare rock barely creeps; a mantle moves freely once it is a few decimetres thick.
        mobility[i] = underWater(domain, i) ? 0.3f : std::clamp(domain.sedimentAt(i) / 0.4f, 0.06f, 1.0f);
        if (!ground) mobility[i] *= 1.0f + FrostCreep * frostShare(domain, i);
    }
    diffuseLines(domain, mobility, height, true, number);
    diffuseLines(domain, mobility, height, false, number);

    double lost[SedimentClasses] = {}, wanted = 0;
    for (size_t i = 0; i < cells; ++i) {
        if (domain.hold[i] != Free) continue;
        const float change = height[i] - before[i];
        if (change >= 0) {
            wanted += change;
            continue;
        }
        const float loose = domain.sedimentAt(i);
        const float fromLoose = std::min(-change, loose);
        const RockTraits& rock = domain.unit[i].traits;
        const float fromRock = std::min(-change - fromLoose, 2.0f * rock.weathering * dt);
        for (int c = 0; c < SedimentClasses; ++c) {
            const float part = loose > 0 ? fromLoose * domain.ground.sediment[c][i] / loose : 0.0f;
            domain.ground.sediment[c][i] -= part;
            lost[c] += part;
        }
        domain.ground.rock[i] -= fromRock;
        lost[Coarse] += fromRock * rock.coarse;
        lost[Sand] += fromRock * rock.sand;
        lost[Fines] += fromRock * (rock.fines + rock.solute);
    }
    const double supply = lost[0] + lost[1] + lost[2];
    if (wanted <= 0 || supply <= 0) return;
    const double share = std::min(1.0, supply / wanted);
    for (size_t i = 0; i < cells; ++i) {
        if (domain.hold[i] != Free) continue;
        const float change = height[i] - before[i];
        if (change <= 0) continue;
        for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] += float(change * share * lost[c] / supply);
    }
}

// Slopes steeper than their material can hold shed the excess to the cell below. Loose cover goes
// first; rock that fails arrives as blocks and scree.
void fail(Domain& domain, FlowScratch& scratch) {
    const int size = domain.size;
    const float cell = float(domain.cell);
    const float area = cell * cell;
    // A coarse cell averages cliffs with the benches between them, so it holds a gentler mean slope.
    const float rockScale = float(1.0 - 0.5 * smoothstep(8.0, 1200.0, domain.cell));
    std::vector<float>& broke = scratch.broke;
    broke.assign(domain.cells(), 0.0f);
    for (int round = 0; round < FailureRounds; ++round) {
        for (auto& layer : scratch.moved) layer.assign(domain.cells(), 0.0f);
        for (int z = 0; z < size - 1; ++z) {
            for (int x = 1; x < size - 1; ++x) {
                const size_t a = domain.index(x, z);
                for (int pair = 0; pair < 4; ++pair) {
                    const size_t b = domain.index(x + PairX[pair], z + PairZ[pair]);
                    const float ha = domain.ground.rock[a] + domain.sedimentAt(a), hb = domain.ground.rock[b] + domain.sedimentAt(b);
                    const size_t from = ha > hb ? a : b, to = ha > hb ? b : a;
                    if (domain.hold[a] != Free || domain.hold[b] != Free) continue;
                    const float drop = std::abs(ha - hb), reach = PairReach[pair] * cell;
                    const float loose = domain.sedimentAt(from);
                    float repose = 0;
                    for (int c = 0; c < SedimentClasses; ++c) repose += ReposeTan[c] * domain.ground.sediment[c][from];
                    repose = loose > 0 ? repose / loose : 0.7f;
                    const float looseExcess = drop - repose * reach;
                    if (looseExcess <= 0) continue;
                    const float shed = std::min(PairShare * looseExcess, 0.2f * loose);
                    if (loose > 0)
                        for (int c = 0; c < SedimentClasses; ++c) {
                            const float part = shed * domain.ground.sediment[c][from] / loose;
                            scratch.moved[c][from] -= part;
                            scratch.moved[c][to] += part;
                        }
                    // Rock under a thick mantle is not the face that is standing too steep.
                    // A face the parent holds is held: the ground level breaks only what stands steeper
                    // than both its rock allows and the parent's surface is there.
                    const float held = domain.key.level == Level::Ground ? 1.15f * std::abs(domain.trend[a] - domain.trend[b]) : 0.0f;
                    const float rockExcess = drop - shed - std::max(held, domain.unit[from].traits.stableTan * rockScale * reach);
                    if (rockExcess <= 0 || loose > 1.0f) continue;
                    const float broken = PairShare * rockExcess;
                    scratch.moved[SedimentClasses][from] -= broken;
                    scratch.moved[Coarse][to] += broken;
                    broke[from] += broken;
                    if (domain.owned(int(from % size_t(size)), int(from / size_t(size)))) domain.ledger.failed += double(broken) * area;
                }
            }
        }
        applyMoves(domain, scratch);
    }
    const float dt = float(domain.step);
    for (size_t i = 0; i < domain.shed.size(); ++i) domain.shed[i] += 0.4f * (broke[i] / dt - domain.shed[i]);
}

} // namespace

void shapeSlopes(Domain& domain, FlowScratch& scratch) {
    // A ground cell is narrower than a joint block: its faces fail pair by pair. The coarser levels
    // judge a face by the strength of the rock mass over the whole of its height, and before the
    // step's waste has formed on it: a face is bare because what weathers off it falls.
    if (domain.key.level == Level::Ground) {
        weather(domain);
        creep(domain, scratch);
        fail(domain, scratch);
        return;
    }
    wearDebris(domain);
    breakSlopes(domain, scratch);
    weather(domain);
    creep(domain, scratch);
    settleDebris(domain, scratch);
}

} // namespace derivean::sim
