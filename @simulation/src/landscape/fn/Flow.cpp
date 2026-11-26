#include "landscape/fn/Process.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

// Thin sheets of water on a slope spread over every lower neighbour; a river follows the steepest
// one. A cell is far wider than the rills on it, so water only gathers into a single path once it
// is the flow of many cells; `own` is what one cell yields by itself. Powers of two for speed.
int focusOf(float discharge, float own) {
    const float many = discharge / own;
    if (many < 8.0f) return 0;      // slope^1: spreading
    if (many < 40.0f) return 1;     // slope^2
    if (many < 200.0f) return 2;    // slope^4
    return 3;                       // steepest only
}

// Above ground level water that could not cut a channel of the level spreads as on any slope, and
// water that can keeps to one path. A share between two neighbours would let the one nearer a grid
// direction cut faster and draw the stream onto that direction.
int gatherOf(float gathered, float head) { return gathered < head ? 0 : 3; }

// Water running down a plane arrives at a cell as a sheet however long the slope is: one cell's
// yield for every cell of the way. That much of a cell's water cuts no channel. What it carries
// beyond the sheet has been gathered from the sides, by a hollow, a crease or a stream.
constexpr float SheetMargin = 1.6f;    // share of the sheet a cell must exceed before any of its water counts as gathered

} // namespace

