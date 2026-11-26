#include "ground/fn/Hydrothermal.hpp"

#include "chronology/type/Calendar.hpp"
#include "core/fn/Hash.hpp"

#include <queue>

namespace derivean::sim {
namespace {

constexpr int Epochs = 60;
constexpr float ActiveYears = 600.0f;      // yr a vent keeps its place
constexpr float Caught = 0.02f;            // share of the dissolved load that comes out within reach of the vent
constexpr float SinterDensity = 1900.0f, TravertineDensity = 2200.0f;   // kg/m³
constexpr float FilmGrowth = 0.25f;        // mm/yr of mineral under a quiet film of fresh spring water
constexpr float FilmDepth = 0.02f;         // m of water under which mineral comes out fastest
constexpr float SpillGain = 12.0f;         // how much faster it comes out per unit fall of the water surface
constexpr float Unevenness = 0.012f;       // m of seeded roughness the first film runs over; pools start from it
constexpr float SprayLayer = 0.03f;        // m a cone gains in one epoch where its spray falls thickest
constexpr float CoolLength = 14.0f;        // m of flow over which hot water loses most of its heat
constexpr float HotVent = 70.0f;           // °C above which the ground around a vent is altered

} // namespace

void growSpringDeposit(const Spring& spring, SpringGround& around) {
    const int side = around.side;
    const size_t cells = size_t(side) * size_t(side);
    const float cell = float(around.cell), area = cell * cell;
    around.crust.assign(cells, 0.0f);
    around.kind.assign(cells, uint8_t(Crust::None));
    around.altered.assign(cells, 0.0f);
    around.pool.assign(cells, 0.0f);
    const int ventX = int((spring.position.x - around.origin.x) / around.cell), ventZ = int((spring.position.z - around.origin.z) / around.cell);
    if (ventX < 2 || ventZ < 2 || ventX >= side - 2 || ventZ >= side - 2 || spring.discharge <= 0) return;
    const size_t vent = size_t(ventZ) * size_t(side) + size_t(ventX);

    const float years = ActiveYears / Epochs;
    // m³ of solid per year the water gives up within reach: sinter as it cools, travertine as it degasses.
    const float sinterSupply = Caught * spring.silica * spring.discharge * float(YearSeconds) / SinterDensity;
    const float travertineSupply = Caught * spring.carbonate * spring.discharge * float(YearSeconds) / TravertineDensity;
    std::vector<float> top(cells), level(cells), water(cells), load(cells), sinter(cells, 0.0f), travertine(cells, 0.0f), heat(cells);
    std::vector<int32_t> order;
    std::vector<uint8_t> seen(cells);
    using Entry = std::pair<float, int32_t>;
    // No ground is as smooth as a surface read off a coarser level; the roughness is fixed in the world.
    std::vector<float> rough(cells);
    const int64_t firstX = int64_t(std::llround(around.origin.x / around.cell)), firstZ = int64_t(std::llround(around.origin.z / around.cell));
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) rough[size_t(z) * size_t(side) + size_t(x)] = Unevenness * float(unit(hash(spring.id ^ 0x53505247, firstX + x, firstZ + z)));

