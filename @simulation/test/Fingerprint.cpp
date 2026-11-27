#include "LandscapeFixture.hpp"
#include "StoredPrints.hpp"

#include "measure/fn/Fingerprint.hpp"
#include "simulation/fn/Query.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace derivean::sim;

// A few tiles of the shared worlds, hashed over every field they keep, against the prints stored
// for this platform's maths library in test/StoredPrints.hpp. A change meant to be bit for bit
// leaves every print; a change meant to move the world takes the lines this test prints instead.

namespace {

// The two platforms the game runs on; Linux is not one of them.
#if defined(_WIN32)
constexpr const char* Platform = "windows";
#else
constexpr const char* Platform = "macos";
#endif

void checkPrints(World& world, const char* worldName, const std::vector<TileKey>& keys) {
    std::vector<uint64_t> found;
    size_t stored = 0, moved = 0;
    for (const TileKey& key : keys) {
        const uint64_t print = fingerprintOf(*world.atlas.require(key));
        found.push_back(print);
        for (const StoredPrint& s : StoredPrints) {
            if (std::strcmp(s.platform, Platform) != 0 || std::strcmp(s.world, worldName) != 0 || s.level != int(key.level) || s.x != key.x || s.z != key.z) continue;
            ++stored;
            if (s.print != print) ++moved;
        }
    }
    if (stored == keys.size() && moved == 0) return;
    std::fprintf(stderr, "fingerprint: %zu of %zu tiles of the %s world moved, %zu have no print stored for %s; if the world is meant to move, these are the lines:\n",
                 moved, keys.size(), worldName, keys.size() - stored, Platform);
    for (size_t k = 0; k < keys.size(); ++k)
        std::fprintf(stderr, "    {\"%s\", \"%s\", %d, %d, %d, 0x%016llxULL},\n", Platform, worldName, int(keys[k].level), keys[k].x, keys[k].z,
                     static_cast<unsigned long long>(found[k]));
    std::exit(1);
}

} // namespace

// The young world around the test focus: one tile of every level, the Ground tile of the second
// pass that testGroundTile builds.
void testFingerprintWorld() {
    World& world = testWorld();
    const Vec2 focus = testFocus();
    TileKey ground = tileAt(Level::Ground, focus);
    ground.x |= 1, ground.z &= ~1;
    requestAround(world, {(ground.x + 1.5) * tileSpan(Level::Ground), (ground.z + 0.5) * tileSpan(Level::Ground)}, 1.0, Level::Ground);
    checkPrints(world, "young", {tileAt(Level::Region, focus), tileAt(Level::Local, focus), tileAt(Level::Site, focus), ground});
}

// The present world: the coast tile of testReliefLedger and the glaciated Site tile of
// testReliefGlaciatedSite, with the Region tiles under them.
void testFingerprintPresent() {
    World& world = testWorldPresent();
    const TileKey coast{Level::Local, 705, 261}, glaciated{Level::Site, 5474, 9868};
    const auto regionUnder = [](const TileKey& key) {
        const double span = tileSpan(key.level);
        return tileAt(Level::Region, {(key.x + 0.5) * span, (key.z + 0.5) * span});
    };
    checkPrints(world, "present", {regionUnder(coast), coast, regionUnder(glaciated), glaciated});
}
