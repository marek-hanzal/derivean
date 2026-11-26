#include "GroundwaterFixture.hpp"

#include "groundwater/fn/Sinking.hpp"
#include "groundwater/type/Sinking.hpp"
#include "landscape/fn/Build.hpp"
#include "landscape/fn/Process.hpp"
#include "measure/fn/Plane.hpp"
#include "planet/type/Planet.hpp"
#include "relief/type/Form.hpp"
#include "relief/type/Ground.hpp"

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
    FlowScratch scratch;   // of the last step: where each cell's water went
    std::vector<float> closedBefore, closedAfter;
    double sunkBefore{};   // m³/yr gone down in the step before the last
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

} // namespace

void testGroundwaterSinking() {
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
    // is all accounted for: the runoff and what rose either went down or left over a cell that
    // passes nothing on.
    double entered = 0, left = 0;
    for (size_t i = 0; i < d.cells(); ++i) {
        entered += double(d.runoff[i]) * area + work.risen[i];
        float passed = 0;
        for (int k = 0; k < 8; ++k) passed += karst.scratch.share[i * 8 + size_t(k)];
        if (passed <= 0) left += d.discharge[i];
    }
    expectWater(std::abs(entered - left - work.total) < 1e-3 * entered, "water of the plateau is lost or made", (entered - left - work.total) / entered);
    expectWater(atEdge > 0.9 * risen, "the water of the soluble rock does not come out where it meets the shale", atEdge / risen);

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
        const bool deepened = karst.closedAfter[i] > karst.closedBefore[i] + 1.0f, wet = d.level[i] > d.surface[i] + 1e-3f;
        std::printf("  hollow %d %d: closed %.1f m, after %.1f m on limestone (%s), %.1f m on sandstone\n", hollow[0], hollow[1], double(karst.closedBefore[i]),
                    double(karst.closedAfter[i]), reached ? "within the rising's reach" : drains ? "drains down" : "holds its water", double(plain.closedAfter[i]));
        if (reached) continue;
        open += drains, deeper += deepened, dry += !wet;
        filled += plain.closedAfter[i] < karst.closedBefore[i];
    }
    expectWater(open >= 4, "the hollows dug in the soluble rock do not drain down", open);
    expectWater(deeper >= 4, "hollows on soluble rock do not deepen into dolines", deeper);
    expectWater(filled >= 4, "hollows on rock that water does not open do not fill in", filled);
    expectWater(dry >= 4, "hollows on soluble rock hold lakes", dry);
    std::printf("ground water: limestone plateau after %d steps: %.0f %% of its runoff goes down, %.3g m3/yr come out at its edge; of the hollows dug %.1f m deep "
                "%d drain down, %d are dolines deeper still and %d dry, %d on sandstone filled in\n",
                Steps, 100 * work.total / runoff, atEdge, double(HollowDepth), open, deeper, dry, filled);

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
