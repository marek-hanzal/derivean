#include "core/service/Workers.hpp"
#include "landscape/fn/Process.hpp"
#include "measure/constant/Angle.hpp"
#include "relief/type/Form.hpp"
#include "volcanic/fn/Emplace.hpp"
#include "volcanic/fn/Minor.hpp"
#include "volcanic/fn/Proving.hpp"
#include "volcanic/fn/Volcanic.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double Km3 = 1e9;

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Volcanic: " << message << " (" << value << ")\n";
    std::exit(1);
}

Emplacement plain(int side, double cell, double fall) {
    Emplacement field;
    field.resize(side, side);
    field.cell = cell;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) field.ground[field.index(x, z)] = float(1000.0 - fall * cell * x);
    return field;
}

double laidVolume(const Emplacement& field) {
    double sum = 0;
    for (const float layer : field.laid) sum += layer;
    return sum * field.cell * field.cell;
}

// Degrees of the mean fall from the ring `inner` cells out to the ring `outer` cells out.
double flank(const Emplacement& field, int x, int z, int inner, int outer) {
    double near = 0, far = 0;
    for (int k = 0; k < 8; ++k) {
        near += field.ground[field.index(x + NeighborX[k] * inner, z + NeighborZ[k] * inner)] / 8;
        far += field.ground[field.index(x + NeighborX[k] * outer, z + NeighborZ[k] * outer)] / 8;
    }
    double run = 0;
    for (int k = 0; k < 8; ++k) run += NeighborReach[k] * (outer - inner) * field.cell / 8;
    return std::atan((near - far) / run) * DegreesPerRadian;
}

// The laws by themselves, on plain grids.
void testLaws() {
    // Fluid lava on a plain: a low shield. Fragments falling around a vent: a cone at their angle of rest.
    Emplacement shield = plain(129, 1024, 0);
    pourLava(shield, BasaltLava, 64, 64, 60 * Km3);
    const double shieldTop = shield.ground[shield.index(64, 64)] - 1000;
    expect(shieldTop > 150 && shieldTop < 1500, "a fluid vent on a plain does not build a low shield", shieldTop);
    expect(flank(shield, 64, 64, 2, 8) < 6, "the flank of a shield is steeper than six degrees", flank(shield, 64, 64, 2, 8));
    expect(std::abs(laidVolume(shield) + shield.lost - 60 * Km3) < 1e-4 * 60 * Km3, "lava on a plain was lost or made", laidVolume(shield) / Km3);

    Emplacement cone = plain(129, 64, 0);
    buildCone(cone, coneFall(cone.center(64, 64), 0.5 * Km3), ReposeTan[Sand]);
    const double rest = std::atan(ReposeTan[Sand]) * DegreesPerRadian, upper = flank(cone, 64, 64, 3, 7);
    expect(upper > rest - 5 && upper < rest + 1.5, "the upper slopes of a cone of fall do not stand at the angle of rest", upper);
    expect(std::abs(laidVolume(cone) + cone.lost - 0.5 * Km3) < 1e-4 * 0.5 * Km3, "the fall of a cone was lost or made", laidVolume(cone) / Km3);
    expect(cone.ground[cone.index(64, 64)] - 1000 > 4 * shieldTop * 64 / 1024, "a cone of fall is no steeper than a shield");

    // Lava that enters a valley keeps to its floor and stops before the valley ends.
    Emplacement valley = plain(160, 64, 0.03);
    for (int z = 0; z < 160; ++z)
        for (int x = 0; x < 160; ++x) valley.ground[valley.index(x, z)] -= float(std::max(0.0, 120.0 - 12.0 * std::abs(z - 80)));
    Rheology flow = BasaltLava;
    flow.coolLength = 5000;
    pourLava(valley, flow, 20, 80, 0.02 * Km3);
    double inside = 0, farthest = 0;
    for (int z = 0; z < 160; ++z)
        for (int x = 0; x < 160; ++x) {
            const float layer = valley.laid[valley.index(x, z)];
            if (layer <= 0) continue;
            if (std::abs(z - 80) <= 10) inside += layer;
            farthest = std::max(farthest, double(x - 20) * 64);
        }
    expect(inside * 64 * 64 > 0.98 * laidVolume(valley), "lava left the valley it entered", inside * 64 * 64 / laidVolume(valley));
    expect(valley.lost == 0 && farthest > 1500 && farthest <= 5000 + 64, "lava in a valley did not run along its floor and stop", farthest);

    // One eruption seen from two grids that share an edge: on the second the vent stands on ground
    // the first has finished. Both must lay the same on the cells next to the edge.
    Emplacement whole = plain(128, 1024, 0.002), part = plain(96, 1024, 0.002);
    part.origin = {32 * 1024.0, 0};
    for (int z = 0; z < 96; ++z)
        for (int x = 0; x < 96; ++x) {
            part.ground[part.index(x, z)] = whole.ground[whole.index(x + 32, z)];
            if (x < 32) part.rest[part.index(x, z)] = Rest::Passing;   // columns 32..63 of the whole grid
        }
    pourLava(whole, AndesiteLava, 58, 48, 30 * Km3);
    pourLava(part, AndesiteLava, 58 - 32, 48, 30 * Km3);
    double own = 0, other = 0, apart = 0;
    for (int z = 0; z < 96; ++z)
        for (int x = 64; x < 80; ++x) {
            const double a = whole.laid[whole.index(x, z)], b = part.laid[part.index(x - 32, z)];
            own += a, other += b, apart += std::abs(a - b);
        }
    expect(own > 0 && apart < 0.02 * own, "an eruption seen from two neighbouring grids differs on their shared edge", apart / std::max(1.0, own));
}

