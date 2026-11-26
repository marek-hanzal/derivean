#include "GroundwaterFixture.hpp"

#include "groundwater/fn/Sinking.hpp"
#include "groundwater/fn/Spill.hpp"
#include "groundwater/type/Sinking.hpp"
#include "landscape/fn/Build.hpp"
#include "landscape/fn/Process.hpp"
#include "measure/fn/Plane.hpp"
#include "planet/type/Planet.hpp"
#include "relief/type/Form.hpp"
#include "relief/type/Ground.hpp"

#include <stdexcept>
#include <cstdio>
#include <queue>

using namespace derivean::sim;

namespace {

// A plateau falling gently east, its rock ending in shale near the foot, with six hollows dug in
// it; the same plateau of sandstone, which water does not open, for comparison.
constexpr int ShaleFrom = 250;                              // cells along the fall line where the soluble rock ends
constexpr float HollowDepth = 8.0f, HollowRadius = 5.0f;    // m, cells
constexpr int Hollows = 6;
constexpr int HollowAt[Hollows][2] = {{70, 90}, {110, 150}, {150, 70}, {190, 200}, {130, 240}, {230, 120}};
constexpr int Steps = 40;

// m each cell lies below the level a flood from the rim fills it to.
std::vector<float> closedDepth(const Domain& d) {
    std::vector<float> fill(d.cells(), 0.0f);
    std::vector<uint8_t> seen(d.cells(), 0);
    using Item = std::pair<float, int32_t>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
    const auto ground = [&](size_t i) { return d.ground.rock[i] + d.sedimentAt(i); };
    for (int z = 0; z < d.size; ++z)
        for (int x = 0; x < d.size; ++x)
            if (x == 0 || z == 0 || x == d.size - 1 || z == d.size - 1) {
                const size_t i = d.index(x, z);
                fill[i] = ground(i), seen[i] = 1, open.push({fill[i], int32_t(i)});
            }
    while (!open.empty()) {
        const auto [level, cell] = open.top();
        open.pop();
        for (int k = 0; k < 8; ++k) {
            const int x = cell % d.size + NeighborX[k], z = cell / d.size + NeighborZ[k];
            if (x < 0 || z < 0 || x >= d.size || z >= d.size || seen[d.index(x, z)]) continue;
            const size_t o = d.index(x, z);
            seen[o] = 1, fill[o] = std::max(level, ground(o)), open.push({fill[o], int32_t(o)});
        }
    }
    for (size_t i = 0; i < d.cells(); ++i) fill[i] -= ground(i);
    return fill;
}

struct Plateau {
    Domain domain;
    FlowScratch scratch; // of the last step: where each cell's water went
    std::vector<float> closedBefore, closedAfter;
    double sunkBefore{}; // m³/yr gone down in the step before the last
};

Plateau replayPlateau(Material rock) {
    PlaneSpec spec;
    spec.level = Level::Local, spec.rock = rock, spec.slope = 0.004, spec.roughness = 1.5, spec.rain = 1.0, spec.steps = Steps;
    Plateau plateau{planeDomain(spec)};
    Domain& d = plateau.domain;
    const UnitCache shale{-1e30f, 1e30f, traitsOf(Material::Shale)};
    for (int z = 0; z < d.size; ++z)
        for (int x = 0; x < d.size; ++x) {
            const size_t i = d.index(x, z);
            if (x >= ShaleFrom) d.unit[i] = shale;
            for (const auto& hollow : HollowAt)
                d.ground.rock[i] -= HollowDepth * std::exp(-float((x - hollow[0]) * (x - hollow[0]) + (z - hollow[1]) * (z - hollow[1])) / (HollowRadius * HollowRadius));
            d.ground.datum[i] = d.ground.rock[i];
        }
    d.settledFrom = d.settledTo = d.ground;
    plateau.closedBefore = closedDepth(d);
    // stepDomain asks the planet for the sea level and its date and nothing else.
    Planet planet;
    planet.seaLevel = d.seaLevel, planet.time = d.endTime;
    Backdrop backdrop;
    const Setting setting{spec.seed, d.endTime, &planet, &backdrop};
    FlowScratch& scratch = plateau.scratch;
    for (int step = 0; step < Steps; ++step) {
        if (step == Steps - 1) plateau.sunkBefore = d.sinking.total;
        stepDomain(d, scratch, setting);
    }
    plateau.closedAfter = closedDepth(d);
    return plateau;
}

// What a route did with the water of a domain on soluble rock: what entered (own runoff, inflow,
// risen), what left over the rim or the sea, went down, ran over the edge or into a finished
// neighbour, or soaked into a dry bed.
struct Routed {
    double entered{}, left{}, sunk{}, overEdge{}, seeped{}, spilled{};
    size_t lakes{};
    double residual() const { return (entered - left - sunk - overEdge - seeped) / entered; }
};

Routed routedOf(const Domain& d, const FlowScratch& scratch) {
    Routed r;
    const float area = float(d.cell * d.cell);
    for (size_t i = 0; i < d.cells(); ++i) {
        r.entered += double(d.runoff[i]) * area + d.sinking.risen[i] + d.inflow[i];
        float passed = 0;
        for (int k = 0; k < 8; ++k) passed += scratch.share[i * 8 + size_t(k)];
        if (passed <= 0 && d.hold[i] != Free) r.left += d.discharge[i];
        if (!d.seeped.empty()) r.seeped += d.seeped[i];
    }
    r.sunk = d.sinking.total, r.overEdge = d.sinking.overEdge, r.spilled = d.sinking.spilled, r.lakes = d.sinking.lakes.size();
    return r;
}

// A 7 × 7 domain falling from every side into a hollow at its middle whose floor takes 1 m³/yr and
// was offered 10 000; its lowest pass leads east to a cell that is `beyond`. With `walled` that cell
// stands high, and the lowest way out is the domain's edge.
Domain bowl(Hold beyond, bool walled) {
    Domain d;
    d.size = 7, d.cell = 10;
    const size_t cells = 49, middle = 3 * 7 + 3, pass = 3 * 7 + 6;
    d.surface.assign(cells, 0.0f), d.discharge.assign(cells, 0.0f), d.hold.assign(cells, Free), d.rank.assign(cells, 0);
    for (int z = 0; z < 7; ++z)
        for (int x = 0; x < 7; ++x) d.surface[size_t(z * 7 + x)] = 10.0f + 0.5f * float(std::abs(x - 3) + std::abs(z - 3));
    d.surface[pass] = walled ? 20.0f : 9.0f, d.hold[pass] = beyond;
    for (size_t i = 0; i < cells; ++i) d.rank[i] = 100 - int(d.surface[i] * 4);
    SinkWork& w = d.sinking;
    w.rate.assign(cells, 0.01f), w.received.assign(cells, -1.0f), w.sunk.assign(cells, 0.0f), w.ponded.assign(cells, 0.0f);
    w.received[middle] = 1e4f, w.ponded[middle] = 1e4f - 1.0f;
    return d;
}

std::vector<float> bowlShares() {
    std::vector<float> share(49 * 8, 0.0f);
    for (int z = 0; z < 7; ++z)
        for (int x = 0; x < 7; ++x) {
            if ((x == 3 && z == 3) || (x == 6 && z == 3)) continue;
            const int dx = x < 3 ? 1 : x > 3 ? -1 : 0, dz = z < 3 ? 1 : z > 3 ? -1 : 0;
            for (int k = 0; k < 8; ++k)
                if (NeighborX[k] == dx && NeighborZ[k] == dz) share[size_t(z * 7 + x) * 8 + size_t(k)] = 1;
        }
    return share;
}

// Water a hollow cannot take runs over its lowest pass: on to the cell beyond, out of the domain
// where that cell is a finished neighbour's, or over the domain's edge where that is lower.
void testSpillByHand() {
    const std::vector<float> share = bowlShares();
    const size_t pass = 3 * 7 + 6;
    Domain onward = bowl(Rim, false), finished = bowl(Settled, false), edge = bowl(Rim, true);
    std::vector<float> gathered(49, 0.0f);
    spillHollows(onward, share, &gathered, QueueStop{0, 1});
    spillHollows(finished, share, nullptr);
    // Spill directly to the edge with no visits. An empty queue is success at the limit.
    spillHollows(edge, share, nullptr, QueueStop{0, 0});
    Domain limited = bowl(Rim, false);
    bool failed = false;
    try {
        spillHollows(limited, share, nullptr, QueueStop{0, 0});
    } catch (const std::logic_error&) {
        failed = true;
    }
    expectWater(failed, "spill returned a result with work still queued");
    expectWater(onward.sinking.spilled == 9999 && onward.discharge[pass] == 9999 && gathered[pass] == 9999 && onward.sinking.overEdge == 0, "a hollow's overflow does not reach the cell beyond its pass", onward.discharge[pass]);
    expectWater(finished.sinking.overEdge == 9999 && finished.discharge[pass] == 0, "overflow into a finished neighbour's cell stays in the domain", finished.sinking.overEdge);
    expectWater(edge.sinking.overEdge == 9999 && edge.discharge[pass] == 0, "a catchment whose lowest way out is the domain's edge does not run over it", edge.sinking.overEdge);
}

} // namespace

