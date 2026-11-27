#include "landscape/fn/Process.hpp"

#include "climate/fn/Seasons.hpp"
#include "landscape/fn/Blur.hpp"

// About 270 lines: one pass of a glacier, from where its snow gathers through what it grinds to
// where its load is laid; the flux, depth and speed fields are shared by every part of it.
namespace derivean::sim {
namespace {

constexpr float IceDepth = 2.7f;             // m of ice per (m³/yr of ice flux)^0.25 at the centre line
constexpr float IceFall = 0.035f;            // slope with which ice thins towards the valley side
constexpr float MoraineLimit = 40.0f;        // m of till one step may leave at an ice front
constexpr float MoraineTan = 0.6f;           // and no more than a heap of till stands above a cell's width (its repose is ~0.7)
constexpr float OutwashSlope = 0.02f;        // fall of a valley train below a glacier: braided outwash plains fall at about 0.5–2 %
constexpr float OutwashFreeboard = 0.02f;    // m an outwash bed stays below the water it is laid into, as a plume's does
constexpr float AvalancheTan = 0.84f;        // tan 40°: snow does not stay on steeper ground, it slides to the cell below
constexpr float SlidingErosion = 2.7e-7f;    // yr/m: bed lowering over the square of the sliding speed (Herman et al. 2015)
constexpr float GlacierHalfWidth = 250.0f;   // m over which a glacier spreads its speed across its bed
constexpr float ThickestShare = 2.0f;        // a cell's ice slides at most this many times the glacier's mean speed
constexpr float BedReach = 40.0f;            // m: half-width of the surface a glacier's bed and its snow are judged on; finer cells are averaged
// m of loose bed that deforms under sliding ice and moves with it at half its speed on average. The
// deforming layer measured under Breiðamerkurjökull is about half a metre (Boulton & Hindmarsh 1987).
constexpr float TillLayer = 0.5f;

// m/yr a glacier slides at that passes `flux` m³/yr of ice. It grows wider and thicker as it grows,
// so its speed rises slowly: Aletsch, about 8e7 m³/yr, slides at some 60 m/yr.
float slidingSpeed(float flux) { return 1.56f * std::pow(flux, 0.2f); }

} // namespace

// Ice gathers where more snow falls than melts and flows down the same fall lines as water. Snow
// does not stay on ground steeper than an avalanche slope: it passes to the cell below, so faces
// stand bare above the ice. A glacier fills its valley to a depth set by the ice it carries and
// slides over its whole bed, fastest where it is thickest; the bed is lowered with the square of
// that speed, so a trough deepens where ice gathers and its walls are left to fail, while ridges
// the ice does not cover keep their height. What the ice plucks travels with it and is dropped
// where it ends, and so does the loose cover of its bed: debris that falls into a glaciated valley
// leaves with the ice. A Site tile lies inside its parent's glacier and is under that ice in a cold
// phase. `cooling` (°C, zero or negative) selects the climate: zero for the present, the depth of a
// glacial maximum for a cold phase.
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

