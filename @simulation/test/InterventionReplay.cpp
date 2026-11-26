#include "InterventionShared.hpp"

using namespace derivean::sim;

namespace {

constexpr double Years = 100.0;

struct Outcome {
    uint64_t digest{};
    WorldBudget budget;
};

// Replays a history on the untouched world: the edits take effect at their own times, and the
// calendar is walked to `end` in `calls` equal steps.
Outcome replay(World& world, const std::vector<Edit>& history, double end, int calls) {
    forgetInterventions(world);
    world.interventions.history = history;
    for (int call = 1; call <= calls; ++call) advance(world, end * call / calls - world.interventions.calendar.seconds);
    Outcome outcome;
    outcome.budget = budgetOf(world);
    outcome.digest = digestOf(world);
    return outcome;
}

double relative(double a, double b) { return std::abs(a - b) / std::max({std::abs(a), std::abs(b), 1e-30}); }

} // namespace

// A stream turned out of its bed, and a hundred years.
void testDiversion(World& world, const StreamSite& site) {
    const ScenarioReport report = runDiversion(world, site, Years);
    expectClosed(report);
    const auto value = [&](size_t stage, const char* measure) { return valueOf(report, stage, measure); };
    enum { Untouched, Day, Century };
    const double flow = value(Untouched, "old course flow");
    expectLive(flow >= StreamFlow, "the scenario did not start on a stream", flow);
    for (const size_t stage : {size_t(Day), size_t(Century)}) {
        expectLive(value(stage, "old course flow") < 0.2 * flow, "the abandoned bed still carries the stream", value(stage, "old course flow") / flow);
        expectLive(value(stage, "new course flow") > 0.5 * flow, "the new channel does not carry the stream", value(stage, "new course flow") / flow);
        expectLive(value(stage, "courses apart") >= 12.0, "the new course is the old one", value(stage, "courses apart"));
    }
    const WorldBudget& century = report.stages[Century].budget;
    expectLive(century.dug > 0 && century.solid.eroded > 0, "a century on a new course cut nothing", century.solid.eroded);
    // What the rivers brought is their load times the time sediment has been moved for.
    expectLive(relative(century.solid.carriedIn, value(Century, "entering load") * Years) < 1e-9, "the load carried in is not load times time", century.solid.carriedIn);
    expectLive(century.solid.deposited <= century.solid.carriedIn + century.solid.eroded + 1e-9, "more was laid than was carried");

    // The game reads the edited world through the same accessors as the untouched one.
    const std::vector<Edit> history = world.interventions.history;
    const double end = world.interventions.calendar.seconds;
    const Vec2 intake = history[1].from + normalized(history[1].to - history[1].from) * 12.0;
    const double cell = specOf(Level::Site).cell;
    const SiteColumn column = siteColumn(world, int64_t(std::floor(intake.x / cell)), int64_t(std::floor(intake.z / cell)));
    SurfaceSample sample = surveyAt(world, intake);
    const float untouched = sample.ground;
    overlaySurface(world, intake, sample);
    expectLive(column.live && sample.ground < untouched - 0.05f && editedTile(world, tileAt(Level::Site, intake)) != nullptr, "the channel is not in what the game reads", untouched - sample.ground);
    expectLive(editedTile(world, {Level::Site, site.tile.x + 5, site.tile.z}) == nullptr, "an untouched tile is called live");

    // The same history gives the same state, to the bit.
    const Outcome first = replay(world, history, end, 1);
    const Outcome again = replay(world, history, end, 1);
    expectLive(first.digest == again.digest, "the same history gave two different worlds");
    // The scenario walked there in three calls; one jump lands within a stated tolerance, and so
    // does a walk in four hundred steps. Filling is exact for any steps; what differs is how the
    // steps cut evaporation and mixing.
    const Outcome walked = replay(world, history, end, 400);
    for (const Outcome* other : {&first, &walked}) {
        expectLive(relative(other->budget.live.water, century.live.water) < 1e-3, "a jump and a walk disagree on the water held", relative(other->budget.live.water, century.live.water));
        expectLive(relative(other->budget.live.salt, century.live.salt) < 2e-2, "a jump and a walk disagree on the salt held", relative(other->budget.live.salt, century.live.salt));
        expectLive(relative(other->budget.groundChange, century.groundChange) < 1e-3, "a jump and a walk disagree on the ground", relative(other->budget.groundChange, century.groundChange));
        expectLive(relative(other->budget.solid.eroded, century.solid.eroded) < 5e-2, "a jump and a walk disagree on what was cut", relative(other->budget.solid.eroded, century.solid.eroded));
        expectLive(waterImbalance(other->budget) <= Closed && saltImbalance(other->budget) <= Closed && solidImbalance(other->budget) <= Closed, "a replay does not close its budgets");
    }
    // A date before the cut holds no cut.
    jumpTo(world, 0.5 * history[1].time);
    expectLive(budgetOf(world).dug == 0 && world.interventions.history.size() == history.size(), "a jump before the cut kept the cut, or lost the history");

    // An edit two tiles away, in a group of its own, changes nothing here: not one bit.
    const double span = tileSpan(Level::Site);
    const Vec2 far{(site.tile.x + 2 + 0.5) * span, (site.tile.z + 0.5) * span};
    std::vector<Edit> wider = history;
    Edit pit = stroke(EditKind::Lower, far - Vec2{20, 0}, far + Vec2{20, 0}, 40.0, -1e6, -1e6);
    pit.heightFrom = pit.heightTo = surveyAt(world, far).ground - 3.0;
    pit.time = 0.5 * end;
    wider.push_back(pit);
    const Outcome both = replay(world, wider, end, 1);
    expectLive(world.interventions.overlay.sets.size() == 2 && both.budget.dug > first.budget.dug + 1000, "the far edit did not make a group of its own");
    // The digest of the stream's tile alone: the far tile is set aside while it is taken.
    auto& tiles = world.interventions.overlay.tiles;
    std::unique_ptr<EditedTile> aside = std::move(tiles[{site.tile.z, site.tile.x + 2}]);
    tiles.erase({site.tile.z, site.tile.x + 2});
    const uint64_t here = digestOf(world);
    tiles[{site.tile.z, site.tile.x + 2}] = std::move(aside);
    expectLive(here == first.digest, "an edit in another group changed this one");
    expectLive(waterImbalance(both.budget) <= Closed && solidImbalance(both.budget) <= Closed, "two groups do not close their budgets together");

    // The same world built on one thread, with the same history, is the same to the bit.
    World single({world.options.seed, world.options.age, 1});
    const Outcome alone = replay(single, history, end, 1);
    expectLive(alone.digest == first.digest, "the live state depends on the thread count");
    forgetInterventions(world);
}
