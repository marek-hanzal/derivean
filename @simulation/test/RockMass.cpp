#include "rockmass/type/RockMass.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace derivean::sim;

namespace {

void require(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "rock mass: %s\n", what);
    std::exit(1);
}

StrataColumn plainColumn() {
    StrataColumn column;
    column.basement = Basement::Craton;
    column.fabric = 0.5f;
    column.foldSpan = QuietFoldSpan;
    column.coverTotal = 1200;
    column.now = 4.6e9;
    return column;
}

} // namespace

void testRockMass() {
    constexpr uint64_t Seed = 42;
    const StrataColumn column = plainColumn();
    const Vec2 origin{311000, -127000};
    const RockMass sandstone = rockMassAt(Seed, column, Bed{40, 64, Material::Sandstone}, 4.2e9, origin);
    const RockMass shale = rockMassAt(Seed, column, Bed{40, 44, Material::Shale}, 4.2e9, origin);
    const RockMass granite = rockMassAt(Seed, column, Bed{0, 1e9f, Material::Granite}, 0, origin);

    // Strength and block size follow the rock: a thick sandstone bed stands, shale crumbles.
    require(sandstone.block > 10 * shale.block, "thick sandstone does not break into far larger blocks than shale");
    require(sandstone.cohesion > 10 * shale.cohesion && granite.cohesion > sandstone.cohesion, "mass cohesion does not follow the rock");
    require(sandstone.dip < 0.15f, "undeformed cover is not near horizontal");

    // Planes are planes: crossing a set at right angles meets its joints about one spacing apart,
    // and the distance to them changes no faster than the walker moves.
    const JointSet& set = sandstone.sets[0];
    const Vec2 normal{-std::sin(set.strike), std::cos(set.strike)};
    int crossings = 0;
    float before = jointDistance(Seed, sandstone, 0, 0, origin, 30);
    bool falling = false;
    constexpr double Step = 0.25, Walk = 4000;
    for (double walked = Step; walked < Walk; walked += Step) {
        const float now = jointDistance(Seed, sandstone, 0, 0, origin + normal * walked, 30);
        if (now < 1e8f && before < 1e8f) {
            require(std::abs(now - before) <= Step * 1.2 + 1e-3, "distance to a joint jumps");
            if (falling && now > before && before < 0.5f * Step + 0.3f) ++crossings;
            falling = now < before;
        }
        before = now;
    }
    const double expected = Walk / set.spacing * set.persistence;
    require(crossings > 0.5 * expected && crossings < 1.6 * expected, "joints are not about one spacing apart");

    // One field at every width: a line of weakness a 64 m cell sees is one a 4 m cell sees too.
    int coarse = 0, agreed = 0;
    for (int sample = 0; sample < 4000; ++sample) {
        const Vec2 at = origin + Vec2{sample * 37.0, (sample % 97) * 53.0};
        if (fractureShare(Seed, sandstone, at, 30, 64) < 0.05f) continue;
        ++coarse;
        float fine = 0;
        for (int dz = -8; dz <= 8; ++dz)
            for (int dx = -8; dx <= 8; ++dx) fine = std::max(fine, fractureShare(Seed, sandstone, at + Vec2{dx * 4.0, dz * 4.0}, 30, 4));
        agreed += fine > 0.3f;
    }
    require(coarse > 20, "a 64 m cell never sees a line of weakness in jointed sandstone");
    require(agreed == coarse, "a corridor seen at 64 m is missing at 4 m");
    std::printf("rock mass: sandstone joints %.1f m and %.1f m apart, blocks %.1f m, mass cohesion %.0f kPa; shale blocks %.2f m, %.0f kPa; granite %.0f kPa\n",
                set.spacing, sandstone.sets[1].spacing, sandstone.block, sandstone.cohesion, shale.block, shale.cohesion, granite.cohesion);
}
