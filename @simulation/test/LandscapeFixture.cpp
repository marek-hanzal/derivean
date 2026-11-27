#include "LandscapeFixture.hpp"

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

#include <cstdlib>
#include <future>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double FarAge = 8e9;   // yr; the present of the game is 4.6e9

std::future<PlanetSummary>& farEvolution() {
    static std::future<PlanetSummary> running = std::async(std::launch::async, [] {
        Workers workers(1);
        return summarize(evolvePlanet(42, FarAge, workers));
    });
    return running;
}

} // namespace

World& testWorld() {
    static World world({42, 1.2e9, 0});
    return world;
}

World& testWorldPresent() {
    static World world({42, 4.6e9, 0});
    return world;
}

void startFarFuture() { farEvolution(); }

const PlanetSummary& farFuture() {
    static const PlanetSummary summary = farEvolution().get();
    return summary;
}

Vec2 testFocus() {
    static const Vec2 focus = [] {
        const Planet& planet = testWorld().planet;
        // A Local tile near the middle of a Region tile has all its parent cells in that one tile,
        // and a Site tile next to the middle of a Local tile likewise. Of the first-pass Local tiles
        // so placed in first-pass Region tiles, the one the landscape will see highest is taken:
        // landscapes read the planet through planetPoint, so that is where the height is asked.
        const int perRegion = int(tileSpan(Level::Region) / tileSpan(Level::Local));
        Vec2 best{};
        double highest = -1e30;
        for (int rz = 0; rz < int(PlanetSpan / tileSpan(Level::Region)); rz += 2)
            for (int rx = 0; rx < int(PlanetSpan / tileSpan(Level::Region)); rx += 2)
                for (int lz = 4; lz <= perRegion - 6; lz += 2)
                    for (int lx = 4; lx <= perRegion - 6; lx += 2) {
                        const Vec2 centre{(rx * perRegion + lx + 0.5) * tileSpan(Level::Local), (rz * perRegion + lz + 0.5) * tileSpan(Level::Local)};
                        const double height = samplePlanet(planet.elevation, planetPoint(planet.seed, centre));
                        if (height > highest) highest = height, best = centre;
                    }
        const TileKey site = tileAt(Level::Site, best);
        return Vec2{((site.x & ~1) + 0.5) * tileSpan(Level::Site), ((site.z & ~1) + 0.5) * tileSpan(Level::Site)};
    }();
    return focus;
}

Vec2 testSpot() { return testFocus() + Vec2{0.5, 0.5} * tileSpan(Level::Ground); }

void expectLandscape(bool passed, const char* message, double value) {
    if (passed) return;
    std::cerr << "Landscape: " << message << " (" << value << ")\n";
    std::exit(1);
}