// Ground of one height with a pile on it, on a lattice that starts at the given node.
GroundLattice pileGround(int64_t origin, int count) {
    GroundLattice lattice;
    lattice.originX = lattice.originZ = origin;
    lattice.countX = lattice.countZ = count;
    const size_t nodes = size_t(count) * size_t(count);
    lattice.rock.assign(nodes, 100.0f);
    lattice.datum.assign(nodes, 80.0f);
    for (auto& layer : lattice.cover) layer.assign(nodes, 0.0f);
    lattice.crust.assign(nodes, 0.0f);
    lattice.crustKind.assign(nodes, uint8_t(Crust::None));
    lattice.coarseKind.assign(nodes, uint8_t(Material::Gravel));
    lattice.fineKind.assign(nodes, uint8_t(Material::Silt));
    lattice.bare.assign(nodes, 0);
    lattice.pile.assign(nodes, PilePoint{30.0f, 1.0f, Material::Andesite, 7});
    smoothLie(lattice);
    return lattice;
}

// What a column of voxels is made of where a pile lies on the layering and where a plug cuts both,
// and that a point reads the same whichever lattice it is asked through.
void testColumn() {
    FormSetting setting;
    setting.seed = 42;
    setting.strata.now = 1e9;
    setting.strata.thickness[0] = setting.strata.coverTotal = 500;
    setting.strata.facies[0] = Facies{0, 255, 0, 0, 0, 0};
    VolcanicTile volcanic;
    Vent vent;
    vent.magma = Magma::Andesite;
    vent.feeders.push_back({{20.0, 8.0}, 1.5f, 0, 0, 0});
    volcanic.vents.push_back(vent);
    setting.volcanic = &volcanic;
    const GroundLattice near = pileGround(0, 120), shifted = pileGround(-7, 140);
    constexpr int Count = 90;
    Material column[Count], again[Count], neck[Count];
    formColumn(setting, near, 0, nullptr, 8.1, 8.3, 58, 0.5, Count, column, 1);
    formColumn(setting, shifted, 0, nullptr, 8.1, 8.3, 58, 0.5, Count, again, 1);
    formColumn(setting, near, 0, nullptr, 20.4, 8.2, 58, 0.5, Count, neck, 1);
    for (int k = 0; k < Count; ++k) {
        const double y = 58 + (k + 0.5) * 0.5;
        expect(column[k] == again[k], "a point in a pile reads differently through another lattice", y);
        if (y > 100.5) expect(column[k] == Material::Air && neck[k] == Material::Air, "rock stands above the ground of a pile", y);
        else if (y > 70.5 && y < 99.5) expect(column[k] == Material::Andesite, "a pile of lava is not lava", y);
        // Sandstone, with the shale seams its layers carry.
        else if (y < 69.5) expect(column[k] == Material::Sandstone || column[k] == Material::Shale, "the layering under a pile is not read below its floor", y);
        if (y < 99.5) expect(neck[k] == Material::Andesite, "the plug of a conduit does not cut pile and layering", y);
    }
}

} // namespace

void testVolcanicJoints();
void testVolcanicCollapse();
void testVolcanicPile();