    // Share of a cell's ice each lower neighbour takes, by the fall of the surface; returns the
    // steepest fall as rise over run, or -1 where the cell has no lower neighbour.
    const auto shares = [&](size_t i, float* share) {
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        float total = 0, steepest = 0;
        for (int k = 0; k < 8; ++k) {
            share[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            if (domain.rank[other] >= domain.rank[i] || domain.level[other] >= domain.level[i]) continue;
            share[k] = (domain.level[i] - domain.level[other]) / NeighborReach[k];
            total += share[k];
            steepest = std::max(steepest, share[k] / cell);
        }
        for (int k = 0; k < 8; ++k) share[k] = total > 0 ? share[k] / total : 0.0f;
        return total > 0 ? steepest : -1.0f;
    };

    // A glacier and the snow that feeds it do not see ground finer than some tens of metres: below
    // that its slope is read off the surface averaged over `BedReach`. Read cell by cell on a Site
    // tile, every rib steeper than an avalanche slope would stay bare and unground while the ice
    // beside it cut down, and grow into a needle hundreds of metres high.
    const int bedRadius = int(BedReach / cell);
    std::vector<float> bed;
    if (bedRadius > 0) {
        bed = domain.level;
        scratch.blurA.assign(cells, 1.0f);
        scratch.blurB.resize(cells);
        meanOver(bed, scratch.blurA, scratch.blurB, size, bedRadius);
    }
    const auto fall = [&](size_t i, float steepest) {
        if (bed.empty()) return steepest;
        const int x = std::clamp(int(i % size_t(size)), 1, size - 2), z = std::clamp(int(i / size_t(size)), 1, size - 2);
        return std::hypot(bed[domain.index(x + 1, z)] - bed[domain.index(x - 1, z)], bed[domain.index(x, z + 1)] - bed[domain.index(x, z - 1)]) / (2 * cell);
    };

    // Upstream first: ice passes downhill until melting has used it up.
    for (size_t step = cells; step-- > 0;) {
        const size_t i = size_t(domain.order[step]);
        if (flux[i] <= 0 || domain.hold[i] == Rim) {
            flux[i] = std::max(0.0f, flux[i]);
            continue;
        }
        float share[8];
        const float steepest = shares(i, share);
        if (steepest < 0) continue;
        if (fall(i, steepest) <= AvalancheTan) depth[i] = IceDepth * std::pow(flux[i], 0.25f);
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        for (int k = 0; k < 8; ++k)
            if (share[k] > 0) flux[domain.index(x + NeighborX[k], z + NeighborZ[k])] += flux[i] * share[k];
    }

    // Ice stands level across a valley: cells beside the centre line are under ice as far as the
    // surface of their neighbour reaches, falling gently towards the side. Only ground beside or
    // above the ice is taken in, below it the glacier has its own flow; a hollow the ice has cut
    // into its own bed fills as a lake fills with water.
    for (int pass = 0; pass < 10; ++pass) {
        for (int z = 1; z < size - 1; ++z) {
            for (int x = 1; x < size - 1; ++x) {
                const size_t i = domain.index(x, z);
                float surface = domain.surface[i] + depth[i];
                for (int k = 0; k < 8; k += 2) {
                    const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                    if (depth[other] > 0 && std::max(domain.surface[i], domain.level[i]) >= domain.surface[other])
                        surface = std::max(surface, domain.surface[other] + depth[other] - IceFall * cell);
                }
                depth[i] = std::max(depth[i], surface - domain.surface[i]);
            }
        }
    }
    // A Site tile is a kilometre of a glacier kilometres long: its own snow builds some 20 m of ice
    // where its parent's trough held 300–500 m. In a cold phase the parent's ice stands over it as it
    // stood over the parent's ground, and slides as fast.
    const bool underParent = erode && !domain.parentIceTop.empty();
    if (underParent)
        for (size_t i = 0; i < cells; ++i) depth[i] = std::max(depth[i], domain.parentIceTop[i] - domain.surface[i]);
    for (size_t i = 0; i < cells; ++i) domain.ice[i] = depth[i];
    if (!erode) return;

    // The glacier's speed over every cell it covers, spread across its width, and its mean
    // thickness there: across a glacier the ice slides fastest where it is thickest.
    const int radius = std::max(1, int(std::lround(GlacierHalfWidth / cell)));
    std::vector<float>& speed = scratch.moved[3];
    std::vector<float>& thick = scratch.work;
    speed.resize(cells);
    thick.resize(cells);
    scratch.blurA.resize(cells);
    scratch.blurB.resize(cells);
    for (size_t i = 0; i < cells; ++i) {
        speed[i] = depth[i] > 0 ? std::max(slidingSpeed(flux[i]), underParent ? domain.parentIceSpeed[i] : 0.0f) : 0.0f;
        thick[i] = depth[i];
        scratch.blurA[i] = depth[i] > 0 ? 1.0f : 0.0f;
    }
    meanOver(speed, scratch.blurA, scratch.blurB, size, radius);
    for (size_t i = 0; i < cells; ++i) scratch.blurA[i] = depth[i] > 0 ? 1.0f : 0.0f;
    meanOver(thick, scratch.blurA, scratch.blurB, size, radius);
    // What a finer level reads of this glacier: how fast it slid at its greatest.
    for (size_t i = 0; i < cells; ++i)
        if (depth[i] > 0) domain.iceSpeed[i] = std::max(domain.iceSpeed[i], speed[i]);
    // On a fine tile the bed is ground as the glacier sees it: every cell inside its outline at
    // the depth of the ice around it, so a rib of rock in the bed goes down with the bed.
    // `bed` now holds that depth, `scratch.blurA` the share of the window under ice.
    if (bedRadius > 0) {
        bed = depth;
        for (size_t i = 0; i < cells; ++i) scratch.blurA[i] = depth[i] > 0 ? 1.0f : 0.0f;
        meanOver(bed, scratch.blurA, scratch.blurB, size, bedRadius);
    }
    for (size_t i = 0; i < cells; ++i) {
        const bool inside = bedRadius > 0 ? scratch.blurA[i] >= 0.5f : depth[i] > 0;
        if (!inside || domain.hold[i] != Free) continue;
        const float ice = bedRadius > 0 ? bed[i] : depth[i];
        const float sliding = speed[i] * std::min(ThickestShare, ice / std::max(1.0f, thick[i]));
        const float cut = std::min(SlidingErosion * sliding * sliding * dt, 0.5f * ice);
        const float loose = domain.sedimentAt(i);
        const float fromLoose = std::min(cut, loose);
        // The loose bed left under the ice deforms with it: a layer `TillLayer` deep moving at half
        // the sliding speed carries 0.5·u·h m² a year past each metre of the glacier's width, far more
        // than the ice grinds off rock. At these cells and steps that exceeds any cover once the ice
        // slides at all (over a kilometre a step at 1 m/yr on a Site cell), so sliding ice sweeps its
        // bed and the cover rides with the plucked rock to the ice front; only where the speed is
        // zero does it stay (no frozen bed is modelled). Valley glaciers leave little till under their trunk.
        const float carried = std::min(loose - fromLoose, 0.5f * TillLayer * sliding * dt / cell);
        const float taken = fromLoose + carried;
        for (int c = 0; c < SedimentClasses; ++c)
            if (loose > 0) domain.ground.sediment[c][i] -= taken * domain.ground.sediment[c][i] / loose;
        domain.ground.rock[i] -= cut - fromLoose;
        debris[i] += (cut + carried) * area;
        domain.eroded[i] += cut + carried;
        if (domain.owned(int(i % size_t(size)), int(i / size_t(size)))) domain.ledger.eroded += double(cut + carried) * area;
    }

    // What the ice plucked rides with it to where it ends and stays there as a moraine of unsorted
    // till. Ice at a glacier's side that the flow lines do not reach (the level fill above) hands
    // its load down the valley side to the ice that does. Beyond the front the meltwater takes what
    // a moraine cannot hold down its own course, laying it as it goes: an outwash train. The train
    // is graded as a braided river lays it: no cell rises above the lowest one whose water it takes,
    // a river a finished neighbour hands on included, nor more than `OutwashSlope` above the one it
    // drains to, nor up to the water where it stands in a lake or the sea; what does not fit goes on
    // down the valley. The load is booked where it crosses the tile's edge.
    const float moraine = std::min(MoraineLimit, MoraineTan * cell);
    std::vector<float> top(cells);
    for (size_t i = 0; i < cells; ++i) top[i] = domain.ground.rock[i] + domain.sedimentAt(i);
    int opposite[8];
    for (int k = 0; k < 8; ++k)
        for (int o = 0; o < 8; ++o)
            if (NeighborX[o] == -NeighborX[k] && NeighborZ[o] == -NeighborZ[k]) opposite[k] = o;
    const auto graded = [&](size_t i, int x, int z, const float* water) {
        float room = domain.level[i] > top[i] ? domain.level[i] - OutwashFreeboard - top[i] : 1e30f;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            const bool into = scratch.share[other * 8 + size_t(opposite[k])] > 0 || (domain.handsOnward(other) && domain.settledWay[other] == opposite[k]);
            if (into) room = std::min(room, top[other] - top[i]);
            if (water[k] > 0) room = std::min(room, top[other] + OutwashSlope * NeighborReach[k] * cell - top[i]);
        }
        return std::max(0.0f, room);
    };
    const auto nearIce = [&](int x, int z) {
        for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = std::clamp(x + dx, 0, size - 1), nz = std::clamp(z + dz, 0, size - 1);
                if (depth[domain.index(nx, nz)] > 0) return true;
            }
        return false;
    };
    for (size_t step = cells; step-- > 0;) {
        const size_t i = size_t(domain.order[step]);
        if (debris[i] <= 0) continue;
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        float share[8];
        const bool rides = flux[i] > 0 && domain.hold[i] != Rim && shares(i, share) >= 0;
        if (!rides) {
            const float* water = &scratch.share[i * 8];
            for (int k = 0; k < 8; ++k) share[k] = domain.hold[i] == Rim ? 0.0f : water[k];
        }
        float passed = 0;
        for (int k = 0; k < 8; ++k) passed += share[k];
        if (domain.hold[i] != Free) {
            // Ground a finished neighbour holds, or the rim: the load leaves the domain here.
            scratch.left += double(debris[i]);
            continue;
        }
        const bool owned = domain.owned(x, z);
        // At the front the till is heaped as one step builds a moraine; beyond it the meltwater
        // lays what it brought as far as the grade has room.
        const bool laid = !rides && !(depth[i] > 0 && passed > 0);
        const float till = !laid ? 0.0f : std::min(debris[i] / area, nearIce(x, z) ? moraine : graded(i, x, z, &scratch.share[i * 8]));
        if (till > 0) {
            top[i] += till;
            domain.ground.sediment[Coarse][i] += 0.5f * till;
            domain.ground.sediment[Sand][i] += 0.2f * till;
            domain.ground.sediment[Fines][i] += 0.3f * till;
            domain.deposited[i] += till;
            if (owned) domain.ledger.deposited += double(till) * area;
        }
        const float rest = debris[i] - till * area;
        if (rest <= 0) continue;
        if (passed <= 0) {
            // Water that goes nowhere further in the domain, the sea or a domain outlet, takes the rest away.
            scratch.left += double(rest);
            if (owned) domain.ledger.sedimentOut += double(rest);
            continue;
        }
        for (int k = 0; k < 8; ++k) {
            if (share[k] <= 0) continue;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            const float onward = rest * share[k] / passed;
            debris[domain.index(nx, nz)] += onward;
            if (owned != domain.owned(nx, nz)) (owned ? domain.ledger.sedimentOut : domain.ledger.sedimentIn) += double(onward);
        }
    }
}

} // namespace derivean::sim
