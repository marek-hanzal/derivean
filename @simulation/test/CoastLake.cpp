// Lake shores: sorting by the lake's own waves, the beach laws against their sources, and a bank of
// sand going back at the rate of wave impact.
#include "coast/fn/Coast.hpp"
#include "coast/fn/Scenario.hpp"

#include "CoastFixture.hpp"

#include <algorithm>
#include <cmath>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) { expectCoast(passed, message, value); }

void lakeBeach() {
    CoastSpec spec;
    spec.scene = CoastScene::Lake, spec.level = Level::Site;
    spec.rock = Material::Granite;
    spec.sand = 1.0, spec.gravel = 0.6;
    spec.steps = 30;
    const Domain lake = worked(spec, spec.steps);
    // Share of gravel in the loose cover lying between `low` and `high` m about still water on the
    // downwind (exposed, side 0) or upwind (sheltered, side 1) shore of the lake; 0.375 was laid.
    const int middle = lake.size / 2;
    const auto share = [&](int side, float low, float high) {
        double coarse = 0, loose = 0;
        for (int z = 1; z < lake.size - 1; ++z)
            for (int x = middle - 30; x <= middle + 30; ++x) {
                const size_t i = lake.index(x, z);
                const float water = lake.shore->still[i], above = lake.ground.rock[i] + lake.sedimentAt(i) - water;
                if (water == NoWater || (z < middle) != (side == 1) || above < low || above >= high) continue;
                coarse += lake.ground.sediment[Coarse][i], loose += lake.ground.sediment[Coarse][i] + lake.ground.sediment[Sand][i];
            }
        return loose > 1 ? coarse / loose : 0.0;
    };
    // The exposed shore: over the stand its waves have carried the sand of the ground they work into
    // deeper water and left the gravel, a lag coarser than what was laid.
    const double lag = share(0, -1.0f, 0.0f);
    expect(lag > 0.42, "the waves of an exposed lake shore left no gravel lag", lag);
    // The sheltered shore sees only the storms of the other quarters over the lake's own fetch: they
    // sort its shoreface, gravel above sand (its face stands 40^¼ ≈ 2.5 times as steep, Beach.cpp).
    const double upper = share(1, -0.6f, 0.0f), lower = share(1, -2.0f, -1.0f);
    expect(upper > lower + 0.015, "gravel does not lie above sand on a lake beach", upper - lower);
    // The swash throws gravel up as readily as sand and the backwash takes only the sand back
    // down: on both shores the berm above still water is of gravel more than what was laid.
    expect(share(0, 0.0f, 0.4f) > 0.42 && share(1, 0.0f, 0.4f) > 0.42, "the berm of a mixed lake beach is not its gravel", std::min(share(0, 0.0f, 0.4f), share(1, 0.0f, 0.4f)));
    float highest = 0;
    for (size_t i = 0; i < lake.cells() * WaveDirections; ++i) highest = std::max(highest, lake.shore->wave[i]);
    expect(highest > 0.01f && highest < 0.25f, "a lake 560 m across carries waves of the open sea", std::sqrt(highest));
}

// The beach laws against their sources: Sunamura's face, flatter under higher waves and steeper on
// coarser grains, and Stockdon's run-up.
void beachLaws() {
    const float sand = faceSlope(Sand, 0.3f), high = faceSlope(Sand, 1.0f), gravel = faceSlope(Coarse, 0.3f);
    expect(std::abs(sand - 0.0859f) < 0.002f && std::abs(high - 0.0638f) < 0.002f, "the face of sand is not Sunamura's", sand);
    expect(std::abs(gravel / sand - std::pow(40.0f, 0.25f)) < 0.01f, "gravel's face does not stand steeper by the fourth root of its grain over sand's", gravel / sand);
    expect(std::abs(runUp(1.0f, 0.05f) - 0.298f) < 0.005f, "the run-up of a metre of sea on a gentle beach is not Stockdon's", runUp(1.0f, 0.05f));
    expect(profileSlope(Sand, 0.09f, 0.3f, sand) == sand && profileSlope(Sand, 2.0f, 0.3f, sand) < 0.5f * sand, "a beach face does not end at its step", profileSlope(Sand, 2.0f, 0.3f, sand));
}

// An 18° bank of deep sand on the downwind shore of a lake scene after the stand, over 36 columns:
// m³ a year the waves took from what stood above high water, per metre of shore, and m its scarp
// (ground a metre over high water) went back.
struct Bank { double taken, back; };
Bank bankAfter(double wind, int steps) {
    CoastSpec spec;
    spec.scene = CoastScene::Lake, spec.level = Level::Site;
    spec.rock = Material::Granite;
    spec.sand = 100, spec.bank = 0.325, spec.cover = 120, spec.wind = wind, spec.steps = steps;
    const Domain start = coastDomain(spec);
    const Domain lake = worked(spec, spec.steps);
    const int middle = lake.size / 2;
    const float high = lake.shore->still[lake.index(middle, middle)] + LakeSwing;
    const auto top = [](const Domain& d, size_t i) { return d.ground.rock[i] + d.sedimentAt(i); };
    Bank bank{};
    for (int x = middle + 5; x <= middle + 40; ++x) {
        int was = middle, now = middle;
        while (was < lake.size - 1 && top(start, start.index(x, was)) < high + 1.0f) ++was;
        while (now < lake.size - 1 && top(lake, lake.index(x, now)) < high + 1.0f) ++now;
        bank.back += double(now - was) * lake.cell / 36;
        for (int z = middle; z < lake.size; ++z) {
            const size_t i = lake.index(x, z);
            if (top(start, i) > high) bank.taken += std::max(0.0f, top(start, i) - top(lake, i)) * float(lake.cell) / 36 / StandTime;
        }
    }
    return bank;
}

// A bank of sand goes back at the rate of wave impact (Larson et al. 2004): the swash of a 0.26 m
// lake sea striking it head-on 0.11 m over high water takes 1.27 m³ a metre a year in its share of
// the year, the trains at half a right angle less; sand laid back at its foot slows it. Faster under
// a stronger wind, and as far whether the stand is cut into 30 steps or 45, whose sweeps stand for
// 25 and 22 years (they differ by some hundredths; sweeps of 40 years and more fall short). At 4 m
// cells the beach face and berm are smaller than a cell: the beach laws are checked by beachLaws.
void sandBank() {
    const Bank calm = bankAfter(4, 30), coarse = bankAfter(12, 30), fine = bankAfter(12, 45);
    expect(coarse.taken > 0.3 && coarse.taken < 2.0, "a bank of sand does not go back at the rate of wave impact", coarse.taken);
    expect(std::abs(coarse.back - fine.back) < 0.2 * fine.back, "how far a bank goes back depends on the sweeps its time is cut into", coarse.back / fine.back);
    expect(coarse.taken > 1.4 * calm.taken, "a stronger wind does not cut a bank faster", coarse.taken / calm.taken);
}

} // namespace

void testCoastLakes() {
    lakeBeach();
    beachLaws();
    sandBank();
}
