#include "groundwater/service/Aquifers.hpp"
#include "landscape/constant/WaterInfluences.hpp"
#include "simulation/fn/BuildGraph.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>

void testBuildGraphTile();

namespace {

using namespace derivean::sim;

void expect(bool accepted) {
    if (accepted) return;
    std::fputs("Build graph has an invalid dependency or lost a boundary input\n", stderr);
    std::exit(1);
}

constexpr std::array TwoStages{InfluenceStage{"surface", 0}, InfluenceStage{"erosion", 1}};
constexpr std::array BadCycle{InfluenceLink{0, 1}, InfluenceLink{1, 0}};
constexpr std::array DelayedCycle{InfluenceLink{0, 1}, InfluenceLink{1, 0, InfluenceTiming::NextStep}};
constexpr std::array SelfCycle{InfluenceLink{0, 0}};
constexpr std::array BadIndex{InfluenceLink{0, 2}};
static_assert(!validInfluences(TwoStages, BadCycle));
static_assert(validInfluences(TwoStages, DelayedCycle));
static_assert(!validInfluences(TwoStages, SelfCycle));
static_assert(!validInfluences(TwoStages, BadIndex));

enum class State { Visiting, Visited };

void visit(BuildKey key, std::unordered_map<BuildKey, State, BuildKeyHash>& seen) {
    const auto [item, first] = seen.emplace(key, State::Visiting);
    if (!first) {
        expect(item->second == State::Visited);
        return;
    }
    BuildKeys inputs;
    for (const BuildKey input : buildReads(key)) {
        expect(inputs.insert(input).second);
        visit(input, seen);
    }
    seen.at(key) = State::Visited;
}

size_t countReads(BuildKey from, BuildProduct product) {
    size_t count = 0;
    for (const BuildKey input : buildReads(from))
        if (input.product == product) ++count;
    return count;
}

template <class Build>
bool rejectsMissing(Build&& build) {
    try {
        build();
    } catch (const std::logic_error&) {
        return true;
    }
    return false;
}

void preparedNeedsInputs() {
    Workers workers(1);
    Atlas atlas(Setting{}, workers);
    Aquifers aquifers;
    const TileKey local{Level::Local, 0, 0};
    const TileKey region{Level::Region, 0, 0};
    // Repeated failure must release each cache claim.
    for (int attempt = 0; attempt < 2; ++attempt) {
        expect(rejectsMissing([&] { atlas.require(local, BuildMode::Prepared); }));
        expect(rejectsMissing([&] { aquifers.requireRegional(atlas, region, BuildMode::Prepared); }));
        expect(rejectsMissing([&] { aquifers.require(atlas, local, BuildMode::Prepared); }));
    }
    expect(atlas.finished() == 0);
    expect(!aquifers.findRegional(region) && !aquifers.find(local));
}

} // namespace

void testBuildGraph() {
    const BuildKey inside{BuildProduct::Terrain, {Level::Ground, 2176, 2176}};
    const BuildKey corner{BuildProduct::Terrain, {Level::Ground, 4095, 4095}};
    const BuildKey aquifer{BuildProduct::LocalWater, {Level::Local, 15, 15}};
    const BuildKey regional{BuildProduct::RegionalWater, {Level::Region, 0, 0}};
    expect(countReads(inside, BuildProduct::LocalWater) == 1);
    expect(countReads(corner, BuildProduct::LocalWater) == 4);
    expect(countReads(aquifer, BuildProduct::Terrain) == 9);
    expect(countReads(aquifer, BuildProduct::RegionalWater) == 4);
    expect(countReads(regional, BuildProduct::Terrain) == 9);
    std::unordered_map<BuildKey, State, BuildKeyHash> seen;
    visit(corner, seen);
    expect(seen.contains(aquifer) && seen.contains(regional));
    preparedNeedsInputs();
    testBuildGraphTile();
}
