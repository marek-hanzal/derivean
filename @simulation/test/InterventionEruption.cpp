#include "InterventionShared.hpp"

#include "chronology/type/Calendar.hpp"
#include "simulation/fn/Live.hpp"
#include "volcanic/fn/Outbreak.hpp"

using namespace derivean::sim;

namespace {

constexpr double Day = 86400.0;

double relative(double a, double b) { return std::abs(a - b) / std::max({std::abs(a), std::abs(b), 1e-30}); }

} // namespace

// An eruption beside the stream during play: what `layOutbreak` leaves goes into the columns as a
// Deposit edit, and a jump that replays the history puts it back.
void testEruption(World& world, const StreamSite& site) {
    forgetInterventions(world);
    const double cell = specOf(Level::Site).cell;

    // One deposit cell: the floor founders, rock seals the cover it buries, loose material lies on top.
    const int64_t gx = int64_t(std::floor(site.at.x / cell)) + 40, gz = int64_t(std::floor(site.at.z / cell));
    {
        auto one = std::make_shared<Deposit>();
        one->origin = {double(gx) * cell, double(gz) * cell};
        one->cell = cell;
        one->width = one->height = 1;
        one->drop = {1.0f};
        one->rock = {3.0f};
        one->loose[Sand] = {0.5f};
        advance(world, Day);
        const SiteColumn before = siteColumn(world, gx, gz);
        const float cover = std::max(0.0f, before.cover[Coarse]) + std::max(0.0f, before.cover[Sand]) + std::max(0.0f, before.cover[Fines]);
        intervene(world, deposit(one));
        const SiteColumn after = siteColumn(world, gx, gz);
        expectLive(after.live && std::abs(after.rock - (before.rock - 1.0f + cover + 3.0f)) < 1e-3f, "a deposit did not sink the floor, seal the cover and lay rock on it", after.rock - before.rock);
        expectLive(after.cover[Coarse] <= 0 && after.cover[Fines] <= 0 && std::abs(after.cover[Sand] - 0.5f) < 1e-4f, "the loose part of a deposit is not on top of its rock", after.cover[Sand]);
        // What the voxels read: the layering went down with the floor, and the rock laid is a pile.
        const CellChange change = changeAt(world, (double(gx) + 0.5) * cell, (double(gz) + 0.5) * cell);
        expectLive(std::abs(change.datum + 1.0f) < 1e-4f, "the layering did not founder with the floor", change.datum);
        expectLive(std::abs(change.pile.thickness - (3.0f + cover)) < 1e-3f && change.pile.lava == 0, "the rock a deposit laid is not a pile of welded fragments", change.pile.thickness);
        // The floor foundering again takes the pile down whole.
        auto sink = std::make_shared<Deposit>(*one);
        sink->rock.clear(), sink->loose[Sand].clear(), sink->drop = {0.5f};
        intervene(world, deposit(sink));
        const CellChange sunk = changeAt(world, (double(gx) + 0.5) * cell, (double(gz) + 0.5) * cell);
        expectLive(std::abs(sunk.datum + 1.5f) < 1e-4f && std::abs(sunk.pile.thickness - change.pile.thickness) < 1e-3f, "foundering thinned the pile on the floor", sunk.pile.thickness - change.pile.thickness);
        const WorldBudget budget = budgetOf(world);
        // To a millimetre of the column: the columns are floats at the ground's height.
        expectLive(std::abs(budget.dug - 1.5 * cell * cell) < 1e-3 * cell * cell && std::abs(budget.filled - 3.5 * cell * cell) < 1e-3 * cell * cell, "a deposit booked other volumes than it laid", budget.filled);
        expectLive(solidImbalance(budget) <= Closed, "a deposit does not close the solid budget", solidImbalance(budget));
        forgetInterventions(world);
    }

    // The ground around the stream, on the site lattice.
    Emplacement field;
    constexpr int Side = 128;
    field.resize(Side, Side);
    field.cell = cell;
    field.origin = {(std::floor(site.at.x / cell) - Side / 2) * cell, (std::floor(site.at.z / cell) - Side / 2) * cell};
    for (int z = 0; z < Side; ++z)
        for (int x = 0; x < Side; ++x) {
            const SurfaceSample sample = surveyAt(world, field.center(x, z));
            field.ground[field.index(x, z)] = sample.ground;
            field.water[field.index(x, z)] = sample.water;
        }
    // A small andesite eruption from a conduit beside the stream.
    Vent vent;
    vent.id = 91;
    vent.magma = Magma::Andesite;
    vent.until = 4.6e9;
    vent.supply = 5e5;
    vent.feeders.push_back({site.at + Vec2{60, 0}, 150.0f, vent.until - 1e5, vent.until, 1e9});
    const Outbreak outbreak = outbreakOf(world.options.seed, vent, 3, vent.until + 1, 2e6);
    const NearField near = layOutbreak(field, outbreak);
    const std::shared_ptr<const Deposit> laid = depositOf(field, near, outbreak);
    double thrown = 0, welded = 0;
    for (size_t i = 0; i < field.cells(); ++i) thrown += double(near.fragments[i]) + double(near.lava[i]), welded += laid->rock[i];
    thrown *= cell * cell;
    expectLive(thrown > 0.5 * (outbreak.fall + outbreak.flows + outbreak.lava) && welded > 0, "the eruption laid little on the grid, or welded none of it", thrown);

    // Live: a day, the eruption, a month in three calls.
    advance(world, Day);
    const double erupted = world.interventions.calendar.seconds;
    const WorldBudget calm = budgetOf(world);
    intervene(world, deposit(laid));
    const WorldBudget fresh = budgetOf(world);
    expectLive(relative(fresh.filled - calm.filled, thrown) < 1e-5 && fresh.dug == calm.dug, "the columns did not take what the eruption laid", (fresh.filled - calm.filled) / thrown);
    expectLive(relative(fresh.groundChange - calm.groundChange, thrown) < 1e-5, "the ground did not rise by what the eruption laid", (fresh.groundChange - calm.groundChange) / thrown);
    const int64_t vx = int64_t(std::floor(outbreak.site.x / cell)), vz = int64_t(std::floor(outbreak.site.z / cell));
    const size_t middle = field.index(int(vx - int64_t(std::floor(field.origin.x / cell))), int(vz - int64_t(std::floor(field.origin.z / cell))));
    expectLive(siteColumn(world, vx, vz).live && laid->rock[middle] > 0, "the vent is not under a welded pile of the eruption", laid->rock[middle]);
    // The voxels read the new rock as andesite of this vent, on top of whatever pile was there.
    GroundLattice lattice;
    lattice.spacing = cell, lattice.shift = 0.5 * cell;
    lattice.originX = vx, lattice.originZ = vz, lattice.countX = lattice.countZ = 1;
    lattice.rock = lattice.datum = lattice.crust = {0.0f};
    for (auto& layer : lattice.cover) layer = {0.0f};
    expectLive(changeLattice(world, lattice) && lattice.pile.size() == 1 && lattice.pile[0].thickness >= laid->rock[middle] - 1e-3f && lattice.pile[0].rock == Material::Andesite && lattice.pile[0].vent == vent.id,
               "the voxels do not read the eruption's rock as its pile", lattice.pile.empty() ? -1.0 : lattice.pile[0].thickness);
    for (int call = 0; call < 3; ++call) advance(world, 10 * Day);
    const double end = world.interventions.calendar.seconds;
    const WorldBudget live = budgetOf(world);
    const SiteColumn over = siteColumn(world, vx, vz);
    expectLive(waterImbalance(live) <= Closed && saltImbalance(live) <= Closed && solidImbalance(live) <= Closed, "a month after an eruption does not close its budgets", solidImbalance(live));

    // Before the eruption the ground holds none of it; at the end a jump that replays it does, to
    // a stated tolerance: the deposit itself exactly, what water did with it since to a part in a
    // thousand, as a jump and a walk differ only in how their steps cut the water's time.
    jumpTo(world, 0.5 * erupted);
    expectLive(budgetOf(world).filled == 0 && world.interventions.history.size() == 1, "a jump before the eruption kept its deposit, or lost the history");
    jumpTo(world, end);
    const WorldBudget replayed = budgetOf(world);
    expectLive(replayed.filled == live.filled && replayed.dug == live.dug, "a replayed eruption laid other volumes", replayed.filled - live.filled);
    // What water moved since, apart from what the edits laid.
    const double worked = live.groundChange - (live.filled - live.dug), reworked = replayed.groundChange - (replayed.filled - replayed.dug);
    expectLive(std::abs(reworked - worked) < 1e-3 * std::max(live.solid.eroded + live.solid.deposited, 1.0), "a replayed eruption left other ground", reworked - worked);
    expectLive(relative(replayed.live.water, live.live.water) < 1e-3, "a replayed eruption left other water", relative(replayed.live.water, live.live.water));
    expectLive(std::abs(siteColumn(world, vx, vz).ground - over.ground) < 0.05f + 1e-3f * laid->rock[middle], "the replayed pile over the vent is not the live one", siteColumn(world, vx, vz).ground - over.ground);
    expectLive(solidImbalance(replayed) <= Closed && waterImbalance(replayed) <= Closed, "a replayed eruption does not close its budgets", solidImbalance(replayed));
    forgetInterventions(world);
}
