#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Ocean.hpp"

namespace derivean::sim {
namespace {

void add(Tally& to, const Tally& part) {
    to.water += part.water;
    to.salt += part.salt;
}

// The sea and the live tiles are closed separately and each against what it holds, so an error on
// a few tiles cannot hide in the rounding of an ocean or behind a century of river passing through.
// Only where more than a hundred thousand times the tiles' own stock has passed through them is
// the error measured against a hundred-thousandth of that instead: sums of doubles end there.
// A body its straits did not book counts too: the sum over all bodies would hide what moved between them.
double imbalanceOf(const WorldBudget& b, double Tally::*part) {
    const WaterFlows& f = b.flows;
    // The salt of the sea is in its bodies and in the crust of the cells that fell dry.
    const bool salt = part == &Tally::salt;
    const double crust = salt ? b.crust : 0.0, crustAtStart = salt ? b.crustAtStart : 0.0;
    const double sea = std::abs(b.ocean.*part + b.others.*part + crust - (b.oceanAtStart.*part + crustAtStart)) / std::max(1.0, b.oceanAtStart.*part + crustAtStart);
    const double live = b.live.*part - (b.adopted.*part + f.inflow.*part + f.rain.*part + f.fromBody.*part - f.evaporation.*part - f.outflow.*part);
    const double passed = f.inflow.*part + f.rain.*part + f.fromBody.*part + f.evaporation.*part + f.outflow.*part;
    return std::max(sea, b.unbooked.*part) + std::abs(live) / std::max({1.0, b.live.*part, b.adopted.*part, 1e-5 * passed});
}

} // namespace

WorldBudget budgetOf(World& world) {
    syncLive(world);
    const Overlay& overlay = world.interventions.overlay;
    const Ocean& ocean = world.ocean;
    WorldBudget budget;
    const std::vector<SeaBody> bodies = seaBodies(ocean);
    budget.seaLevel = ocean.pristineLevel;
    for (const SeaBody& body : bodies) {
        add(body.ocean ? budget.ocean : budget.others, body.stock);
        if (body.ocean) budget.seaLevel = body.level;
    }
    for (const Tally& moved : ocean.moved) add(budget.moved, {std::abs(moved.water), std::abs(moved.salt)});
    budget.oceanAtStart = ocean.initial;
    budget.oceanAtStart.salt -= ocean.initialCrust;
    budget.crust = crustOf(ocean.grid);
    budget.crustAtStart = ocean.initialCrust;
    budget.unbooked = ocean.unbooked;

    const double area = specOf(Level::Site).cell * specOf(Level::Site).cell;
    for (const auto& entry : overlay.tiles) {
        const EditedTile& tile = *entry.second;
        const Ground& was = tile.pristine->now;
        for (size_t cell = 0; cell < tile.depth.size(); ++cell) {
            budget.live.water += tile.depth[cell] * area;
            budget.live.salt += tile.salt[cell] * area;
            double change = double(tile.now.rock[cell]) - double(was.rock[cell]);
            for (int c = 0; c < SedimentClasses; ++c) change += double(tile.now.sediment[c][cell]) - double(was.sediment[c][cell]);
            budget.groundChange += change * area;
        }
    }
    budget.adopted = overlay.adopted;
    budget.flows = overlay.water;
    budget.dug = overlay.dug;
    budget.filled = overlay.filled;
    budget.solid = overlay.solid;
    return budget;
}

double waterImbalance(const WorldBudget& budget) { return imbalanceOf(budget, &Tally::water); }
double saltImbalance(const WorldBudget& budget) { return imbalanceOf(budget, &Tally::salt); }

// What the columns gained must be what was built and laid less what was dug and cut, and what
// water picked up or brought in must have settled or left.
double solidImbalance(const WorldBudget& b) {
    const double shaped = b.filled + b.dug + b.solid.deposited + b.solid.eroded, carried = b.solid.carriedIn + b.solid.eroded + b.solid.deposited + b.solid.exported;
    return std::abs(b.groundChange - (b.filled - b.dug + b.solid.deposited - b.solid.eroded)) / std::max(1e-6, shaped) +
           std::abs(b.solid.carriedIn + b.solid.eroded - b.solid.deposited - b.solid.exported) / std::max(1e-6, carried);
}

} // namespace derivean::sim
