#include "landscape/fn/Process.hpp"

namespace derivean::sim {
namespace {

constexpr int Rounds = 2;         // the second only meets what the first left within its tolerance
constexpr float Least = 1e-4f;    // m of loose cover below which a cell has nothing to shed
constexpr float Passing = 0.95f;  // share of the angle of repose from which a slope passes moving debris on

// Debris that comes to a stop builds a cone at its angle of repose on whatever lies there: the
// cone's top is raised until the cone holds what arrived. `repose` is the fall per cell of distance.
void heap(Domain& domain, std::vector<float>& height, int x, int z, const float moving[SedimentClasses], float total, float repose) {
    const int size = domain.size;
    const size_t centre = domain.index(x, z);
    // m the cone would hold with its top `rise` above the cell; with `lay` it is laid down, scaled to `total`.
    const auto cone = [&](float rise, bool lay, float scale) {
        const int reach = int(rise / repose) + 1;
        float held = 0;
        for (int dz = -reach; dz <= reach; ++dz) {
            for (int dx = -reach; dx <= reach; ++dx) {
                const int nx = x + dx, nz = z + dz;
                if (nx < 1 || nz < 1 || nx >= size - 1 || nz >= size - 1) continue;
                const size_t i = domain.index(nx, nz);
                if (domain.hold[i] != Free) continue;
                const float room = height[centre] + rise - repose * std::sqrt(float(dx * dx + dz * dz)) - height[i];
                if (room <= 0) continue;
                held += room;
                if (lay) {
                    for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] += moving[c] / total * room * scale;
                    // The top itself is raised last: every other cell is measured against it.
                    if (i != centre) height[i] += room * scale;
                }
            }
        }
        return held;
    };
    float low = 0, high = total;
    for (int k = 0; k < 14; ++k) {
        const float middle = 0.5f * (low + high);
        (cone(middle, false, 1) > total ? high : low) = middle;
    }
    const float held = cone(high, false, 1);
    cone(high, true, total / held);
    height[centre] += high * total / held;
}

} // namespace

