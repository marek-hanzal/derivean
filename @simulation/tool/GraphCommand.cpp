#include "Arguments.hpp"

#include "ground/type/Detail.hpp"
#include "landscape/type/WaterInfluences.hpp"
#include "measure/fn/Digest.hpp"
#include "simulation/fn/BuildGraph.hpp"
#include "simulation/type/World.hpp"

#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>

using namespace derivean::sim;

namespace {

std::string label(BuildKey key) {
    const char* product = "Terrain";
    if (key.product == BuildProduct::RegionalWater) product = "Regional water";
    if (key.product == BuildProduct::LocalWater) product = "Local aquifer";
    return std::string(product) + " L" + std::to_string(int(key.tile.level)) + " (" +
           std::to_string(key.tile.x) + ", " + std::to_string(key.tile.z) + ")";
}

void describe(std::ostream& out, BuildKey key, BuildKeys& shown) {
    if (!shown.insert(key).second) return;
    out << "  \"" << label(key) << "\";\n";
    for (const BuildKey input : buildReads(key)) {
        out << "  \"" << label(input) << "\" -> \"" << label(key) << "\";\n";
        describe(out, input, shown);
    }
}


void describeWaterInfluences(std::ostream& out) {
    out << "digraph WaterInfluences {\n";
    for (const InfluenceStage& stage : WaterStages)
        if (!stage.feedback) out << "  \"" << stage.name << "\";\n";
    out << "  subgraph cluster_routing {\n"
        << "    label=\"Surface water solver: queue empty; limit "
        << HollowSpillStop.visitsPerCell << "*cells + " << HollowSpillStop.allowance << "\";\n";
    for (const InfluenceStage& stage : WaterStages)
        if (stage.feedback) out << "    \"" << stage.name << "\";\n";
    out << "  }\n";
    for (const InfluenceLink link : WaterLinks) {
        out << "  \"" << WaterStages[link.source].name << "\" -> \"" << WaterStages[link.target].name << "\"";
        out << " [";
        if (link.time == InfluenceTiming::NextStep) out << "style=dashed, ";
        out << "label=\"" << link.signal;
        if (link.time == InfluenceTiming::NextStep) out << " (next step)";
        out << "\"];\n";
    }
    out << "}\n";
}

using Clock = std::chrono::steady_clock;

double milliseconds(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}

template <class T>
uint64_t addDigest(uint64_t digest, const std::vector<T>& values) {
    return digestOf(values.data(), values.size() * sizeof(T), digest);
}

uint64_t surfaceDetail(const Tile& tile) {
    const GroundDetail& detail = *tile.detail;
    uint64_t result = addDigest(1, tile.flowTo);
    result = addDigest(result, detail.crust);
    result = addDigest(result, detail.crustKind);
    result = addDigest(result, detail.coarseKind);
    result = addDigest(result, detail.fineKind);
    result = addDigest(result, detail.bare);
    result = addDigest(result, detail.stepGaps);
    for (const Clast& stone : detail.clasts) {
        const float shape[]{stone.x, stone.z, stone.y, stone.a, stone.b, stone.c, stone.yaw,
                            stone.pitch, stone.roll, stone.roundness, stone.travel};
        const double source[]{stone.source.x, stone.source.z};
        const uint8_t type[]{uint8_t(stone.rock), uint8_t(stone.origin)};
        result = digestOf(shape, sizeof(shape), result);
        result = digestOf(source, sizeof(source), result);
        result = digestOf(type, sizeof(type), result);
    }
    return result;
}

size_t buildBatch(World& world, const std::vector<BuildKey>& ready) {
    std::vector<std::future<double>> done;
    const size_t count = std::min(ready.size(), size_t(world.workers.threads()));
    done.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const auto task = std::make_shared<std::packaged_task<double()>>([&world, key = ready[i]] {
            const auto start = Clock::now();
            buildReady(world, key);
            return milliseconds(start);
        });
        done.push_back(task->get_future());
        world.workers.submit([task] { (*task)(); });
    }
    std::exception_ptr failure;
    for (size_t i = 0; i < done.size(); ++i) {
        try {
            const double elapsed = done[i].get();
            std::cerr << label(ready[i]) << " built in " << elapsed << " ms\n";
        } catch (...) {
            if (!failure) failure = std::current_exception();
        }
    }
    if (failure) std::rethrow_exception(failure);
    return count;
}

void evaluate(World& world, BuildKey target, const char* pass, bool demand) {
    const auto start = Clock::now();
    size_t builds = 0;
    double planning = 0.0;
    if (demand) world.atlas.require(target.tile);
    while (!built(world, target)) {
        const auto beginPlan = Clock::now();
        std::vector<BuildKey> ready;
        BuildKeys planned;
        buildFrontier(world, target, planned, ready);
        planning += milliseconds(beginPlan);
        if (ready.empty()) throw std::logic_error("unfinished build graph has no ready node");
        builds += buildBatch(world, ready);
    }
    const double elapsed = milliseconds(start);
    const auto tile = world.atlas.find(target.tile);
    const TileDigest digest = digestOf(*tile);
    std::cerr << pass << ": " << elapsed << " ms";
    if (!demand) std::cerr << ", " << builds << " builds, " << planning << " ms planning";
    std::cerr << "; terrain digest " << std::hex << digest.ground << ' ' << digest.water << ' '
              << digest.discharge << ' ' << surfaceDetail(*tile) << std::dec << '\n';
}

} // namespace

int graphCommand(const Arguments& arguments) {
    const Vec2 at{arguments.number("x", 0), arguments.number("z", 0)};
    const BuildKey target{BuildProduct::Terrain, tileAt(Level::Ground, at)};
    const std::string path = arguments.text("out", "-");
    std::ofstream file;
    std::ostream* out = &std::cout;
    if (path != "-") {
        file.open(path);
        if (!file) {
            std::cerr << "Cannot write graph to " << path << '\n';
            return 2;
        }
        out = &file;
    }
    try {
        if (arguments.text("view", "build") == "influences") describeWaterInfluences(*out);
        else {
            BuildKeys shown;
            *out << "digraph Build {\n";
            describe(*out, target, shown);
            *out << "}\n";
        }
        if (arguments.number("build", 0) == 0) return 0;
        const FrostMovement frost = arguments.number("frost-movement", 1) == 0
                                   ? FrostMovement::PassThrough : FrostMovement::Apply;
        std::cerr << "Frost movement " << (frost == FrostMovement::Apply ? "applied" : "pass-through") << '\n';
        World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0)), frost});
        const bool demand = arguments.text("route", "graph") == "demand";
        evaluate(world, target, "cold", demand);
        evaluate(world, target, "warm", demand);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
