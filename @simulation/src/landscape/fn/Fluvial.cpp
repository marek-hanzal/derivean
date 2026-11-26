#include "landscape/fn/Process.hpp"

// About 320 lines: the erosion factor, the implicit solve and the carrying of the load are three
// sweeps over one flow order that share one working state and are only meaningful together.
namespace derivean::sim {
namespace {

constexpr int Passes = 2;            // implicit solves; each refines the deposition of the one before
constexpr float FreeFall = 1.19f;    // rise over run (50°) above which water leaves the bed and falls

struct Sweep {
    Domain& domain;
    FlowScratch& scratch;
    float dt, cell, area;

    // Water stood on the cell when the step began, and still does: a lake whose outlet is cut
    // below a cell within the step leaves that cell to the river that now crosses it.
    bool standing(size_t i) const { return domain.level[i] > domain.surface[i] + 1e-3f; }
    bool flooded(size_t i) const { return standing(i) && !scratch.drained[i]; }
};

// Implicit erosion factors of every cell, K·dt·(Q^m - Q0^m)/cell: one for its rock and one for the
// loose cover on it. At ground level they are zero where water stands; the coarser levels keep them
// there for the case that the lake drains within the step.
void measurePower(Sweep& sweep) {
    Domain& domain = sweep.domain;
    // `ground` here is every level that cuts with all the water of a cell (Process.hpp, gathersWater).
    const bool ground = !gathersWater(domain.key.level), drains = domain.key.level != Level::Ground;
    const float head = std::pow(channelHeadOf(domain.key.level), float(StreamExponent));
    const float hidden = HiddenStreams[size_t(domain.key.level)];
    // Within the width of a river every cell of the bed is worked by the whole river.
    const std::vector<float>& working = domain.stream.empty() ? domain.discharge : domain.stream;
    // Every cell by itself: rows may be worked side by side.
    forRows(domain, domain.size, [&](int begin, int end) {
    for (size_t i = size_t(begin) * size_t(domain.size); i < size_t(end) * size_t(domain.size); ++i) {
        float rock = 0, cover = 0;
        if (domain.hold[i] != Settled && (drains ? !(sweep.standing(i) && domain.hold[i] == Rim) : !sweep.flooded(i))) {
            // Where a level cuts with all of a cell's water, it cuts once that is more than a channel
            // head's worth. The coarser levels cut with the water a cell has gathered, not with the
            // sheet that any long slope carries. A cell that has gathered less than a channel's worth
            // still holds streams of its own inside it, the largest about `hidden` or all the water the
            // cell has, where that is less: its ground is lowered at the pace such a stream cuts on the
            // cell's slope. Ground without water is not lowered.
            const float wash = ground ? std::max(0.0f, std::pow(working[i], float(StreamExponent)) - head)
                                      : std::pow(std::max(sweep.scratch.gathered[i], std::min(hidden, domain.discharge[i])), float(StreamExponent));
            if (wash > 0) {
                // What is about to settle here is as easy to pick up again as what already lies here.
                // Storms count for loose cover only: rock is cut by what runs over it year in, year out.
                float loose = 0, soft = 0;
                for (int c = 0; c < SedimentClasses; ++c) {
                    const float layer = domain.ground.sediment[c][i] + sweep.scratch.settle[c][i];
                    loose += layer;
                    soft += SedimentErodibility[c] * layer;
                }
                rock = domain.unit[i].traits.erodibility * domain.quality[i] * (1.0f + 2.5f * domain.weakness[i]) * wash * rockCutOf(domain.key.level) * sweep.dt / sweep.cell;
                if (loose > 0) cover = soft / loose * wash * stormGainOf(domain.key.level, working[i]) * sweep.dt / sweep.cell;
                if (drains) {
                    // Water wears a bed in proportion to its fall only while it runs on the bed. Down
                    // a face too steep for debris to lie on it falls free, and wears the rock no faster
                    // than it would at that limit: walls are the weather's and their own weight's.
                    const float* share = &sweep.scratch.share[i * 8];
                    const int x = int(i % size_t(domain.size)), z = int(i / size_t(domain.size));
                    float fall = 0;
                    for (int k = 0; k < 8; ++k)
                        if (share[k] > 0) fall += share[k] * (domain.surface[i] - domain.surface[domain.index(x + NeighborX[k], z + NeighborZ[k])]) / NeighborReach[k];
                    if (fall > FreeFall * sweep.cell) rock *= FreeFall * sweep.cell / fall, cover *= FreeFall * sweep.cell / fall;
                }
            }
        }
        sweep.scratch.power[i] = rock;
        sweep.scratch.coverPower[i] = cover;
    }
    });
}

// m a cell under `loose` m of cover is lowered in the step. `cutBy(power, below)` is what an erosion
// factor takes off a surface that starts `below` m under the cell's top. The cover goes first, at
// its own pace, and the rock is cut in what is left of the step: a mantle never lends the rock under
// it its own erodibility.
template<typename Cut>
float lowering(float loose, float coverPower, float rockPower, Cut&& cutBy) {
    if (loose <= 0) return cutBy(rockPower, 0.0f);
    const float throughCover = cutBy(coverPower, 0.0f);
    return throughCover <= loose ? throughCover : loose + cutBy(rockPower * (1 - loose / throughCover), loose);
}

// Outlets first: each cell is solved against the already updated heights of the cells it drains to.
void solveHeights(Sweep& sweep) {
    Domain& domain = sweep.domain;
    FlowScratch& scratch = sweep.scratch;
    const int size = domain.size;
    const bool drains = domain.key.level != Level::Ground;
    for (const int32_t cell : domain.order) {
        const size_t i = size_t(cell);
        const float before = domain.surface[i];
        if (domain.hold[i] == Settled) {
            scratch.next[i] = before;
            scratch.pool[i] = std::max(before, domain.level[i]);
            continue;
        }
        if (domain.hold[i] == Rim) {
            // Only an outlet cuts down; the rest of the rim is a divide that rises with its rock.
            const float fall = sweep.cell * domain.rimSlope[i];
            scratch.next[i] = !domain.outlet[i] ? before
                : std::max(std::min(before, domain.rimFloor[i]), before - lowering(domain.sedimentAt(i), scratch.coverPower[i], scratch.power[i], [&](float power, float) { return power * fall; }));
            scratch.pool[i] = sweep.standing(i) ? domain.level[i] : scratch.next[i];
            continue;
        }
        const float laid = scratch.settle[0][i] + scratch.settle[1][i] + scratch.settle[2][i];
        const float* share = &scratch.share[i * 8];
        const int x = cell % size, z = cell / size;
        if (sweep.standing(i) && !scratch.drained[i]) {
            // Under standing water nothing is cut; what arrives settles from the plume afterwards.
            // Above ground level a lake is only as lasting as its outlet: once the cell this one
            // drains to has been cut below it, the water has left and the river works its floor.
            float held = domain.level[i];
            if (drains)
                for (int k = 0; k < 8; ++k)
                    if (share[k] > 0) held = std::min(held, scratch.pool[domain.index(x + NeighborX[k], z + NeighborZ[k])]);
            if (held >= before - 1e-3f) {
                scratch.next[i] = before;
                scratch.pool[i] = std::max(held, before);
                continue;
            }
            scratch.drained[i] = 1;
        }
        const float raised = before + laid;
        if (scratch.power[i] <= 0 && scratch.coverPower[i] <= 0) {
            scratch.next[i] = scratch.pool[i] = raised;
            continue;
        }
        float pull = 0, weighted = 0;
        for (int k = 0; k < 8; ++k) {
            if (share[k] <= 0) continue;
            pull += share[k] / NeighborReach[k];
            weighted += share[k] / NeighborReach[k] * scratch.next[domain.index(x + NeighborX[k], z + NeighborZ[k])];
        }
        // The implicit step against the cells below, of a surface with that erosion factor.
        const auto cutBy = [&](float power, float below) {
            const float top = raised - below;
            return std::max(0.0f, top - (top + power * weighted) / (1 + power * pull));
        };
        scratch.next[i] = scratch.pool[i] = raised - lowering(domain.sedimentAt(i) + laid, scratch.coverPower[i], scratch.power[i], cutBy);
    }
}

// Headwaters first: what a cell loses joins the load, what settles leaves it. On the last pass the
// result is written into the ground, so every metre removed or laid is counted exactly once.
void carryLoad(Sweep& sweep, bool final) {
    Domain& domain = sweep.domain;
    FlowScratch& scratch = sweep.scratch;
    const int size = domain.size;
    const float perYear = sweep.area / sweep.dt;
    for (auto& flux : scratch.flux) std::fill(flux.begin(), flux.end(), 0.0f);
    std::vector<float> fresh[SedimentClasses];
    for (auto& layer : fresh) layer.assign(domain.cells(), 0.0f);

    for (size_t step = domain.cells(); step-- > 0;) {
        const int32_t cell = domain.order[step];
        const size_t i = size_t(cell);
        const int x = cell % size, z = cell / size;
        float load[SedimentClasses] = {scratch.flux[0][i], scratch.flux[1][i], scratch.flux[2][i]};
        const bool owned = domain.owned(x, z);
        // Rock that broke off inside a Region cell this step leaves with the cell's water: it is
        // erosion of the cell like what the water itself cut, and the plate answers it the same way.
        float shed = 0;
        if (!scratch.delivered[0].empty())
            for (int c = 0; c < SedimentClasses; ++c) load[c] += scratch.delivered[c][i] * perYear, shed += scratch.delivered[c][i];
        if (domain.inflowLoad[i] > 0) {
            load[Coarse] += 0.15f * domain.inflowLoad[i];
            load[Sand] += 0.45f * domain.inflowLoad[i];
            load[Fines] += 0.40f * domain.inflowLoad[i];
        }
        if (domain.hold[i] == Settled && (domain.key.level != Level::Ground || load[0] + load[1] + load[2] < domain.settledLoad[i])) {
            // As with its water, a finished neighbour's cell passes on the load recorded there; on the
            // ground level the load that arrived, where a river reached the cell with more.
            load[Coarse] = 0.15f * domain.settledLoad[i];
            load[Sand] = 0.45f * domain.settledLoad[i];
            load[Fines] = 0.40f * domain.settledLoad[i];
        }

        if (domain.hold[i] != Settled) {
            float laid[SedimentClasses] = {scratch.settle[0][i], scratch.settle[1][i], scratch.settle[2][i]};
            float laidTotal = laid[0] + laid[1] + laid[2];
            const float planned = laidTotal;
            if (sweep.flooded(i) && laidTotal > 0) {
                const float room = std::max(0.0f, scratch.next[i] - domain.surface[i]);
                for (float& part : laid) part *= room / laidTotal;
                laidTotal = room;
            }
            // What the water takes in this step it takes first from what it was about to lay here:
            // only the net of the two changes the ground. Booking them separately would let a river
            // that merely passes its load on dig into rock under a blanket of its own deposit.
            float cut = std::max(0.0f, domain.surface[i] + (sweep.flooded(i) ? laidTotal : planned) - scratch.next[i]);
            if (laidTotal > 0) {
                const float kept = std::max(0.0f, laidTotal - cut) / laidTotal;
                cut = std::max(0.0f, cut - laidTotal);
                for (float& part : laid) part *= kept;
                laidTotal *= kept;
            }
            const float loose = domain.sedimentAt(i);
            const float fromLoose = std::min(cut, loose);
            const float fromRock = cut - fromLoose;
            const RockTraits& rock = domain.unit[i].traits;
            float source[SedimentClasses] = {fromRock * rock.coarse, fromRock * rock.sand, fromRock * rock.fines};
            float taken[SedimentClasses] = {};
            if (loose > 0)
                for (int c = 0; c < SedimentClasses; ++c) source[c] += taken[c] = fromLoose * domain.ground.sediment[c][i] / loose;

            float shortfall = 0;
            for (int c = 0; c < SedimentClasses; ++c) {
                load[c] += (source[c] - laid[c]) * perYear;
                if (load[c] < 0) {
                    // More was planned to settle than arrived; only what arrived can stay.
                    const float missing = -load[c] / perYear;
                    laid[c] -= missing;
                    shortfall += missing;
                    load[c] = 0;
                }
            }
            if (final) {
                for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] += laid[c] - taken[c];
                domain.ground.rock[i] -= fromRock;
                const float settled = laidTotal - shortfall;
                domain.erosionRate[i] += 0.4f * ((cut + shed) / sweep.dt - domain.erosionRate[i]);
                domain.depositionRate[i] += 0.4f * (settled / sweep.dt - domain.depositionRate[i]);
                domain.eroded[i] += cut + shed;
                domain.deposited[i] += settled;
                if (owned) {
                    domain.ledger.eroded += double(cut + shed) * sweep.area;
                    domain.ledger.deposited += double(settled) * sweep.area;
                    domain.ledger.dissolved += double(fromRock * rock.solute) * sweep.area;
                }
            }
            // Bedload grinds itself down on the way.
            const float worn = load[Coarse] * std::min(0.9f, BedloadWear * sweep.cell);
            load[Coarse] -= worn;
            load[Sand] += 0.6f * worn;
            load[Fines] += 0.4f * worn;
            // Share of the passing load that settles here next pass: transport length shrinks with
            // grain size and collapses where the water stands.
            const float water = std::max(domain.discharge[i], 1.0f);
            if (domain.hold[i] == Free) {
                float planned = 0;
                for (int c = 0; c < SedimentClasses; ++c) {
                    fresh[c][i] = sweep.flooded(i) ? 0.0f : std::min(SettlingNumber[c] * load[c] / water * sweep.dt, load[c] / perYear);
                    planned += fresh[c][i];
                }
                // A bed may not rise above the cells that drain into it: that would dam them.
                const float room = 0.8f * scratch.room[i];
                if (planned > room)
                    for (int c = 0; c < SedimentClasses; ++c) fresh[c][i] *= room / planned;
            }
        }

        if (sweep.flooded(i) && domain.hold[i] != Rim) {
            if (final)
                for (int c = 0; c < SedimentClasses; ++c) scratch.plume[c][i] += load[c] * sweep.dt;
            continue;
        }
        const float* share = &scratch.share[i * 8];
        bool passed = false;
        for (int k = 0; k < 8; ++k) {
            if (share[k] <= 0) continue;
            passed = true;
            const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
            for (int c = 0; c < SedimentClasses; ++c) scratch.flux[c][other] += load[c] * share[k];
        }
        if (!passed && final) domain.ledger.sedimentOut += double(load[0] + load[1] + load[2]) * sweep.dt;
    }
    for (int c = 0; c < SedimentClasses; ++c) scratch.settle[c].swap(fresh[c]);
}

} // namespace