// Loose cover that lies steeper than it can rest slides down. First, lowest cell first, the
// surface that does rest is found: no cell more than its angle of repose above a lower neighbour,
// none below its rock. What stands above that surface is on the move: highest cell first it runs
// down slopes that fall at repose or steeper, and where the fall eases it stops and builds a cone
// against whatever stands there. An apron under a wall is many such cones. Cells that do not
// evolve take what reaches them out of the domain.
void settleDebris(Domain& domain, FlowScratch& scratch) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const float cell = float(domain.cell), area = cell * cell;
    std::vector<float>& height = scratch.work;
    for (auto& layer : scratch.moved) layer.assign(cells, 0.0f);
    for (int round = 0; round < Rounds; ++round) {
        for (size_t i = 0; i < cells; ++i) height[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        if (round > 0) orderByHeight(height, scratch.byHeight, scratch.spare);
        bool any = false;
        for (const int32_t at : scratch.byHeight) {
            const size_t i = size_t(at);
            const int x = at % size, z = at / size;
            if (domain.hold[i] != Free || x < 1 || z < 1 || x >= size - 1 || z >= size - 1) continue;
            const float loose = domain.sedimentAt(i);
            if (loose < Least) continue;
            float repose = 0;
            for (int c = 0; c < SedimentClasses; ++c) repose += ReposeTan[c] * domain.ground.sediment[c][i];
            repose *= cell / loose;
            float rests = height[i];
            for (int k = 0; k < 8; ++k) rests = std::min(rests, height[domain.index(x + NeighborX[k], z + NeighborZ[k])] + repose * NeighborReach[k]);
            const float sheds = std::min(loose, height[i] - rests);
            if (sheds < Least) continue;
            for (int c = 0; c < SedimentClasses; ++c) {
                const float part = sheds * domain.ground.sediment[c][i] / loose;
                domain.ground.sediment[c][i] -= part;
                scratch.moved[c][i] += part;
            }
            height[i] -= sheds;
            any = true;
        }
        if (!any) break;
        // Highest first by the surface as it now rests, so debris only ever runs on to cells the
        // pass has still to visit.
        orderByHeight(height, scratch.byHeight, scratch.spare);
        for (size_t step = scratch.byHeight.size(); step-- > 0;) {
            const int32_t at = scratch.byHeight[step];
            const size_t i = size_t(at);
            float moving[SedimentClasses] = {scratch.moved[0][i], scratch.moved[1][i], scratch.moved[2][i]};
            const float total = moving[0] + moving[1] + moving[2];
            if (total < Least) continue;
            for (auto& layer : scratch.moved) layer[i] = 0;
            const int x = at % size, z = at / size;
            const float repose = (ReposeTan[0] * moving[0] + ReposeTan[1] * moving[1] + ReposeTan[2] * moving[2]) / total * cell;
            float excess[8], steep = 0;
            for (int k = 0; k < 8; ++k) {
                excess[k] = 0;
                if (x + NeighborX[k] < 0 || z + NeighborZ[k] < 0 || x + NeighborX[k] >= size || z + NeighborZ[k] >= size) continue;
                // Only cells the highest-first pass has still to visit can take it on.
                const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                if (height[other] >= height[i]) continue;
                // Ground that falls away at repose carries what moves over it; it stops where the fall eases.
                excess[k] = std::max(0.0f, (height[i] - height[other]) / NeighborReach[k] - Passing * repose);
                steep += excess[k];
            }
            if (steep <= 0 || domain.hold[i] != Free) {
                if (domain.hold[i] != Free) {
                    scratch.left += double(total) * area;
                    if (domain.owned(x, z)) domain.ledger.sedimentOut += double(total) * area;
                    continue;
                }
                heap(domain, height, x, z, moving, total, repose);
                continue;
            }
            for (int k = 0; k < 8; ++k) {
                if (excess[k] <= 0) continue;
                const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                for (int c = 0; c < SedimentClasses; ++c) scratch.moved[c][other] += moving[c] * excess[k] / steep;
            }
        }
        // A cone raises cells around it, so a share can still land on a cell the pass has left
        // behind. It comes to rest there as a cone of its own: nothing that moved may go missing.
        for (size_t i = 0; i < cells; ++i) {
            float moving[SedimentClasses] = {scratch.moved[0][i], scratch.moved[1][i], scratch.moved[2][i]};
            const float total = moving[0] + moving[1] + moving[2];
            if (total <= 0) continue;
            for (auto& layer : scratch.moved) layer[i] = 0;
            const int x = int(i % size_t(size)), z = int(i / size_t(size));
            if (domain.hold[i] != Free || x < 1 || z < 1 || x >= size - 1 || z >= size - 1) {
                if (domain.hold[i] == Free)
                    for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] += moving[c];
                else scratch.left += double(total) * area;
                continue;
            }
            heap(domain, height, x, z, moving, total, (ReposeTan[0] * moving[0] + ReposeTan[1] * moving[1] + ReposeTan[2] * moving[2]) / total * cell);
        }
    }
}

// Blocks and gravel that lie at the surface break down as the rock under them does (a cell does
// not record where its debris came from); what they become is sand and fines in that rock's
// proportion, which running water can carry.
void wearDebris(Domain& domain) {
    const float dt = float(domain.step);
    for (size_t i = 0; i < domain.cells(); ++i) {
        const float coarse = domain.ground.sediment[Coarse][i];
        if (domain.hold[i] != Free || coarse <= 0) continue;
        const RockTraits& rock = domain.unit[i].traits;
        const float wet = (0.3f + 0.7f * std::min(1.0f, domain.rain[i])) * (1.0f + DebrisFrost * frostShare(domain, i));
        const float worn = std::min(coarse, rock.weathering * wet * dt);
        const float finer = rock.sand + rock.fines;
        domain.ground.sediment[Coarse][i] -= worn;
        domain.ground.sediment[Sand][i] += worn * (finer > 0 ? rock.sand / finer : 1.0f);
        domain.ground.sediment[Fines][i] += worn * (finer > 0 ? rock.fines / finer : 0.0f);
    }
}

} // namespace derivean::sim
