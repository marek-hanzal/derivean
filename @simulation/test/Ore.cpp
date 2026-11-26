#include "ore/type/Ore.hpp"
#include "planet/fn/Heat.hpp"

#include <cstdio>
#include <cstdlib>

using namespace derivean::sim;

namespace {

void require(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "ore: %s\n", what);
    std::exit(1);
}

StrataColumn arcColumn(float melt) {
    StrataColumn column;
    column.basement = Basement::Arc;
    column.melt = melt;
    column.meltTime = 4.0e9f;
    column.now = 4.6e9;
    column.fabric = 0.4f;
    return column;
}

} // namespace

void testOre() {
    constexpr uint64_t Seed = 42;
    // Veins exist only in the aureole of a pluton of a province that received melt, they are thin,
    // and a caller that skips columns by nearCuttingRock loses none of them.
    const StrataColumn fed = arcColumn(3000), starved = arcColumn(0);
    long veined = 0, sampled = 0, inAureole = 0, missed = 0, starvedVeins = 0, widest = 0, run = 0;
    for (int line = 0; line < 400; ++line) {
        const double z = 1200.0 + line * 613.0;
        for (int step = 0; step < 4000; ++step) {
            const Vec2 at{step * 0.5 + line * 977.0, z};
            const OreField field = oreFieldAt(Seed, fed, at);
            ++sampled;
            const bool vein = cuttingRock(Seed, field, at, 30.0f, 0) == Material::OreVein;
            run = vein ? run + 1 : 0;
            widest = std::max(widest, run);
            if (!vein) continue;
            ++veined;
            if (std::abs(plutonMargin(Seed, fed, at)) < 0.07) ++inAureole;
            if (!nearCuttingRock(Seed, field, at, 30.0f, 0)) ++missed;
            if (cuttingRock(Seed, oreFieldAt(Seed, starved, at), at, 30.0f, 0) != Material::Air) ++starvedVeins;
        }
    }
    const double share = double(veined) / double(sampled);
    require(veined > 0, "a fed arc has no veins at all");
    require(share < 0.01, "veins fill more than a hundredth of the ground");
    require(inAureole == veined, "a vein lies outside the aureole of its pluton");
    require(missed == 0, "nearCuttingRock skipped a vein");
    require(starvedVeins == 0, "a province without melt has veins");
    require(widest * 0.5 < 12.0, "a vein is wider than a vein");
    // Rock laid down after the fluids moved is not cut.
    require(cuttingRock(Seed, oreFieldAt(Seed, fed, {0, 0}), {0, 0}, 30.0f, 4.2e9) == Material::Air, "a vein cuts younger cover");
    std::printf("ore: veins fill %.3f %% of fed arc ground, widest crossing %.1f m\n", 100.0 * share, widest * 0.5);
}
