#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

Ground coverOf(const Domain& domain) {
    Ground cover;
    for (int c = 0; c < SedimentClasses; ++c) cover.sediment[c] = domain.ground.sediment[c];
    return cover;
}

} // namespace

bool leaveFiner(Domain& domain, SheetWork& work, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const size_t cells = domain.cells();
    if (std::none_of(parent.iceThen.begin(), parent.iceThen.end(), [](float ice) { return ice >= LeastSheet; })) {
        keepLaidBack(domain, work, nullptr, nullptr);
        return false;
    }
    work.reached = true;
    work.worn = parent.worn;
    for (auto* field : {&work.left.slideX, &work.left.slideZ, &work.left.till, &work.left.moraine, &work.left.outwash, &work.left.esker, &work.left.meltwater}) field->assign(cells, 0.0f);
    work.left.meltTo.assign(cells, 255);
    for (auto& part : work.left.passing) part.assign(cells, 0.0f);
    // The tunnels' gravel and sand are set aside before the ice takes up the cover they lie in, and
    // stand on top of the till once the ice has gone.
    const Ground before = coverOf(domain);
    const EskerFill eskers = setAsideEskers(domain, parent, tunnels);
    const Ground tunnelled = coverOf(domain);
    SheetLoad carried = takeUpBed(domain, work, parent);
    keepLaidBack(domain, work, &before, &tunnelled);
    lodgeTill(domain, work, parent, carried);
    layEskers(domain, work, eskers);
    return true;
}

std::shared_ptr<const SheetRecord> presentFiner(Domain& domain, const SheetWork& work, const ParentSheet& parent) {
    for (size_t i = 0; i < domain.cells(); ++i) {
        const float ground = domain.ground.rock[i] + domain.sedimentAt(i);
        if (parent.iceNow[i] >= LeastSheet) domain.ice[i] = std::max(domain.ice[i], parent.topNow[i] - ground);
        if (parent.iceThen[i] >= LeastSheet) domain.iceAge[i] = std::max(domain.iceAge[i], parent.topThen[i] - ground);
        domain.iceAge[i] = std::max(domain.iceAge[i], domain.ice[i]);
    }
    // A domain the sheet never reached keeps a record only of the parent's deposits laid back on it.
    bool laid = false;
    if (work.left.laidBack[0].size() == domain.cells())
        for (int z = Halo; z < Halo + TileCells && !laid; ++z)
            for (int x = Halo; x < Halo + TileCells && !laid; ++x)
                for (int c = 0; c < SedimentClasses; ++c) laid = laid || work.left.laidBack[c][domain.index(x, z)] > 0;
    if (!work.reached && !laid) return nullptr;
    auto record = std::make_shared<SheetRecord>();
    record->reached = work.reached;
    const size_t cells = size_t(TileCells) * TileCells;
    for (auto* field : {&record->slideX, &record->slideZ, &record->worn, &record->till, &record->moraine, &record->outwash, &record->esker, &record->meltwater}) field->assign(cells, 0.0f);
    record->meltTo.assign(cells, 255);
    if (work.reached)
        for (auto& part : record->passing) part.resize(cells);
    if (laid)
        for (auto& part : record->laidBack) part.resize(cells);
    record->sheetThen.resize(cells);
    record->eskerArrived = work.left.eskerArrived, record->eskerSent = work.left.eskerSent;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            for (int c = 0; c < SedimentClasses && laid; ++c) record->laidBack[c][to] = work.left.laidBack[c][from];
            if (!work.reached) continue;
            record->slideX[to] = parent.slideX[from], record->slideZ[to] = parent.slideZ[from];
            record->worn[to] = work.worn[from];
            record->till[to] = work.left.till[from], record->moraine[to] = work.left.moraine[from], record->esker[to] = work.left.esker[from], record->outwash[to] = work.left.outwash[from];
            for (int c = 0; c < SedimentClasses; ++c) record->passing[c][to] = work.left.passing[c][from];
            // Over this tile's own ground, so the level below finds the parent's surface again.
            const float ground = domain.ground.rock[from] + domain.sedimentAt(from);
            record->sheetThen[to] = parent.iceThen[from] >= LeastSheet ? std::max(0.0f, parent.topThen[from] - ground) : 0.0f;
        }
    return record;
}

} // namespace derivean::sim