void testGroundwaterSinking() {
    testSpillByHand();
    const Plateau karst = replayPlateau(Material::Limestone), plain = replayPlateau(Material::Sandstone);
    const Domain& d = karst.domain;
    expectWater(d.sinking.active() && !plain.domain.sinking.active(), "only soluble rock takes in surface water");
    const SinkWork& work = d.sinking;
    const float area = float(d.cell * d.cell);

    // Streams sink: most of what runs off the soluble rock goes down, and the water that went down
    // in the step before comes out, all of it, at the edge of the soluble rock.
    double runoff = 0, risen = 0, atEdge = 0;
    int open = 0;
    for (int z = 0; z < d.size; ++z)
        for (int x = 0; x < d.size; ++x) {
            const size_t i = d.index(x, z);
            risen += work.risen[i];
            if (work.risen[i] > 0 && x == ShaleFrom - 1) atEdge += work.risen[i];
            if (d.hold[i] != Free || x >= ShaleFrom) continue;
            runoff += d.runoff[i] * area;
        }
    expectWater(work.total > 0.5 * runoff, "streams on soluble rock do not go down", work.total / runoff);
    expectWater(work.total < runoff, "water that came out at the rising goes down again", work.total / runoff);
    expectWater(std::abs(risen - karst.sunkBefore) < 1e-3 * karst.sunkBefore, "water that went down does not all come out", (risen - karst.sunkBefore) / karst.sunkBefore);
    // Nothing evaporates here (rain exceeds what open water gives back), so the last step's water
    // is all accounted for: the runoff and what rose either went down, left over the rim, or
    // reached an open hollow that could not take it and leaves the step's surface water there.
    double entered = 0, left = 0, ponded = 0;
    for (size_t i = 0; i < d.cells(); ++i) {
        entered += double(d.runoff[i]) * area + work.risen[i];
        float passed = 0;
        for (int k = 0; k < 8; ++k) passed += karst.scratch.share[i * 8 + size_t(k)];
        if (passed <= 0 && d.hold[i] != Free) left += d.discharge[i];
        ponded += work.ponded[i];
    }
    // What open hollows could not take ran over their passes and left over the rim with the rest.
    expectWater(std::abs(entered - left - work.total - work.overEdge) < 1e-3 * entered, "water of the plateau is lost or made", (entered - left - work.total - work.overEdge) / entered);
    expectWater(ponded < 0.01 * work.total, "open hollows are offered much more than their floors take", ponded / std::max(1.0, work.total));
    expectWater(atEdge > 0.9 * risen, "the water of the soluble rock does not come out where it meets the shale", atEdge / risen);
    // Floors that take a tenth and a hundredth as much leave hollows overflowing: their water runs
    // over the passes, lakes join, and none is lost or made. A route run again after the step, as
    // the coast's replay does, books the same water once.
    for (const float slower : {0.1f, 0.01f}) {
        Domain slow = d;
        for (float& rate : slow.sinking.rate) rate *= slower;
        for (float& soak : slow.sinking.soak) soak *= slower;
        FlowScratch scratch;
        floodLevels(slow, scratch);
        routeWater(slow, scratch);
        const Routed once = routedOf(slow, scratch);
        routeWater(slow, scratch);
        const Routed again = routedOf(slow, scratch);
        std::printf("ground water: floors taking %.2f of their rate: %.0f %% of the water runs over passes of %zu lakes, %.2g left unaccounted\n", double(slower), 100 * once.spilled / once.entered, once.lakes, once.residual());
        expectWater(std::abs(once.residual()) < 1e-4, "water that runs over the passes is lost or made", once.residual());
        expectWater(once.spilled > 0.005 * once.entered && once.lakes > 20, "overflowing hollows spill little", once.spilled / once.entered);
        expectWater(std::abs(again.left - once.left) <= 1e-6 * once.entered && std::abs(again.sunk - once.sunk) <= 1e-6 * once.entered && again.overEdge == once.overEdge,
                    "a route run twice books its water twice", (again.left - once.left) / once.entered);
    }
    // No floor takes in more than it can: its rate over the cell, and the share of its own runoff
    // that soaks in where it falls. Over a replay what sinks then stays within what fell and rose.
    double over = 0, most = 0;
    for (size_t i = 0; i < d.cells(); ++i) {
        const double can = double(work.rate[i]) * area + double(work.soak[i]) * d.runoff[i] * area;
        over = std::max(over, double(work.sunk[i]) - can - 1e-3 * can), most = std::max(most, double(work.sunk[i]) / std::max(1.0, can));
    }
    expectWater(over <= 0, "a floor takes in more water than it can", most);

    // Closed hollows drain down instead of holding lakes, and the water they gather dissolves their
    // floors: they deepen where those on rock that water does not open fill in.
    int deeper = 0, filled = 0, dry = 0;
    for (const auto& hollow : HollowAt) {
        const size_t i = d.index(hollow[0], hollow[1]);
        // A hollow near the foot lies within the reach of the rising: water stands in it.
        const bool reached = work.rising[i] < 0 || d.surface[i] < d.surface[size_t(work.rising[i])] + SinkRisingZone;
        // Its water goes down in an open hollow somewhere on its floor.
        bool drains = false;
        for (int dz = -int(HollowRadius); dz <= int(HollowRadius); ++dz)
            for (int dx = -int(HollowRadius); dx <= int(HollowRadius); ++dx) {
                const size_t at = d.index(hollow[0] + dx, hollow[1] + dz);
                drains = drains || (drainsDown(d, at) && work.sunk[at] > 0);
            }
        const bool deepened = karst.closedAfter[i] > karst.closedBefore[i] + 0.1f, wet = d.level[i] > d.surface[i] + 1e-3f;
        std::printf("  hollow %d %d: closed %.1f m, after %.1f m on limestone (%s), %.1f m on sandstone\n", hollow[0], hollow[1], double(karst.closedBefore[i]),
                    double(karst.closedAfter[i]), reached ? "within the rising's reach" : drains ? "drains down" : "holds its water", double(plain.closedAfter[i]));
        // A doline may deepen down into its rising's reach; it deepened all the same.
        deeper += deepened;
        if (reached) continue;
        open += drains, dry += !wet;
        filled += plain.closedAfter[i] < karst.closedBefore[i];
    }
    expectWater(open >= 4, "the hollows dug in the soluble rock do not drain down", open);
    // A floor takes in no more than its rate, so how far a doline deepens in 40 steps varies with
    // what reaches it (up to 13 m here); that it deepens, by more than float noise or moved cover,
    // where sandstone fills is the check.
    expectWater(deeper >= 4, "hollows on soluble rock do not deepen into dolines", deeper);
    expectWater(filled >= 4, "hollows on rock that water does not open do not fill in", filled);
    expectWater(dry >= 4, "hollows on soluble rock hold lakes", dry);
    std::printf("ground water: limestone plateau after %d steps: %.0f %% of its runoff goes down, %.3g m3/yr come out at its edge, %.2g m3/yr ponded in open hollows; of the hollows dug %.1f m deep "
                "%d drain down, %d are dolines deeper still and %d dry, %d on sandstone filled in\n",
                Steps, 100 * work.total / runoff, atEdge, ponded, double(HollowDepth), open, deeper, dry, filled);

    // Under a lake the parent holds, solution stops at the parent's floor: the last step's sinking
    // dissolved once more, with that floor a millimetre under every cell, and once with none.
    Domain free = d, held = d;
    free.lakeFloor.clear();
    held.lakeFloor.resize(d.cells());
    for (size_t i = 0; i < d.cells(); ++i) held.lakeFloor[i] = d.ground.rock[i] + d.sedimentAt(i) - 1e-3f;
    dissolveSinks(free), dissolveSinks(held);
    float freely = 0, under = 0;
    for (size_t i = 0; i < d.cells(); ++i) {
        freely = std::max(freely, d.ground.rock[i] - free.ground.rock[i]);
        under = std::max(under, held.lakeFloor[i] - (held.ground.rock[i] + held.sedimentAt(i)));
    }
    expectWater(freely > 1e-2f, "a step's sinking dissolves nothing to hold back", freely);
    expectWater(under < 1e-5f, "karst lowers a floor below the parent's lake floor", under);
    std::printf("ground water: a step's sinking lowers floors up to %.2f m, and none below the parent's lake floor\n", double(freely));
}

