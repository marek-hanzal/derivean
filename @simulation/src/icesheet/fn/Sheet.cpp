#include "icesheet/fn/Sheet.hpp"

#include "icesheet/fn/Dating.hpp"
#include "icesheet/fn/Local.hpp"
#include "icesheet/fn/Region.hpp"
#include "icesheet/service/Glaciations.hpp"
#include "landscape/fn/Process.hpp"


#include <cstdlib>   // TEMPORARY A/B

namespace derivean::sim {
namespace {

constexpr int Stride = 4;              // region steps whose glaciations are worked into the ground together
constexpr int Places = 4;              // places between two stages a stage's margin stands at in turn
constexpr double ReboundTime = 5000;   // yr over which unloaded crust comes back by all but 1/e

SheetWork& workOf(Domain& domain, const Setting& setting) {
    if (!domain.sheet) {
        domain.sheet = std::make_shared<SheetWork>();
        if (const auto ice = continentalIce(setting)) domain.sheet->source = worldSource(ice, setting.seed, setting.age);
    }
    return *domain.sheet;
}

// Ground can rise into the snow during a replay, so a domain no ice reached is asked again.
bool reached(const Domain& domain, SheetWork& work) {
    if (work.source.cooling.empty()) return false;
    return work.reached = work.reached || iceReaches(domain, work.source);
}

// Every Stride steps the glaciations of those steps are worked into the ground at once. The last
// cycle of the window is left to the Leave stage, which books what it leaves.
void stepRegion(Domain& domain, const Setting& setting) {
    SheetWork& work = workOf(domain, setting);
    const int done = domain.taken + 1;
    const bool last = done >= windowAt(Level::Region, setting.age).steps;
    if (done % Stride != 0 && !last) return;
    const int steps = done - work.worked;
    work.worked = done;
    if (!reached(domain, work)) return;
    const double span = steps * domain.step, years = span - (last ? GlacialCycle : 0.0);
    if (years <= 0) return;
    // A stage stands for every climate between itself and the next. Working after working it is
    // taken a different part of the way there, so its margin does not wear one line into the
    // ground for the whole window.
    const double towards = double((done / Stride) % Places) / Places, middle = domain.time + domain.step - 0.5 * span;
    const std::vector<float> shares = stageShares(shiftedLadder(work.source, towards), setting.seed, middle, lastTermination(setting.age));
    workSheets(domain, work, shares, towards, years, span, false, setting.workers);
}

void leaveRegion(Domain& domain, const Setting& setting) {
    SheetWork& work = workOf(domain, setting);
    if (!reached(domain, work)) return;
    const double present = lastTermination(setting.age);
    workSheets(domain, work, stageShares(work.source.cooling, setting.seed, present, present), 0.0, GlacialCycle, 0.0, true, setting.workers);
    work.rest = sinkCrust(domain, work, std::exp(-sinceMaximum(setting.seed, present) / ReboundTime));
}

// The tile's own cells of what the last sheet left over the whole domain.
std::shared_ptr<const SheetRecord> presentRegion(Domain& domain, const Setting& setting) {
    SheetWork& work = workOf(domain, setting);
    if (!work.reached || work.now.empty()) return nullptr;
    auto record = std::make_shared<SheetRecord>();
    const size_t cells = size_t(TileCells) * TileCells;
    for (auto* field : {&record->slideX, &record->slideZ, &record->worn, &record->till, &record->moraine, &record->outwash, &record->esker, &record->meltwater}) field->resize(cells);
    record->meltTo.resize(cells);
    for (size_t i = 0; i < domain.cells(); ++i) {
        domain.ice[i] = work.now[i];
        domain.iceAge[i] = work.lastMaximum[i];
        domain.rebound[i] += work.rest[i] / float(ReboundTime);
    }
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            record->slideX[to] = work.left.slideX[from], record->slideZ[to] = work.left.slideZ[from];
            record->worn[to] = work.worn.empty() ? 0.0f : work.worn[from];
            record->till[to] = work.left.till[from], record->moraine[to] = work.left.moraine[from];
            record->outwash[to] = work.left.outwash[from], record->esker[to] = work.left.esker[from];
            record->meltwater[to] = work.left.meltwater[from], record->meltTo[to] = work.left.meltTo[from];
        }
    }
    return record;
}

// A finer level reads what its parent recorded once it has settled, and is left by the last sheet.
std::shared_ptr<const SheetRecord> sheetFiner(Domain& domain, IceStage stage, const std::function<const Tile*(TileKey)>& tiles) {
    if (stage == IceStage::Step || std::getenv("ICE_FINER_OFF")) return nullptr;   // TEMPORARY A/B
    const ParentSheet parent = readParentSheet(domain, tiles);
    if (!parent.any) return nullptr;
    if (!domain.sheet) domain.sheet = std::make_shared<SheetWork>();
    if (stage == IceStage::Leave) leaveFiner(domain, *domain.sheet, parent, parentTunnels(domain, tiles));
    return stage == IceStage::Present ? presentFiner(domain, *domain.sheet, parent) : nullptr;
}

} // namespace

std::shared_ptr<const SheetRecord> sheetIce(Domain& domain, FlowScratch&, const Setting& setting, IceStage stage, const std::function<const Tile*(TileKey)>* tiles) {
    if (domain.key.level == Level::Ground) return nullptr;
    if (domain.key.level != Level::Region) return tiles ? sheetFiner(domain, stage, *tiles) : nullptr;
    if (stage == IceStage::Step) stepRegion(domain, setting);
    if (stage == IceStage::Leave) leaveRegion(domain, setting);
    return stage == IceStage::Present ? presentRegion(domain, setting) : nullptr;
}

} // namespace derivean::sim
