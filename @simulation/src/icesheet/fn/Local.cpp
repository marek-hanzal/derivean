#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"

#include <algorithm>

namespace derivean::sim {

bool leaveFiner(Domain& domain, SheetWork& work, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const size_t cells = domain.cells();
    if (std::none_of(parent.iceThen.begin(), parent.iceThen.end(), [](float ice) { return ice >= LeastSheet; })) return false;
    work.reached = true;
    work.worn = parent.worn;
    for (auto* field : {&work.left.slideX, &work.left.slideZ, &work.left.till, &work.left.moraine, &work.left.outwash, &work.left.esker, &work.left.meltwater}) field->assign(cells, 0.0f);
    work.left.meltTo.assign(cells, 255);
    work.left.passing.assign(cells, 0.0f);
    // The tunnels' gravel and sand are set aside before the ice takes up the cover they lie in, and
    // stand on top of the till once the ice has gone.
    const EskerFill eskers = setAsideEskers(domain, parent, tunnels);
    SheetLoad carried = takeUpBed(domain, work, parent);
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
    if (!work.reached) return nullptr;
    auto record = std::make_shared<SheetRecord>();
    const size_t cells = size_t(TileCells) * TileCells;
    for (auto* field : {&record->slideX, &record->slideZ, &record->worn, &record->till, &record->moraine, &record->outwash, &record->esker, &record->meltwater}) field->resize(cells);
    record->meltTo.assign(cells, 255);
    record->passing.resize(cells);
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            record->slideX[to] = parent.slideX[from], record->slideZ[to] = parent.slideZ[from];
            record->worn[to] = work.worn[from];
            record->till[to] = work.left.till[from], record->moraine[to] = work.left.moraine[from], record->esker[to] = work.left.esker[from];
            record->passing[to] = work.left.passing[from];
        }
    return record;
}

} // namespace derivean::sim
