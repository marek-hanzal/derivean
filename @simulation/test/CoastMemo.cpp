#include "CoastFixture.hpp"

#include "coast/fn/Coast.hpp"
#include "coast/fn/Scenario.hpp"
#include "coast/type/Shore.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

using namespace derivean::sim;

// The wave memo (coast/type/Shore.hpp) keeps the trains of cells whose inputs did not move; a raise
// after each kind of change within a step must come out as a full raise does, to the bit.
void testCoastMemo() {
    CoastSpec spec;
    spec.rock = Material::Limestone, spec.other = Material::Chalk;
    spec.joints = 0.8, spec.windFrom = 30;
    Domain domain = worked(spec, 4);
    Shore& shore = shoreOf(domain);
    readStanding(domain, shore), spreadWater(domain, shore), raiseWaves(domain, shore);
    const auto same = [](const std::vector<float>& a, const std::vector<float>& b) { return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0; };
    const auto agrees = [&](const char* message) {
        const std::vector<float> before = shore.wave;
        raiseWaves(domain, shore);
        Shore full = shore;
        full.raised.valid = false;
        raiseWaves(domain, full);
        expectCoast(same(shore.wave, full.wave) && same(shore.arrive, full.arrive) && same(shore.crest, full.crest), message);
        return !same(before, shore.wave);
    };
    // The ground and water of wet cells.
    int moved = 0;
    for (size_t i = 0; i < domain.cells(); i += 37) {
        if (shore.still[i] == NoWater) continue;
        shore.top[i] += 0.3f * float(1 + moved % 3);
        if (moved % 4 == 0) domain.ground.rock[i] -= 0.5f;
        ++moved;
    }
    expectCoast(agrees("a raise that kept unchanged cells differs from a full one after wet cells moved") && moved > 100, "the check of the wave memo moved no train", moved);
    // Still water alone, the ground under it as it was.
    int risen = 0;
    for (size_t i = 0; i < domain.cells(); i += 53)
        if (shore.still[i] != NoWater && domain.hold[i] == Free) shore.still[i] += 0.5f, ++risen;
    expectCoast(agrees("a raise that kept unchanged cells differs from a full one after still water rose") && risen > 50, "the check of the wave memo raised no water", risen);
    // Columns come down to below the water as Cliff.cpp's bringDown takes them: the water reaches
    // the dry cells behind them, which join the box.
    const int size = domain.size;
    int fallen = 0, joined = 0;
    for (int z = 2; z < size - 2 && fallen < 40; ++z)
        for (int x = 2; x < size - 2 && fallen < 40; ++x) {
            const size_t i = domain.index(x, z);
            if (shore.still[i] == NoWater || shore.top[i] < shore.still[i] + 1 || domain.hold[i] != Free) continue;
            bool dryBehind = false;
            for (int k = 0; k < 8; ++k) dryBehind = dryBehind || shore.still[domain.index(x + NeighborX[k], z + NeighborZ[k])] == NoWater;
            if (!dryBehind) continue;
            domain.ground.rock[i] = shore.top[i] = shore.still[i] - 1;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                float& still = shore.still[domain.index(nx, nz)];
                if (still != NoWater) continue;
                still = shore.still[i], ++joined;
                shore.minX = std::min(shore.minX, std::max(0, nx - 1)), shore.maxX = std::max(shore.maxX, std::min(size - 1, nx + 1));
                shore.minZ = std::min(shore.minZ, std::max(0, nz - 1)), shore.maxZ = std::max(shore.maxZ, std::min(size - 1, nz + 1));
            }
            ++fallen;
        }
    expectCoast(agrees("a raise that kept unchanged cells differs from a full one after columns came down") && fallen > 10 && joined > 10,
                "the check of the wave memo brought no column down", fallen);
    // Rim cells, which read no train behind them.
    int rims = 0;
    for (int z = shore.minZ; z <= shore.maxZ; ++z)
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Rim || shore.still[i] == NoWater || (x + z) % 5 != 0) continue;
            shore.top[i] -= 0.4f, ++rims;
        }
    expectCoast(rims > 0, "the check of the wave memo found no wet rim cell");
    agrees("a raise that kept unchanged cells differs from a full one after rim cells moved");
    // A new sea level: the memo is not used.
    domain.seaLevel += 0.25;
    agrees("a raise after the sea level moved differs from a full one");
}