// A vent on a plane of a region, through the level's own replay.
void testVolcanicLaws() {
    testLaws();
    testColumn();
    testVolcanicJoints();
    testVolcanicCollapse();
    testVolcanicPile();

    ProvingSpec spec;
    spec.ground.level = Level::Region;
    spec.ground.slope = 0.002, spec.ground.roughness = 8, spec.ground.rain = 0.5, spec.ground.rock = Material::Granite;
    spec.magma = Magma::Basalt, spec.volume = 60 * Km3, spec.episodes = 3;
    const Domain shield = replayProving(spec);
    const EdificeShape low = measureEdifice(gridOf(shield), shield.volcanic.vents.front().centre, 60e3);
    expect(low.height > 150 && low.midSlope < 6, "a basalt vent on a region's plain does not stand as a low shield", low.midSlope);

    spec.magma = Magma::Andesite;
    const Domain cone = replayProving(spec);
    const EdificeShape high = measureEdifice(gridOf(cone), cone.volcanic.vents.front().centre, 60e3);
    expect(high.height > 2 * low.height && high.midSlope > 2 * low.midSlope, "an andesite vent builds no steeper and higher than a basalt one", high.midSlope);
    expect(high.footSlope < high.midSlope, "the flank of an andesite cone is not concave", high.footSlope);

    // Everything a vent brought up is lava, fall or ash on the ground, or is booked as having left;
    // and what the pile has lost since is no more than erosion booked.
    const VolcanicLedger& ledger = cone.volcanic.ledger;
    const double accounted = ledger.lava + ledger.cone + ledger.ash + ledger.left + ledger.drifted;
    expect(std::abs(ledger.erupted - accounted) < 1e-6 * ledger.erupted, "the erupted volume is not the volume laid plus what left", (ledger.erupted - accounted) / Km3);
    // The region leaves the small eruptions of young episodes to the local level.
    double due = 0;
    for (const Episode& episode : cone.volcanic.vents.front().episodes) due += episode.volume - minorVolume(cone.volcanic.vents.front(), episode);
    expect(std::abs(ledger.erupted - due) < 1e-6 * due, "a vent did not erupt what its episodes say", ledger.erupted / Km3);
    const TileLedger& land = cone.ledger;
    expect(high.pile <= ledger.lava + ledger.cone + ledger.sealed + 1.0, "the pile holds more than was laid", high.pile / Km3);
    expect(ledger.lava + ledger.cone - high.pile <= land.eroded + land.weathered + land.failed + land.dissolved + 0.02 * (ledger.lava + ledger.cone),
           "the pile lost more than erosion booked", (ledger.lava + ledger.cone - high.pile) / Km3);

    // A single great eruption empties the chamber: the summit founders by what came out.
    spec.paroxysm = 40 * Km3;
    const Domain caldera = replayProving(spec);
    const EdificeShape pit = measureEdifice(gridOf(caldera), caldera.volcanic.vents.front().centre, 60e3);
    expect(std::abs(caldera.volcanic.ledger.foundered - 40 * Km3) < 1e-3 * 40 * Km3, "the roof did not go down by the volume that left the chamber", caldera.volcanic.ledger.foundered / Km3);
    // It founders at the end of its step and that step's erosion follows: at a region's kilometre
    // cells slope failure widens the ring wall into a funnel and part of the pit fills. The wall as
    // steep as rock holds is the local level's to draw.
    expect(pit.pit > 100 && pit.pitRadius >= 2000 && pit.pitRadius <= 10000, "a great eruption left no caldera", pit.pit);
    // The region records the pit it cut, and at the end how much of it is left: a share of it,
    // since erosion followed, but not none, since the pit is there.
    expect(caldera.volcanic.calderas.size() == 1 && caldera.volcanic.calderas.front().cut > 100, "a region did not record the caldera it cut", caldera.volcanic.calderas.size());
    Domain finished = caldera;
    const std::vector<Caldera> kept = finishCalderas(finished);
    expect(kept.size() == 1 && kept.front().kept > 0.2f && kept.front().kept <= 1.0f && kept.front().before.empty(), "a region's record of a caldera says no pit is left", kept.empty() ? -1 : kept.front().kept);

    // The pool only spreads what a replay may spread: the ground is the same to the last bit.
    spec.paroxysm = 0, spec.episodes = 2;
    Workers pool(4);
    const Domain alone = replayProving(spec), shared = replayProving(spec, &pool);
    expect(alone.ground.rock == shared.ground.rock && alone.volcanic.floor == shared.volcanic.floor, "a replay with a vent depends on the number of threads");
}