void cutAndFill(Domain& domain, FlowScratch& scratch) {
    Sweep sweep{domain, scratch, float(domain.step), float(domain.cell), float(domain.cell * domain.cell)};
    const size_t cells = domain.cells();
    scratch.next.resize(cells);
    scratch.power.resize(cells);
    scratch.coverPower.resize(cells);
    scratch.drained.assign(cells, 0);
    scratch.pool.resize(cells);
    for (int c = 0; c < SedimentClasses; ++c) {
        scratch.flux[c].resize(cells);
        scratch.settle[c].assign(cells, 0.0f);
    }
    // Height each cell can gain in one step: not above the lowest cell that drains into it, and
    // not steeper above the cell it drains to than a deposit of its level stands (a river plain at
    // region scale, a fan at local scale, a debris cone at site scale, a bar front at ground scale).
    constexpr float DepositSlope[LevelCount] = {0, 0.02f, 0.06f, 0.25f, 0.5f};
    const float rise = DepositSlope[size_t(domain.key.level)] * sweep.cell;
    scratch.room.assign(cells, 1e9f);
    for (size_t i = 0; i < cells; ++i) {
        const float* share = &scratch.share[i * 8];
        float lowest = domain.level[i];
        for (int k = 0; k < 8; ++k) {
            if (share[k] <= 0) continue;
            const size_t other = domain.index(int(i % size_t(domain.size)) + NeighborX[k], int(i / size_t(domain.size)) + NeighborZ[k]);
            scratch.room[other] = std::min(scratch.room[other], std::max(0.0f, domain.level[i] - domain.level[other]));
            lowest = std::min(lowest, domain.level[other]);
        }
        if (lowest < domain.level[i]) scratch.room[i] = std::min(scratch.room[i], std::max(0.0f, lowest + rise - domain.level[i]));
    }
    for (auto& cloud : scratch.plume) cloud.assign(cells, 0.0f);
    for (int pass = 0; pass < Passes; ++pass) {
        measurePower(sweep);
        solveHeights(sweep);
        carryLoad(sweep, pass + 1 == Passes);
    }
    // The plume fills a lake up to the water its outlet now holds: none where the lake has drained.
    if (domain.key.level != Level::Ground)
        for (size_t i = 0; i < cells; ++i)
            if (sweep.standing(i)) domain.level[i] = scratch.drained[i] ? domain.ground.rock[i] + domain.sedimentAt(i) : std::min(domain.level[i], scratch.pool[i]);
    settlePlume(domain, scratch, sweep.dt, sweep.area);
}

} // namespace derivean::sim
