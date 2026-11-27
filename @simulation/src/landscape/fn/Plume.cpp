#include "landscape/fn/Process.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr float Freeboard = 0.02f;   // m a bed stays below the water that buries it

} // namespace

// Sediment that reaches standing water spreads from where it entered and sinks as it goes: gravel
// within a cell or two of the river mouth, sand a little farther, mud across the basin. The spread
// has no direction of its own, so a delta takes the shape of the shore and the supply, and a bed
// never rises above the water that carries the sediment to it.
//
// A plume covers a few cells around the mouths, so each class keeps the cells it holds anything in
// and works only those, in rising index order: the order of a sweep over the whole domain, so every
// sum is taken in the same order and the result is that sweep's to the bit. It starts from the cells
// the river fed, the only ones not at zero, and leaves every cell at zero for the next step.
void settlePlume(Domain& domain, FlowScratch& scratch, float dt, float area) {
    constexpr int Rounds = 16;
    constexpr float Spread = 0.2f;                                    // share passed to each of four neighbours per round
    constexpr float Sinking[SedimentClasses] = {0.7f, 0.35f, 0.06f};   // share that reaches the bed per round
    const int size = domain.size;
    const size_t cells = domain.cells();

    // The ledger is the tile's: what entered the plume on its own cells against what the plume laid
    // there. The net of the two crossed the tile's edge as suspended load, out or in; what goes out
    // and comes back over the same edge within the step cancels.
    const auto owned = [&](size_t i) { return domain.owned(int(i % size_t(size)), int(i / size_t(size))); };
    double entered = 0, laidHere = 0;
    bool any = false;
    // Each cell once: the river's pass visits every cell of the flow order once.
    std::vector<int32_t>& fed = scratch.plumeFed;
    std::sort(fed.begin(), fed.end());
    for (int c = 0; c < SedimentClasses; ++c) {
        const std::vector<float>& cloud = scratch.plume[c];
        std::vector<int32_t>& active = scratch.plumeCells[c];
        active.clear();
        // A cell the sweeps work is one not at zero or below, NaN included; what entered and whether
        // anything did count only what is above zero.
        for (const int32_t cell : fed) {
            const size_t i = size_t(cell);
            if (cloud[i] <= 0) continue;
            active.push_back(cell);
            if (cloud[i] > 0 && owned(i)) entered += cloud[i];
            any = any || cloud[i] > 0;
        }
    }
    // Every cell still holding anything is one the river fed or one the last round worked.
    const auto clear = [&] {
        for (int c = 0; c < SedimentClasses; ++c) {
            for (const int32_t cell : fed) scratch.plume[c][size_t(cell)] = 0;
            for (const int32_t cell : scratch.plumeCells[c]) scratch.plume[c][size_t(cell)] = 0;
        }
        fed.clear();
    };
    if (!any) return clear();
    std::vector<float>& next = scratch.plumeNext;
    std::vector<uint8_t>& mark = scratch.plumeMark;
    std::vector<int32_t>& reached = scratch.plumeReached;
    if (next.size() != cells) next.assign(cells, 0.0f);
    if (mark.size() != cells) mark.assign(cells, 0);
    const auto reach = [&](size_t i) {
        if (mark[i]) return;
        mark[i] = 1;
        reached.push_back(int32_t(i));
    };
    for (int round = 0; round < Rounds; ++round) {
        for (int c = 0; c < SedimentClasses; ++c) {
            std::vector<float>& cloud = scratch.plume[c];
            std::vector<int32_t>& active = scratch.plumeCells[c];
            for (const int32_t cell : active) {
                const size_t i = size_t(cell);
                if (cloud[i] <= 0) continue;
                if (domain.hold[i] == Rim) {
                    cloud[i] = 0;
                    continue;
                }
                if (domain.hold[i] != Free) continue;
                const float depth = domain.level[i] - Freeboard - domain.ground.rock[i] - domain.sedimentAt(i);
                const float laid = std::min(std::max(0.0f, depth), (round + 1 == Rounds ? 1.0f : Sinking[c]) * cloud[i] / area);
                if (laid <= 0) continue;
                domain.ground.sediment[c][i] += laid;
                cloud[i] -= laid * area;
                domain.deposited[i] += laid;
                domain.depositionRate[i] += 0.4f * laid / dt;
                if (owned(i)) domain.ledger.deposited += double(laid) * area, laidHere += double(laid) * area;
            }
            if (round + 1 == Rounds) continue;   // the last round only lays, every class
            // The spread of the round, from the cells in rising order; the outer ring of the domain
            // passes nothing on and keeps nothing.
            reached.clear();
            for (const int32_t cell : active) {
                const size_t i = size_t(cell);
                const int x = cell % size, z = cell / size;
                if (x < 1 || z < 1 || x >= size - 1 || z >= size - 1 || cloud[i] <= 0) continue;
                float kept = cloud[i];
                for (int k = 0; k < 8; k += 2) {
                    const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                    if (domain.level[other] <= domain.surface[other] + 1e-3f) continue;   // only into water
                    next[other] += Spread * cloud[i];
                    kept -= Spread * cloud[i];
                    reach(other);
                }
                next[i] += kept;
                reach(i);
            }
            for (const int32_t cell : active) cloud[size_t(cell)] = 0;
            std::sort(reached.begin(), reached.end());
            for (const int32_t cell : reached) {
                cloud[size_t(cell)] = next[size_t(cell)];
                next[size_t(cell)] = 0;
                mark[size_t(cell)] = 0;
            }
            active.swap(reached);
        }
    }
    // What a full basin could not take stays suspended and leaves with the water.
    (entered > laidHere ? domain.ledger.sedimentOut : domain.ledger.sedimentIn) += std::abs(entered - laidHere);
    clear();
}

} // namespace derivean::sim
