#include "landscape/fn/Process.hpp"
#include "measure/fn/Plane.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

// Sediment that reaches standing water settles from the plume (landscape/fn/Plume.cpp): every
// class reaches the bed by the last round where the basin has room for it, and what a full basin
// cannot take leaves as suspended load, so laid plus sent out is what entered.

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "LandscapePlume: " << message << " (" << value << ")\n";
    std::exit(1);
}

constexpr double Volume = 100;   // m³ of each class entering at the middle of the tile

// A lake over the whole domain, `depth` m of water over a level rock floor, open at the rim.
Domain lake(float depth) {
    Domain domain;
    domain.key = {Level::Site, 0, 0};
    domain.cell = specOf(Level::Site).cell;
    allocateBare(domain);
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            domain.hold[i] = x == 0 || z == 0 || x == last || z == last ? Rim : Free;
            domain.ground.rock[i] = -depth;
            domain.surface[i] = -depth;
            domain.level[i] = 0;
        }
    return domain;
}

struct Outcome {
    double laid[SedimentClasses]{};   // m³ on the bed, by class
    double suspended{};               // m³ left in the plume, which leaves with the water as sent out
    bool emptied{};                   // the plume holds nothing and lists no cell after it settled
};

Outcome settle(Domain& domain) {
    FlowScratch scratch;
    for (auto& cloud : scratch.plume) cloud.assign(domain.cells(), 0.0f);
    const size_t middle = domain.index(domain.size / 2, domain.size / 2);
    for (auto& cloud : scratch.plume) cloud[middle] = float(Volume);
    scratch.plumeFed.push_back(int32_t(middle));
    const float area = float(domain.cell * domain.cell);
    settlePlume(domain, scratch, 1.0f, area);
    Outcome settled;
    settled.suspended = domain.ledger.sedimentOut;
    settled.emptied = scratch.plumeFed.empty();
    for (int c = 0; c < SedimentClasses; ++c) {
        for (size_t i = 0; i < domain.cells(); ++i) settled.laid[c] += double(domain.ground.sediment[c][i]) * area;
        for (float volume : scratch.plume[c]) settled.emptied = settled.emptied && volume == 0;
    }
    return settled;
}

} // namespace

void testLandscapePlume() {
    // Deep water: the last round lays what is still suspended of every class, mud included.
    {
        Domain domain = lake(50);
        const Outcome settled = settle(domain);
        for (int c = 0; c < SedimentClasses; ++c)
            expect(std::abs(settled.laid[c] - Volume) < 1e-4 * Volume, "a class did not reach the bed of a deep lake", settled.laid[c] - Volume);
        expect(settled.suspended < 1e-4 * Volume, "sediment stayed suspended in a deep lake", settled.suspended);
        expect(settled.emptied, "the plume of a deep lake was not left empty for the next step");
        expect(std::abs(domain.ledger.deposited - 3 * Volume) < 1e-4 * Volume && domain.ledger.sedimentOut + domain.ledger.sedimentIn < 1e-4 * Volume,
               "the ledger of a deep lake moves sediment across the edge", domain.ledger.sedimentOut + domain.ledger.sedimentIn);
    }
    // A film of water: the bed fills to 2 cm below it and no higher, and the rest stays suspended
    // and leaves, booked as sent out.
    {
        constexpr float Film = 0.03f, Freeboard = 0.02f;
        Domain domain = lake(Film);
        const Outcome settled = settle(domain);
        const double laid = settled.laid[Coarse] + settled.laid[Sand] + settled.laid[Fines];
        expect(laid > 0 && laid < 3 * Volume && settled.suspended > 0, "a film of water took all or none of the plume", laid);
        expect(settled.emptied, "the plume of a film was not left empty for the next step");
        float highest = -1e9f;
        for (size_t i = 0; i < domain.cells(); ++i) highest = std::max(highest, domain.ground.rock[i] + domain.sedimentAt(i));
        expect(highest > -Film && highest <= -Freeboard + 1e-6f, "the plume's bed rose above its water or did not reach it", highest);
        expect(std::abs(domain.ledger.deposited - laid) < 1e-4 * Volume, "the ledger did not book what was laid", domain.ledger.deposited - laid);
        expect(std::abs(domain.ledger.sedimentOut - (3 * Volume - laid)) < 1e-4 * Volume && domain.ledger.sedimentIn == 0, "what the film could not take was not sent out",
               domain.ledger.sedimentOut - (3 * Volume - laid));
    }
}