// Bare limestone opens into grikes along its joints; under a cover of soil, or where the ground
// sheds no water, it stays whole.
void testGroundwaterPavement() {
    const auto lattice = [](float cover) {
        GroundLattice ground;
        ground.countX = ground.countZ = 200;
        const size_t nodes = 200 * 200;
        ground.rock.assign(nodes, 100.0f), ground.datum.assign(nodes, 100.0f), ground.lie.assign(nodes, 100.0f), ground.crust.assign(nodes, 0.0f);
        for (auto& layer : ground.cover) layer.assign(nodes, 0.0f);
        ground.cover[Fines].assign(nodes, cover);
        ground.crustKind.assign(nodes, uint8_t(Crust::None)), ground.coarseKind.assign(nodes, uint8_t(Material::Gravel)), ground.fineKind.assign(nodes, uint8_t(Material::Silt));
        ground.bare.assign(nodes, 0);
        return ground;
    };
    FormSetting setting;
    setting.seed = 42;
    setting.strata.now = 1e9;
    setting.strata.thickness[0] = setting.strata.coverTotal = 500;
    setting.strata.facies[0] = Facies{0, 0, 40, 215, 0, 0};
    const GroundLattice bare = lattice(0), covered = lattice(0.5f);
    constexpr int Count = 24;
    int opened[3] = {}, columns = 0;
    float deepest = 0;
    for (double x = 10; x < 40; x += 0.25)
        for (double z = 10; z < 40; z += 1.0) {
            ++columns;
            for (int pass = 0; pass < 3; ++pass) {
                FormSetting asked = setting;
                asked.runoff = pass == 2 ? 0.0f : 0.6f;
                Material column[Count];
                formColumn(asked, pass == 1 ? covered : bare, 0, nullptr, x, z, 94, 0.25, Count, column, 1);
                // The rock surface lies at 100 m: air or cover below it is a grike.
                float open = 0;
                for (int k = 0; k < Count; ++k) {
                    const float y = float(94 + (k + 0.5) * 0.25);
                    if (y < 100 && (column[k] == Material::Air)) open = std::max(open, 100 - y);
                }
                opened[pass] += open > 0;
                if (pass == 0) deepest = std::max(deepest, open);
            }
        }
    const double share = double(opened[0]) / columns;
    expectWater(share > 0.03 && share < 0.4, "bare limestone does not open into grikes on a share of its ground", share);
    expectWater(deepest > 0.5f, "grikes in bare limestone are shallow", deepest);
    expectWater(opened[1] == 0 && opened[2] == 0, "limestone opens into grikes under soil or where no water runs", opened[1] + opened[2]);
    std::printf("ground water: bare limestone shedding 0.6 m/yr is grike over %.0f %% of its columns, up to %.1f m deep; none under 0.5 m of soil or without water\n", 100 * share,
                double(deepest));
}
