#include "intervention/fn/Scenario.hpp"

namespace derivean::sim {
namespace {

constexpr double Hour = 3600.0;

// `intervene` rebuilds the groups, so a LiveSet reference is only good until the next edit: the
// set is looked up again after each, and what the edits need of the grid is copied here first.
struct Frame {
    int64_t originX{}, originZ{};   // global site cell of the grid's corner
    int width{};
    double cell{};
};

// The grid cells [x0, x1] × [z0, z1] as one stroke.
Edit box(const Frame& frame, EditKind kind, int x0, int x1, int z0, int z1, double height) {
    if (x0 > x1) std::swap(x0, x1);
    if (z0 > z1) std::swap(z0, z1);
    const double middle = (double(frame.originZ) + 0.5 * (z0 + z1 + 1)) * frame.cell;
    return stroke(kind, {double(frame.originX + x0) * frame.cell, middle}, {double(frame.originX + x1 + 1) * frame.cell, middle}, (z1 - z0 + 1) * frame.cell,
                  height, height, Coarse);
}

} // namespace

// The works are an excavation on the stream cell nearest the middle of the tile, so they stand on
// any ground: a pit walled all round and split by two inner dams into an upper lake, a small lower
// lake and a third basin, with the upper lake towards the incoming water. A moat outside the wall
// leads the stream round them, so that sealing the works dams nothing; a sluice from the moat then
// feeds the upper lake. Lengths along the works are in cells from the anchor.
ScenarioReport runLakeCascade(World& world, const StreamSite& site) {
    ScenarioReport report;
    report.title = "Cascade: the dam of an upper lake is cut, the lower lake overflows into a third basin";
    report.measures = {"upper level", "upper volume", "upper full", "lower level", "lower volume", "lower full", "third level", "third volume", "elsewhere"};
    const LiveSet& untouched = openSite(world, site);
    // The works need ground to stand on: an open stream bed with next to no standing water under
    // their footprint. A pit dug inside a lake starts full, and nothing can then be told apart.
    const Frame frame{int64_t(untouched.originX) * TileCells, int64_t(untouched.originZ) * TileCells, untouched.grid.width, untouched.grid.cell};
    const auto [anchor, flip] = cascadePlace(untouched);
    if (anchor < 0) {
        report.failure = "no open stream bed with dry ground for the works in the middle of the site";
        return report;
    }
    const int ax = anchor % frame.width, az = anchor / frame.width;
    const double stream = surfaceAt(untouched, anchor);
    const double floor = stream - 3.5, upperSill = floor + 2.5, lowerSill = floor + 1.2, crest = stream + 1.0, moat = stream - 0.5;
    const auto along = [&](int cells) { return ax + flip * cells; };
    const auto cellAt = [&](int x, int z) { return int32_t(z * frame.width + x); };
    const int32_t probes[3] = {cellAt(along(0), az), cellAt(along(9), az), cellAt(along(22), az)};
    const int bounds[3][2] = {{-5, 4}, {7, 12}, {15, 29}};
    const auto measure = [&](const std::string& label) {
        syncLive(world);
        const LiveSet& set = setOf(world);
        std::vector<double> values;
        double held = 0;
        for (int basin = 0; basin < 3; ++basin) {
            const LakeState lake = lakeAt(set, probes[basin]);
            const double volume = waterIn(set, along(bounds[basin][0]), along(bounds[basin][1]), az - 7, az + 7);
            values.insert(values.end(), {lake.level, volume});
            if (basin < 2) values.push_back(lake.full ? 1.0 : 0.0);
            held += volume;
        }
        const WorldBudget budget = budgetOf(world);
        values.push_back(budget.live.water - held);
        report.stages.push_back({label, budget, std::move(values)});
    };
    measure("untouched stream");

    for (const Edit& edit : {box(frame, EditKind::Lower, along(-9), along(33), az - 11, az - 10, moat), box(frame, EditKind::Lower, along(-9), along(33), az + 10, az + 11, moat),
                             box(frame, EditKind::Lower, along(-9), along(-8), az - 11, az + 11, moat), box(frame, EditKind::Lower, along(32), along(33), az - 11, az + 11, moat),
                             box(frame, EditKind::Level, along(-5), along(29), az - 7, az + 7, floor),
                             box(frame, EditKind::Raise, along(-7), along(31), az - 9, az - 8, crest), box(frame, EditKind::Raise, along(-7), along(31), az + 8, az + 9, crest),
                             box(frame, EditKind::Raise, along(-7), along(-6), az - 9, az + 9, crest), box(frame, EditKind::Raise, along(30), along(31), az - 9, az + 9, crest),
                             box(frame, EditKind::Raise, along(5), along(6), az - 7, az + 7, upperSill), box(frame, EditKind::Raise, along(13), along(14), az - 7, az + 7, lowerSill)})
        intervene(world, edit);
    advance(world, Hour);
    measure("works built, still sealed");
    if (setOf(world).grid.width != frame.width || world.interventions.overlay.sets.size() != 1) {
        report.failure = "the works reached beyond the site's tile";
        return report;
    }

    // The sluice goes where the water in the moat stands highest against the wall of the upper lake,
    // a little under that water; where the moat runs dry there, under its bed.
    {
        const LiveSet& set = setOf(world);
        int32_t best = -1;
        Edit sluice;
        const auto consider = [&](int x, int z, const Edit& through) {
            const int32_t cell = cellAt(x, z);
            const bool wet = set.grid.depth[size_t(cell)] > 0.02, bestWet = best >= 0 && set.grid.depth[size_t(best)] > 0.02;
            if (!wet && set.grid.discharge[size_t(cell)] < StreamFlow) return;
            if (best >= 0 && (wet != bestWet ? bestWet : wet ? surfaceAt(set, cell) <= surfaceAt(set, best) : set.grid.discharge[size_t(cell)] <= set.grid.discharge[size_t(best)])) return;
            best = cell;
            sluice = through;
            sluice.heightFrom = sluice.heightTo = wet ? surfaceAt(set, cell) - 0.1 : double(set.grid.ground[size_t(cell)]) - 0.3;
        };
        for (int z = az - 6; z <= az + 6; ++z) consider(along(-8), z, box(frame, EditKind::Lower, along(-7), along(-6), z - 1, z + 1, 0));
        for (int step = -4; step <= 3; ++step) {
            consider(along(step), az - 10, box(frame, EditKind::Lower, along(step) - 1, along(step) + 1, az - 9, az - 8, 0));
            consider(along(step), az + 10, box(frame, EditKind::Lower, along(step) - 1, along(step) + 1, az + 8, az + 9, 0));
        }
        if (best < 0) {
            report.failure = "the stream does not reach the wall of the upper lake";
            return report;
        }
        intervene(world, sluice);
    }
    // The upper lake is filled in steps kept to about a hundred cubic metres each, whatever the
    // stream brings, so that the moment it is full is caught before much has gone over into the
    // lower one and the step that follows the cut adds little of its own.
    const auto upperHolds = [&] {
        syncLive(world);
        return waterIn(setOf(world), along(bounds[0][0]), along(bounds[0][1]), az - 7, az + 7);
    };
    double step = 600.0, held = upperHolds();
    for (int steps = 0; steps < 4000 && !lakeAt(setOf(world), probes[0]).full; ++steps) {
        advance(world, step);
        const double gained = upperHolds() - held;
        held += gained;
        step = gained > 150.0 ? std::max(60.0, 0.5 * step) : gained < 50.0 ? std::min(5 * DaySeconds, 2.0 * step) : step;
    }
    if (!lakeAt(setOf(world), probes[0]).full) report.failure = "the upper lake never filled";
    measure("sluice open, upper lake full");

    intervene(world, box(frame, EditKind::Lower, along(5), along(6), az - 1, az + 1, floor + 0.4));
    advance(world, step);
    measure("just after the cut");
    advance(world, 30 * DaySeconds);
    measure("thirty days later");
    return report;
}

// The stream is given a lower way out: a channel whose intake lies under the bed the water used
// to leave by, led aside and then down to lower ground or water. Nothing is dammed; the old bed is
// left dry because the water no longer reaches it.
ScenarioReport runDiversion(World& world, const StreamSite& site, double years) {
    constexpr double Wide = 8.0;          // m: two site cells, so the channel holds across a diagonal
    constexpr double Aside = 40.0;        // m the channel leaves the old bed by before it turns
    ScenarioReport report;
    report.title = "Diversion: a new channel takes a stream out of its bed";
    report.measures = {"old course flow", "new course flow", "courses apart", "intake level", "outfall level", "entering load"};
    const LiveSet& untouched = openSite(world, site);
    const WaterGrid& grid = untouched.grid;   // like `untouched`, only good until the first edit
    const int width = grid.width;
    const IntakePlace place = diversionPlace(untouched);
    const int32_t anchor = place.anchor, outfall = place.outfall;
    const double intake = place.intake;
    const std::vector<int32_t>& old = place.old;
    if (anchor < 0) {
        report.failure = "no open stream bed in the middle of the site has lower ground within reach";
        return report;
    }
    // Aside first, at right angles to the old bed and towards the outfall's side, then on to it.
    const Vec2 from = centreOf(untouched, anchor), to = centreOf(untouched, outfall);
    const Vec2 down = normalized(centreOf(untouched, old[6]) - from), across = perpendicular(down) * (dot(perpendicular(down), to - from) >= 0 ? 1.0 : -1.0);
    const Vec2 corners[3] = {from, from + across * Aside, to};
    const double legs[2] = {Aside, length(corners[2] - corners[1])};
    // Onto dry ground the channel ends just above it; into a lake it ends a little under the surface.
    const double end = grid.depth[size_t(outfall)] > 0 ? std::max(double(grid.ground[size_t(outfall)]) + 0.05, surfaceAt(untouched, outfall) - 0.2)
                                                        : double(grid.ground[size_t(outfall)]) + 0.1;
    const double heights[3] = {intake, mix(intake, end, legs[0] / (legs[0] + legs[1])), end};
    std::vector<Vec2> oldCourse;
    for (const int32_t cell : old) oldCourse.push_back(centreOf(untouched, cell));

    // The new course is read across the channel's whole width, most of the way along its first leg.
    const Vec2 gauge = from + across * (0.7 * Aside);
    const int gaugeX = int(std::floor(gauge.x / grid.cell)) - untouched.originX * TileCells, gaugeZ = int(std::floor(gauge.z / grid.cell)) - untouched.originZ * TileCells;
    double apart = 1e30;
    for (const Vec2& point : oldCourse) apart = std::min(apart, length(point - gauge));
    const auto measure = [&](const std::string& label) {
        syncLive(world);
        const LiveSet& set = setOf(world);
        double taken = 0, load = 0;
        for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 1; ++dx) taken = std::max(taken, double(set.grid.discharge[size_t(gaugeZ + dz) * size_t(width) + size_t(gaugeX + dx)]));
        for (const WaterSource& source : set.grid.sources) load += source.load;
        report.stages.push_back({label, budgetOf(world), {set.grid.discharge[size_t(old[6])], taken, apart, surfaceAt(set, anchor), surfaceAt(set, outfall), load}});
    };
    measure("untouched stream");
    for (int leg = 0; leg < 2; ++leg) {
        // Legs overlap by half a width so that the bend is cut out whole.
        const Vec2 run = normalized(corners[leg + 1] - corners[leg]);
        intervene(world, stroke(EditKind::Lower, corners[leg] - run * (0.5 * Wide), corners[leg + 1] + run * (0.5 * Wide), Wide, heights[leg], heights[leg + 1]));
    }
    if (setOf(world).grid.width != width || world.interventions.overlay.sets.size() != 1) {
        report.failure = "the channel reached beyond the site's tile";
        return report;
    }
    advance(world, DaySeconds);
    measure("one day after the cut");
    advance(world, years * YearSeconds - DaySeconds);
    measure("after the jump");
    return report;
}

} // namespace derivean::sim
