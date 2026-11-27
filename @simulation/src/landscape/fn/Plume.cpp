#include "landscape/fn/Process.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr float Freeboard = 0.02f;   // m a bed stays below the water that buries it

} // namespace

// Sediment that reaches standing water spreads from where it entered and sinks as it goes: gravel
// within a cell or two of the river mouth, sand a little farther, mud across the basin. The spread
// has no direction of its own, so a delta takes the shape of the shore and the supply, and a bed
// never rises above the water that carries the sediment to it.
void settlePlume(Domain& domain, FlowScratch& scratch, float dt, float area) {
    constexpr int Rounds = 16;
    constexpr float Spread = 0.2f;                                    // share passed to each of four neighbours per round
    constexpr float Sinking[SedimentClasses] = {0.7f, 0.35f, 0.06f};   // share that reaches the bed per round
    const int size = domain.size;
    bool any = false;
    for (const auto& cloud : scratch.plume)
        for (float volume : cloud) any = any || volume > 0;
    if (!any) return;

    // The ledger is the tile's: what entered the plume on its own cells against what the plume laid
    // there. The net of the two crossed the tile's edge as suspended load, out or in; what goes out
    // and comes back over the same edge within the step cancels.
    const auto owned = [&](size_t i) { return domain.owned(int(i % size_t(size)), int(i / size_t(size))); };
    double entered = 0, laidHere = 0;
    for (const auto& cloud : scratch.plume)
        for (size_t i = 0; i < cloud.size(); ++i)
            if (cloud[i] > 0 && owned(i)) entered += cloud[i];
    std::vector<float> next(domain.cells());
    for (int round = 0; round < Rounds; ++round) {
        for (int c = 0; c < SedimentClasses; ++c) {
            std::vector<float>& cloud = scratch.plume[c];
            for (size_t i = 0; i < domain.cells(); ++i) {
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
            std::fill(next.begin(), next.end(), 0.0f);
            for (int z = 1; z < size - 1; ++z) {
                for (int x = 1; x < size - 1; ++x) {
                    const size_t i = domain.index(x, z);
                    if (cloud[i] <= 0) continue;
                    float kept = cloud[i];
                    for (int k = 0; k < 8; k += 2) {
                        const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                        if (domain.level[other] <= domain.surface[other] + 1e-3f) continue;   // only into water
                        next[other] += Spread * cloud[i];
                        kept -= Spread * cloud[i];
                    }
                    next[i] += kept;
                }
            }
            cloud.swap(next);
        }
    }
    // What a full basin could not take stays suspended and leaves with the water.
    (entered > laidHere ? domain.ledger.sedimentOut : domain.ledger.sedimentIn) += std::abs(entered - laidHere);
}

} // namespace derivean::sim
