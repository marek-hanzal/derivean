#include "icesheet/service/Glaciations.hpp"

#include "icesheet/fn/Continental.hpp"

#include <mutex>
#include <vector>

namespace derivean::sim {
namespace {

// A world is told apart by what its planet is a function of, and by the planet itself: two worlds
// of one seed and age hold equal planets, and a planet is never written after it is made.
struct Solved {
    uint64_t seed;
    double age, planetTime;
    std::shared_ptr<const ContinentalIce> ice;
};

std::mutex mutex;
std::vector<Solved> solved;

} // namespace

std::shared_ptr<const ContinentalIce> continentalIce(const Setting& setting) {
    if (!setting.planet || setting.planet->elevation.cells.empty()) return nullptr;
    // Tiles that ask while the first one is solving wait here; the solve spreads its rows over the
    // pool from the asking thread, which finishes them alone if every other thread is waiting.
    std::lock_guard lock(mutex);
    for (const Solved& known : solved)
        if (known.seed == setting.planet->seed && known.age == setting.age && known.planetTime == setting.planet->time) return known.ice;
    solved.push_back({setting.planet->seed, setting.age, setting.planet->time, std::make_shared<const ContinentalIce>(solveContinental(*setting.planet, setting.age, setting.workers))});
    return solved.back().ice;
}

} // namespace derivean::sim