    for (int epoch = 0; epoch < Epochs; ++epoch) {
        // Fill every hollow of the surface as it stands, from the open edge inward.
        for (size_t i = 0; i < cells; ++i) top[i] = around.ground[i] + rough[i] + sinter[i] + travertine[i];
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
        std::fill(seen.begin(), seen.end(), 0);
        order.clear();
        for (int z = 0; z < side; ++z)
            for (int x = 0; x < side; ++x) {
                if (x > 0 && z > 0 && x < side - 1 && z < side - 1) continue;
                const size_t i = size_t(z) * size_t(side) + size_t(x);
                seen[i] = 1, level[i] = top[i];
                open.push({level[i], int32_t(i)});
            }
        while (!open.empty()) {
            const int32_t at = open.top().second;
            open.pop();
            order.push_back(at);
            const int x = at % side, z = at / side;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= side || nz >= side) continue;
                const size_t other = size_t(nz) * size_t(side) + size_t(nx);
                if (seen[other]) continue;
                seen[other] = 1;
                level[other] = std::max(top[other], std::nextafter(level[size_t(at)], 1e9f));
                open.push({level[other], int32_t(other)});
            }
        }
        // The spring's water downhill over the filled surface, upstream cells first. `water` is the
        // share of the spring that passes a cell, `load` the share of its mineral still dissolved
        // there and `heat` how much of its warmth it still has.
        std::fill(water.begin(), water.end(), 0.0f);
        std::fill(load.begin(), load.end(), 0.0f);
        std::fill(heat.begin(), heat.end(), 0.0f);
        water[vent] = load[vent] = 1.0f - spring.steamShare;
        heat[vent] = 1;
        for (size_t step = order.size(); step-- > 0;) {
            const size_t i = size_t(order[step]);
            if (water[i] <= 1e-5f) continue;
            const int x = int(i % size_t(side)), z = int(i / size_t(side));
            float slope[8], total = 0, steepest = 0;
            for (int k = 0; k < 8; ++k) {
                slope[k] = 0;
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= side || nz >= side) continue;
                const size_t other = size_t(nz) * size_t(side) + size_t(nx);
                if (level[other] >= level[i]) continue;
                slope[k] = (level[i] - level[other]) / (cell * NeighborReach[k]);
                total += slope[k];
                steepest = std::max(steepest, slope[k]);
            }
            // Mineral comes out where the water is a thin film and where it spills: on rims and lips.
            // Silica needs the water to have cooled as well; carbonate only needs it to lose its gas.
            // What a cell takes is gone from the water that runs on.
            const float depth = level[i] - top[i];
            const float eager = (1.0f + SpillGain * std::min(steepest, 1.5f)) / (1.0f + depth / FilmDepth);
            const float wantSinter = sinterSupply * (0.25f + 0.75f * (1.0f - heat[i])), wantTravertine = travertineSupply;
            const float supply = (wantSinter + wantTravertine) * years;   // m³ the whole spring could lay this epoch as this cell's water is
            float dropped = 0;
            if (supply > 0) {
                // Beyond the reach a deposit is followed to nothing is laid; it thins towards there.
                const float away = cell * std::sqrt(float((x - ventX) * (x - ventX) + (z - ventZ) * (z - ventZ)));
                const float within = std::max(0.0f, 1.0f - away * away / float(SpringReach * SpringReach));
                const float laid = std::min(0.001f * FilmGrowth * years * eager * within * load[i] / water[i], load[i] * supply / area);
                sinter[i] += laid * wantSinter / (wantSinter + wantTravertine);
                travertine[i] += laid * wantTravertine / (wantSinter + wantTravertine);
                dropped = laid * area / supply;
            }
            if (total <= 0) continue;
            for (int k = 0; k < 8; ++k) {
                if (slope[k] <= 0) continue;
                const size_t other = size_t(z + NeighborZ[k]) * size_t(side) + size_t(x + NeighborX[k]);
                const float part = slope[k] / total;
                heat[other] = std::max(heat[other], heat[i] * std::exp(-cell * NeighborReach[k] / CoolLength));
                water[other] += water[i] * part;
                load[other] += (load[i] - dropped) * part;
            }
        }
        // What leaves as steam and spray falls back around the vent: most at a little distance, so a
        // cone with a throat grows. A cone gains no more than SprayLayer in an epoch where it gains
        // most; what the spray carried beyond that is blown away.
        if (spring.steamShare > 0.05f) {
            const float ring = 0.5f + 2.0f * spring.steamShare, volume = spring.steamShare * sinterSupply * years;
            const int reach = int(4 * ring / cell) + 1;
            const auto fallen = [&](int dx, int dz) {
                const float r = cell * std::sqrt(float(dx * dx + dz * dz)) / ring;
                return r > 4.0f ? 0.0f : r * std::exp(-r);
            };
            float weight = 0;
            for (int dz = -reach; dz <= reach; ++dz)
                for (int dx = -reach; dx <= reach; ++dx) weight += fallen(dx, dz);
            // The fall is thickest one ring out, where r·exp(−r) is 1/e.
            const float thickest = volume * 0.3679f / weight / area, kept = std::min(1.0f, SprayLayer / thickest);
            for (int dz = -reach; dz <= reach; ++dz)
                for (int dx = -reach; dx <= reach; ++dx) {
                    const int x = ventX + dx, z = ventZ + dz;
                    if (x >= 0 && z >= 0 && x < side && z < side) sinter[size_t(z) * size_t(side) + size_t(x)] += kept * volume * fallen(dx, dz) / weight / area;
                }
        }
    }

    for (size_t i = 0; i < cells; ++i) {
        around.crust[i] = sinter[i] + travertine[i];
        if (around.crust[i] > 0.005f) around.kind[i] = uint8_t(travertine[i] > sinter[i] ? Crust::Travertine : Crust::Sinter);
        around.pool[i] = water[i] > 1e-4f ? std::max(0.0f, level[i] - top[i]) : 0.0f;
        // Hot water and its steam bleach what they soak: around the vent, and along the hot reach of the run-off.
        const int x = int(i % size_t(side)), z = int(i / size_t(side));
        const float away = cell * std::sqrt(float((x - ventX) * (x - ventX) + (z - ventZ) * (z - ventZ)));
        const float scald = std::clamp((spring.temperature - HotVent) / 25.0f, 0.0f, 1.0f);
        const float halo = 2.0f + 5.0f * scald * (0.4f + spring.steamShare);
        around.altered[i] = scald * std::max(std::exp(-away * away / (halo * halo)), water[i] > 1e-3f ? heat[i] : 0.0f);
    }
}

} // namespace derivean::sim
