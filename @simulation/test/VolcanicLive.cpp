#include "chronology/type/Calendar.hpp"
#include "volcanic/fn/Outbreak.hpp"
#include "volcanic/fn/Source.hpp"
#include "volcanic/fn/Unrest.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "VolcanicLive: " << message << " (" << value << ")\n";
    std::exit(1);
}

Vent supplied(Magma magma, double supply) {
    Vent vent;
    vent.id = 77;
    vent.magma = magma;
    vent.until = 4.6e9;
    vent.supply = supply;
    vent.held = 0.3 * supply * 100;
    vent.chamber = chamberOf(supply);
    vent.count = 500;
    vent.recent = {{vent.until - 900, 2e6}, {vent.until - 200, 5e6}};
    vent.feeders.push_back({{1000.0, 2000.0}, 150.0f, vent.until - 1e5, vent.until, 1e9});
    return vent;
}

} // namespace

// Eruptions during play: the vent's own sequence in calendar time, and what each puts into the air.
void testVolcanicLive() {
    constexpr uint64_t Seed = 42;
    const Vent vent = supplied(Magma::Andesite, 5e5);

    // The same eruptions whatever window asks, however it is cut.
    std::vector<Outbreak> whole, first, second;
    outbreaksBetween(Seed, vent, vent.until - 1000, vent.until + 100000, whole);
    outbreaksBetween(Seed, vent, vent.until - 1000, vent.until + 37300, first);
    outbreaksBetween(Seed, vent, vent.until + 37300, vent.until + 100000, second);
    expect(whole.size() > 10 && whole.size() == first.size() + second.size(), "a window cut in two holds other eruptions", double(whole.size()));
    for (size_t k = 0; k < whole.size(); ++k) {
        const Outbreak& b = k < first.size() ? first[k] : second[k - first.size()];
        expect(whole[k].number == b.number && whole[k].time == b.time && whole[k].volume == b.volume, "an eruption differs with the window that asked for it", double(k));
        if (k > 0) expect(whole[k].number == whole[k - 1].number + 1 && whole[k].time >= whole[k - 1].time, "eruptions are out of order", double(k));
    }
    expect(whole.front().time == vent.recent.front().time, "the record's last eruptions are not the first in play");

    // Over a long span the vent brings up what it is supplied with: no more, and short of it by no
    // more than the chamber holds at the end.
    std::vector<Outbreak> long_;
    const double span = 2e5;
    outbreaksBetween(Seed, vent, vent.until, vent.until + span, long_);
    double brought = 0, largest = 0;
    for (const Outbreak& outbreak : long_) brought += outbreak.volume, largest = std::max(largest, outbreak.volume);
    const double supply = vent.supply * span + vent.held;
    expect(brought <= supply * (1 + 1e-9) && brought >= supply - 2 * largest, "the eruptions do not bring up what the vent was supplied with", brought / supply);
    const double perYear = double(long_.size()) / span, expected = vent.supply / meanEruption(vent);
    expect(std::abs(perYear / expected - 1) < 0.25, "a vent erupts at another rate than its supply and mean eruption give", perYear / expected);
    // The sequence's own mean is the mean of the law, and no eruption is larger than the chamber.
    double drawn = 0, most = 0;
    for (int64_t number = 0; number < 200000; ++number) {
        const double volume = eruptionVolume(Seed, vent, number);
        drawn += volume, most = std::max(most, volume);
    }
    expect(std::abs(drawn / 200000 / meanEruption(vent) - 1) < 0.02 && most <= vent.chamber, "the eruptions are not drawn by the chamber's law", drawn / 200000 / meanEruption(vent));

    // The unrest of the chamber names the eruption that comes next.
    const double now = vent.until + 5000;
    const Unrest unrest = ventUnrest(Seed, vent, now);
    std::vector<Outbreak> after;
    outbreaksBetween(Seed, vent, now, now + 1e6, after);
    expect(unrest.supplied && !after.empty() && std::abs(now + unrest.untilNext - after.front().time) < 1e-6 && after.front().volume == unrest.nextVolume,
           "the unrest of a chamber does not name its next eruption", unrest.untilNext);

    // A vent whose melt has stopped keeps its record and erupts no more.
    Vent spent = vent;
    spent.supply = 0;
    std::vector<Outbreak> none;
    outbreaksBetween(Seed, spent, vent.until, vent.until + 1e6, none);
    expect(none.empty(), "a vent without supply erupts", double(none.size()));

    // An eruption below the collapse of its chamber throws fragments, then pours lava.
    std::vector<AirSource> sources;
    airSourcesOf(outbreakOf(Seed, vent, 4, vent.until + 10, 1e9), vent.until, 1500, sources);
    expect(sources.size() == 2, "an eruption of fragments and lava is not two sources", double(sources.size()));
    // Into the air: the ash the column carries, and the gas of all the melt. Pinatubo took most of
    // a chamber of a few cubic kilometres and left a caldera: all of it fragments.
    const Outbreak great = outbreakOf(Seed, vent, 3, vent.until + 10, 5e9);
    sources.clear();
    airSourcesOf(great, vent.until, 1500, sources);
    expect(great.empties && sources.size() == 1, "a chamber-emptying eruption is not one column", double(sources.size()));
    double ash = 0, sulphur = 0;
    for (const AirSource& source : sources) {
        const double seconds = source.end - source.start;
        expect(seconds > 0 && source.start >= 10 * YearSeconds - 1, "a source does not start with its eruption", source.start);
        for (const double grain : source.ash) ash += grain * seconds;
        sulphur += source.sulphur * seconds;
    }
    expect(std::abs(ash - rockMass(great.ash + 0.3 * great.flows)) < 1e-6 * ash, "the ash in the air is not the ash the eruption threw", ash);
    expect(std::abs(sulphur - rockMass(great.volume) * 1.3e-3) < 1e-6 * sulphur, "the gas in the air is not the gas of the melt", sulphur);
    // Pinatubo brought up about 5 km³ of dense rock in a column 35 km high within a day or two.
    expect(great.column > 2e4 && great.column < 4e4 && great.blast < 5 * 86400, "a great eruption has no plinian column", great.column);
    expect(sources[0].height == 1500 + great.column, "a column does not rise from the vent's ground", sources[0].height);

    // A small basalt eruption: lava for weeks, a low plume, little in the air.
    const Vent fluid = supplied(Magma::Basalt, 1e5);
    const Outbreak flow = outbreakOf(Seed, fluid, 9, fluid.until + 1, 1e8);
    expect(!flow.empties && flow.effusion > 7 * 86400 && flow.effusion < 120 * 86400, "a basalt flow does not last weeks", flow.effusion / 86400);
    expect(flow.plume > 500 && flow.plume < 8000 && flow.column < 1e4, "the plume over basalt lava is not low", flow.plume);
    expect(length(flow.site - fluid.feeders.back().site) < 150, "an eruption opens away from its conduit", length(flow.site - fluid.feeders.back().site));

    // A chamber-emptying eruption is all fragments and founders its roof.
    const Outbreak caldera = outbreakOf(Seed, vent, 11, vent.until + 1, vent.chamber);
    expect(caldera.empties && caldera.lava == 0 && caldera.effusion == 0, "a chamber-emptying eruption pours lava or leaves the roof standing", caldera.lava);

    // On the ground: a small andesite eruption on live site cells lays its cone, its currents and
    // its lava, and nothing is lost or made.
    Emplacement ground;
    ground.resize(400, 400);
    ground.cell = 4;
    for (int z = 0; z < 400; ++z)
        for (int x = 0; x < 400; ++x) ground.ground[ground.index(x, z)] = float(500.0 - 0.05 * 4 * x);
    Vent small = vent;
    small.feeders.back().site = ground.center(200, 200);
    const Outbreak little = outbreakOf(Seed, small, 21, vent.until + 3, 2e6);
    const NearField near = layOutbreak(ground, little);
    double laid = 0, rock = 0;
    for (size_t i = 0; i < ground.cells(); ++i) laid += near.fragments[i] + near.lava[i], rock += near.lava[i];
    laid *= 16, rock *= 16;
    const double thrown = little.fall + little.flows + little.lava;
    expect(std::abs(laid + near.left - thrown) < 1e-3 * thrown, "an eruption on live ground lost or made rock", (laid + near.left) / thrown);
    expect(std::abs(rock - little.lava) < 0.02 * little.lava + near.left, "the lava of an eruption on live ground is not its lava", rock / little.lava);
    const size_t middle = ground.index(int((little.site.x) / 4), int((little.site.z) / 4));
    expect(near.fragments[middle] + near.lava[middle] > 5, "an eruption on live ground built nothing over its vent", near.fragments[middle] + near.lava[middle]);
}
