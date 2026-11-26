#include "icesheet/fn/Sheet.hpp"

#include "chronology/constant/Timeline.hpp"
#include "climate/type/Climate.hpp"
#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Meltwater.hpp"
#include "icesheet/fn/Region.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {
namespace {

float groundOf(const Domain& domain, size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); }

void clear(SheetRecord& left, size_t cells) {
    for (auto* field : {&left.slideX, &left.slideZ, &left.till, &left.moraine, &left.outwash, &left.esker, &left.meltwater}) field->assign(cells, 0.0f);
    left.meltTo.assign(cells, 255);
}

} // namespace

SheetSource worldSource(std::shared_ptr<const ContinentalIce> ice, uint64_t seed, double age) {
    SheetSource source;
    for (const PlanetStage& stage : ice->stages) source.cooling.push_back(stage.cooling);
    // The smooth reading leaves a film of ice beyond the planet's last iced cell; it is not a sheet.
    source.thickness = [ice, seed](int stage, Vec2 position) {
        const float thickness = float(samplePlanet(ice->stages[size_t(stage)].thickness, planetPoint(seed, position)));
        return thickness < LeastSheet ? 0.0f : thickness;
    };
    source.sink = [ice, seed](int stage, Vec2 position) { return float(samplePlanet(ice->stages[size_t(stage)].sink, planetPoint(seed, position))); };
    source.heat = [ice, seed](Vec2 position) { return float(samplePlanet(ice->heat, planetPoint(seed, position), Kernel::Linear)); };
    source.halfRange = [seed, age](Vec2 position) { return float(zonalClimate(seed, planetPoint(seed, position).z, age).seasonal); };
    return source;
}

WearTotals workSheets(Domain& domain, SheetWork& work, const std::vector<float>& shares, double towards, double years, double span, bool leave, Workers* workers) {
    WearTotals totals;
    const size_t cells = domain.cells();
    std::vector<float> debris[SedimentClasses], flow, top[2];
    std::vector<uint8_t> to;
    if (leave) clear(work.left, cells);
    for (int stage = 0; stage < work.source.stages(); ++stage) {
        const bool kept = leave && stage <= IceLastMaximum;
        if (shares[size_t(stage)] <= 0 && !kept) continue;
        solveStage(domain, work, stage, leave ? 0.0 : towards, kept ? stage : -1, workers);
        const SheetField field = readStage(domain, work, stage, workers);
        const std::vector<int32_t> downIce = downIceOrder(field);
        const double time = shares[size_t(stage)] * years;
        // Only the last maximum's ice has since left the ground it covered: its deposits are the ones to be seen.
        const bool record = leave && stage == IceLastMaximum;
        if (record) routeMeltwater(domain, field, flow, to);
        if (time > 0) {
            wearBed(domain, work, field, downIce, time, span, debris, totals);
            carryDebris(domain, field, downIce, debris, time, years / GlacialCycle, span, record ? &flow : nullptr, record ? &work.left : nullptr, totals);
        }
        if (record) {
            spreadOutwash(domain, field, flow, to, work.left, totals);
            for (size_t i = 0; i < cells; ++i) {
                const float steep = std::sqrt(field.fallX[i] * field.fallX[i] + field.fallZ[i] * field.fallZ[i]);
                if (steep > 0) work.left.slideX[i] = -field.slide[i] * field.fallX[i] / steep, work.left.slideZ[i] = -field.slide[i] * field.fallZ[i] / steep;
            }
            work.left.meltwater = flow, work.left.meltTo = to;
        }
        if (kept) {
            top[stage].assign(cells, -1e30f);
            for (size_t i = 0; i < cells; ++i)
                if (field.thickness[i] > 0) top[stage][i] = field.top[i];
        }
    }
    if (leave) {
        // The ice stands on the ground as the last cycle left it.
        work.now.resize(cells), work.lastMaximum.resize(cells);
        for (size_t i = 0; i < cells; ++i) {
            const float ground = groundOf(domain, i);
            work.now[i] = std::max(0.0f, top[IceNow][i] - ground);
            work.lastMaximum[i] = std::max(work.now[i], top[IceLastMaximum][i] - ground);
        }
    }
    return totals;
}

std::vector<double> shiftedLadder(const SheetSource& source, double towards) {
    std::vector<double> ladder = source.cooling;
    for (size_t k = 0; k + 1 < ladder.size(); ++k) ladder[k] = mix(source.cooling[k], source.cooling[k + 1], towards);
    return ladder;
}

std::vector<float> sinkCrust(Domain& domain, const SheetWork& work, double remaining) {
    const IceGrid& now = work.coarse[IceNow];
    const IceGrid& last = work.coarse[IceLastMaximum];
    const int n = now.width;
    std::vector<float> rest(domain.cells(), 0.0f);
    for (int z = 0; z < domain.size; ++z) {
        const float gz = (z + 0.5f) / SheetCoarsening - 0.5f;
        const int iz = std::clamp(int(std::floor(gz)), 0, n - 2);
        const float tz = std::clamp(gz - float(iz), 0.0f, 1.0f);
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] == Settled) continue;
            const float gx = (x + 0.5f) / SheetCoarsening - 0.5f;
            const int ix = std::clamp(int(std::floor(gx)), 0, n - 2);
            const float tx = std::clamp(gx - float(ix), 0.0f, 1.0f);
            const auto blend = [&](const std::vector<float>& from) {
                return (1 - tx) * (1 - tz) * from[now.index(ix, iz)] + tx * (1 - tz) * from[now.index(ix + 1, iz)] + (1 - tx) * tz * from[now.index(ix, iz + 1)] +
                       tx * tz * from[now.index(ix + 1, iz + 1)];
            };
            const float held = blend(now.sink);
            rest[i] = float(remaining) * std::max(0.0f, blend(last.sink) - held);
            domain.ground.rock[i] -= held + rest[i];
            domain.ground.datum[i] -= held + rest[i];
        }
    }
    return rest;
}

} // namespace derivean::sim