void routeWater(Domain& domain, FlowScratch& scratch) {
    const size_t cells = domain.cells();
    const int size = domain.size;
    const float area = float(domain.cell * domain.cell);
    scratch.share.assign(cells * 8, 0.0f);
    for (size_t i = 0; i < cells; ++i) domain.discharge[i] = domain.runoff[i] * area + domain.inflow[i];
    const bool ground = !gathersWater(domain.key.level);
    const float head = channelHeadOf(domain.key.level), own = std::max(2.0f * head, 0.4f * area);
    // m·m³/yr: the way each cell's inflow has come since it fell as rain, summed over that inflow.
    std::vector<float>& travelled = scratch.travelled;
    if (!ground) travelled.assign(cells, 0.0f), scratch.gathered.resize(cells);
    const float sea = float(domain.seaLevel);

    // Upstream cells first: by the time a cell is visited it has received all of its water.
    for (size_t step = cells; step-- > 0;) {
        const int32_t cell = domain.order[step];
        const int x = cell % size, z = cell / size;
        const float here = domain.level[size_t(cell)];
        const int32_t rank = domain.rank[size_t(cell)];
        float slope[8];
        float steepest = 0;
        int best = -1;
        for (int k = 0; k < 8; ++k) {
            slope[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            if (domain.rank[other] >= rank || domain.level[other] >= here) continue;
            slope[k] = (here - domain.level[other]) / NeighborReach[k];
            if (slope[k] > steepest) {
                steepest = slope[k];
                best = k;
            }
        }
        // An outlet on the rim is where water leaves the domain. Passing it on along the rim would
        // make the rim a gutter that gathers every sheet crossing it into a river of the domain's own.
        if (!ground && domain.hold[size_t(cell)] == Rim && domain.outlet[size_t(cell)]) best = -1;
        if (best < 0) {
            // Where the water leaves the domain or has nowhere to go, all of it counts as gathered.
            if (!ground) scratch.gathered[size_t(cell)] = domain.discharge[size_t(cell)];
            continue;
        }
        // A finished neighbour's cell hands on exactly what that neighbour recorded passing through
        // it. Adding what reached it inside this domain would count the same river twice.
        // A ground tile's river can reach such a cell beside the course the neighbour had for it, over
        // the bank of a channel this domain has not cut yet; there the larger of the two goes on, which
        // is the recorded flow wherever both agree.
        if (domain.hold[size_t(cell)] == Settled)
            domain.discharge[size_t(cell)] = domain.key.level == Level::Ground ? std::max(domain.discharge[size_t(cell)], domain.settledFlow[size_t(cell)]) : domain.settledFlow[size_t(cell)];
        float* share = &scratch.share[size_t(cell) * 8];
        const bool standing = here > domain.surface[size_t(cell)] + 1e-3f;
        // Open water gives back to the air what the climate can take; a lake in a dry basin passes
        // on only what is left, and none at all once its surface loses as much as it receives.
        if (standing && here > sea + 0.5f) {
            const float demand = std::clamp(0.25f + 0.035f * domain.warmthNow[size_t(cell)], 0.05f, 1.6f);
            domain.discharge[size_t(cell)] = std::max(0.0f, domain.discharge[size_t(cell)] - std::max(0.0f, demand - domain.rain[size_t(cell)]) * area);
        }
        const float water = domain.discharge[size_t(cell)];
        float gathered = water, way = 0;
        if (!ground) {
            const float yield = domain.runoff[size_t(cell)] * area, arrived = water - yield - domain.inflow[size_t(cell)];
            way = arrived > 0 ? travelled[size_t(cell)] / arrived : 0.0f;
            // The water of a sheet has come, on average, half the length of the slope above the cell.
            gathered = std::max(0.0f, water - SheetMargin * yield * (2.0f * way / float(domain.cell) + 1.0f));
            scratch.gathered[size_t(cell)] = gathered;
        }
        // Through standing water the flow keeps to one path: the way to the lake's outlet.
        const int focus = standing ? 3 : ground ? focusOf(water, own) : gatherOf(gathered, head);
        if (!ground && !standing && focus > 0) {
            // Gathered water falls along the true fall line, which lies between the steepest
            // neighbour and the steeper of the two beside it: on the plane through the three cells
            // it is `turn` of the way from the one on the grid axis to the one on the diagonal. The
            // stream takes one of the two by a fixed draw per cell with those odds, so its course
            // keeps the fall line on average and prefers no direction the grid happens to offer.
            const int beside[2] = {(best + 1) & 7, (best + 7) & 7};
            const int second = slope[beside[0]] >= slope[beside[1]] ? beside[0] : beside[1];
            float toSecond = 0;
            if (slope[second] > 0) {
                const bool diagonal = best & 1;
                const float axis = slope[diagonal ? second : best], across = 1.41421356f * slope[diagonal ? best : second] - axis;
                const float turn = across <= 0 ? 0.0f : across >= axis ? 1.0f : std::atan(across / axis) * 1.27323954f;
                toSecond = diagonal ? 1 - turn : turn;
            }
            const Vec2 centre = domain.center(x, z);
            share[unit(hash(0x52495645, int64_t(centre.x / domain.cell), int64_t(centre.z / domain.cell))) < toSecond ? second : best] = 1;
        } else if (focus == 3) {
            // A grid offers eight directions; a river on a smooth slope must not lock onto one of
            // them. The choice between the two steepest falls to a fixed draw per cell, weighted by
            // slope, so the path follows the true fall line on average.
            int second = -1;
            for (int k = 0; k < 8; ++k)
                if (k != best && slope[k] > 0 && (second < 0 || slope[k] > slope[second])) second = k;
            int chosen = best;
            if (second >= 0) {
                float odds = slope[second] / steepest;
                odds *= odds;
                odds *= odds;
                const Vec2 centre = domain.center(x, z);
                if (unit(hash(0x52495645, int64_t(centre.x / domain.cell), int64_t(centre.z / domain.cell))) < odds / (1 + odds)) chosen = second;
            }
            share[chosen] = 1;
        } else {
            float total = 0;
            for (int k = 0; k < 8; ++k) {
                float weight = slope[k] / steepest;
                for (int square = 0; square < focus; ++square) weight *= weight;
                share[k] = weight;
                total += weight;
            }
            for (int k = 0; k < 8; ++k) share[k] /= total;
        }
        for (int k = 0; k < 8; ++k) {
            if (share[k] <= 0) continue;
            const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
            domain.discharge[other] += water * share[k];
            if (!ground) travelled[other] += water * share[k] * (way + NeighborReach[k] * float(domain.cell));
        }
    }
}

} // namespace derivean::sim
