#include "landscape/fn/Process.hpp"

#include "climate/fn/Seasons.hpp"

namespace derivean::sim {
namespace {

constexpr float IceDepth = 2.7f;             // m of ice per (m³/yr of ice flux)^0.25 at the centre line
constexpr float IceFall = 0.035f;            // slope with which ice thins towards the valley side
constexpr float Plucking = 6.0e-6f;          // 1/yr: erosion per metre of ice per unit slope
constexpr float MoraineLimit = 40.0f;        // m of till one step may leave at an ice front

} // namespace

// Ice gathers where more snow falls than melts and flows down the same fall lines as water. A
// glacier fills its valley to a depth set by the ice it carries, grinds hardest where it is thick
// and steep, and drops what it has plucked where it ends. `cooling` (°C, zero or negative) selects
// the climate: zero for the present, the depth of a glacial maximum for a cold phase.
void glaciate(Domain& domain, FlowScratch& scratch, float halfRange, float cooling, bool erode) {
    const size_t cells = domain.cells();
    const int size = domain.size;
    const float dt = float(domain.step), area = float(domain.cell * domain.cell), cell = float(domain.cell);
    std::vector<float>& flux = scratch.moved[0];
    std::vector<float>& depth = scratch.moved[1];
    std::vector<float>& debris = scratch.moved[2];
    flux.assign(cells, 0.0f);
    depth.assign(cells, 0.0f);
    debris.assign(cells, 0.0f);

    bool any = false;
    for (size_t i = 0; i < cells; ++i) {
        flux[i] = float(annualSnowBalance(domain.warmthNow[i] + cooling, halfRange, domain.rain[i]).net) * area;
        any = any || flux[i] > 0;
    }
    std::fill(domain.ice.begin(), domain.ice.end(), 0.0f);
    if (!any) return;

    // Upstream first: ice passes downhill in proportion to slope until melting has used it up.
    for (size_t step = cells; step-- > 0;) {
        const int32_t at = domain.order[step];
        const size_t i = size_t(at);
        if (flux[i] <= 0 || domain.hold[i] == Rim) {
            flux[i] = std::max(0.0f, flux[i]);
            continue;
        }
        const int x = at % size, z = at / size;
        float slope[8], total = 0;
        for (int k = 0; k < 8; ++k) {
            slope[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            if (domain.rank[other] >= domain.rank[i] || domain.level[other] >= domain.level[i]) continue;
            slope[k] = (domain.level[i] - domain.level[other]) / NeighborReach[k];
            total += slope[k];
        }
        if (total <= 0) continue;
        depth[i] = IceDepth * std::pow(flux[i], 0.25f);
        for (int k = 0; k < 8; ++k) {
            if (slope[k] <= 0) continue;
            const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
            flux[other] += flux[i] * slope[k] / total;
            debris[other] += debris[i] * slope[k] / total;
        }
        if (!erode || domain.hold[i] != Free) continue;
        // Grinding and plucking under the ice; what is loosened travels on with it.
        const float cut = std::min(Plucking * depth[i] * (total / cell / 8.0f + 0.02f) * dt, 0.5f * depth[i]);
        const float loose = domain.sedimentAt(i);
        const float fromLoose = std::min(cut, loose);
        for (int c = 0; c < SedimentClasses; ++c)
            if (loose > 0) domain.ground.sediment[c][i] -= fromLoose * domain.ground.sediment[c][i] / loose;
        domain.ground.rock[i] -= cut - fromLoose;
        debris[i] += cut * area;
        domain.eroded[i] += cut;
        if (domain.owned(x, z)) domain.ledger.eroded += double(cut) * area;
    }

    // Ice stands level across a valley: cells beside the centre line are under ice as far as the
    // surface of their neighbour reaches, falling gently towards the side.
    for (int pass = 0; pass < 10; ++pass) {
        for (int z = 1; z < size - 1; ++z) {
            for (int x = 1; x < size - 1; ++x) {
                const size_t i = domain.index(x, z);
                float surface = domain.surface[i] + depth[i];
                for (int k = 0; k < 8; k += 2) {
                    const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                    // Only ground beside or above the ice is taken in: below it the glacier has its own flow.
                    if (depth[other] > 0 && domain.surface[i] >= domain.surface[other])
                        surface = std::max(surface, domain.surface[other] + depth[other] - IceFall * cell);
                }
                depth[i] = std::max(depth[i], surface - domain.surface[i]);
            }
        }
    }

    for (size_t i = 0; i < cells; ++i) {
        domain.ice[i] = depth[i];
        if (!erode || domain.hold[i] != Free) continue;
        // Under the margins the bed is worn too, less than under the centre line.
        if (depth[i] > 0 && flux[i] <= 0) {
            const float cut = 0.03f * Plucking * depth[i] * dt;
            domain.ground.rock[i] -= std::max(0.0f, cut - domain.sedimentAt(i));
        }
        // The front: ice arrived but none leaves. Its debris stays as a moraine of unsorted till.
        if (debris[i] > 0 && flux[i] <= 0) {
            const float till = std::min(MoraineLimit, debris[i] / area);
            domain.ground.sediment[Coarse][i] += 0.5f * till;
            domain.ground.sediment[Sand][i] += 0.2f * till;
            domain.ground.sediment[Fines][i] += 0.3f * till;
            domain.deposited[i] += till;
            const int x = int(i % size_t(size)), z = int(i / size_t(size));
            if (domain.owned(x, z)) domain.ledger.deposited += double(till) * area;
        }
    }
}

} // namespace derivean::sim
