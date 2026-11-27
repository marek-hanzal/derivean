#include "ground/fn/Hydrothermal.hpp"

#include "ground/constant/Ground.hpp"

namespace derivean::sim {

void laySpringDeposits(Domain& domain, const std::vector<Spring>& springs, const SiteView& sites, GroundDetail& detail, std::vector<float>& altered) {
    altered.assign(domain.cells(), 0.0f);
    const int64_t firstX = int64_t(std::llround(domain.origin.x / domain.cell)), firstZ = int64_t(std::llround(domain.origin.z / domain.cell));
    for (const Spring& spring : springs) {
        // Its water joins the last routing, so the pools it dammed hold it.
        const auto feed = [&] {
            const int64_t x = int64_t(std::floor(spring.position.x / domain.cell)), z = int64_t(std::floor(spring.position.z / domain.cell));
            const int vx = int(x - firstX), vz = int(z - firstZ);
            if (vx > 0 && vz > 0 && vx < domain.size - 1 && vz < domain.size - 1) domain.inflow[domain.index(vx, vz)] += (1.0f - spring.steamShare) * spring.discharge * float(YearSeconds);
        };
        if (laysNothing(spring, domain.cell)) {
            feed();
            continue;
        }
        // A window on the ground level's own lattice, centred on the vent.
        SpringGround around;
        around.cell = domain.cell;
        const int reach = int(SpringReach / domain.cell);
        const int64_t ventX = int64_t(std::floor(spring.position.x / domain.cell)), ventZ = int64_t(std::floor(spring.position.z / domain.cell));
        around.side = 2 * reach + 1;
        around.origin = {double(ventX - reach) * domain.cell, double(ventZ - reach) * domain.cell};
        around.ground.resize(size_t(around.side) * size_t(around.side));
        for (int z = 0; z < around.side; ++z)
            for (int x = 0; x < around.side; ++x)
                around.ground[size_t(z) * size_t(around.side) + size_t(x)] = sites.groundAt({around.origin.x + (x + 0.5) * domain.cell, around.origin.z + (z + 0.5) * domain.cell});
        growSpringDeposit(spring, around);
        for (int z = 0; z < around.side; ++z)
            for (int x = 0; x < around.side; ++x) {
                const int dx = int(ventX - reach - firstX) + x, dz = int(ventZ - reach - firstZ) + z;
                if (dx < 0 || dz < 0 || dx >= domain.size || dz >= domain.size) continue;
                const size_t from = size_t(z) * size_t(around.side) + size_t(x), to = domain.index(dx, dz);
                altered[to] = std::max(altered[to], around.altered[from]);
                if (!domain.owned(dx, dz) || around.crust[from] <= 0) continue;
                const size_t own = size_t(dz - Halo) * TileCells + size_t(dx - Halo);
                if (around.crust[from] > detail.crust[own]) detail.crustKind[own] = around.kind[from];
                detail.crust[own] += around.crust[from];
            }
        feed();
    }
}

} // namespace derivean::sim
