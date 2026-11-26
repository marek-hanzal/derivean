#include "aeolian/fn/Wind.hpp"

namespace derivean::sim {
namespace {

// Dust is two sizes: coarse silt that comes down within tens of kilometres and makes loess, and
// fine dust that stays up for hundreds. Rain washes both out sooner.
constexpr int Sizes = 2;
constexpr float SizeShare[Sizes] = {0.6f, 0.4f};
constexpr float FallLength[Sizes] = {60e3f, 600e3f};   // m of travel over which all but 1/e has settled in dry air
constexpr float WashRain = 0.5f;                       // m/yr of rain that doubles the fall-out

} // namespace

// Dust travels down the same graph the sand does: the drift that moves sand is the wind strong
// enough to raise and carry silt. Every cell on the way keeps the share that settles while the air
// crosses it. It settles on everything, water included; where it stays is then up to what works
// the ground: running water takes it from slopes, passing sand strikes it up again.
void settleDust(Domain& domain, const WindGraph& graph, const std::vector<float>& raised) {
    WindField& wind = domain.aeolian;
    const size_t cells = domain.cells();
    const float cell = float(domain.cell), area = cell * cell;
    std::vector<float> load[Sizes];
    for (auto& size : load) size.assign(cells, 0.0f);
    // Where the graph closes on itself air can reach a cell already dealt with; what it holds comes down there.
    std::vector<float> late(cells, 0.0f);
    std::vector<uint8_t> done(cells, 0);
    const auto lay = [&](size_t i, float settled) {
        if (settled <= 0) return;
        if (domain.hold[i] != Free) {
            wind.ledger.dustOut += settled;
            return;
        }
        const bool water = heldByWater(domain, i);
        domain.ground.sediment[Fines][i] += settled / area;
        bookWind(domain, i, settled / area);
        if (!water) wind.labels.loess[i] += settled / area;
        (water ? wind.ledger.dustDrowned : wind.ledger.dustSettled) += settled;
    };
    for (const int32_t at : graph.order) {
        const size_t i = size_t(at);
        done[i] = 1;
        float settled = 0;
        for (int s = 0; s < Sizes; ++s) {
            float& air = load[s][i];
            air += raised[i] * area * SizeShare[s];
            const float falls = air * (1.0f - std::exp(-cell / FallLength[s] * (1.0f + domain.rain[i] / WashRain)));
            air -= falls, settled += falls;
            for (int k = 0; k < 2; ++k) {
                const float part = air * graph.share[i * 2 + size_t(k)];
                const int32_t next = graph.to[i * 2 + size_t(k)];
                if (next < 0) wind.ledger.dustOut += part;
                else if (done[size_t(next)]) late[size_t(next)] += part;
                else load[s][size_t(next)] += part;
            }
            // Air that stands still over a cell drops all it holds there.
            if (graph.share[i * 2] + graph.share[i * 2 + 1] <= 0) settled += air;
        }
        lay(i, settled);
    }
    for (size_t i = 0; i < cells; ++i) lay(i, late[i]);
}

} // namespace derivean::sim
