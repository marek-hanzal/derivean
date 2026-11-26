#include "volcanic/fn/Source.hpp"

#include "core/fn/Hash.hpp"
#include "landscape/fn/Forcing.hpp"
#include "landscape/type/Level.hpp"

#include <set>

namespace derivean::sim {
namespace {

constexpr uint64_t VentStream = 0x564f4c43;
constexpr double SpellSteps = 4;       // region steps a vent stays awake or asleep
constexpr double Awake = 0.7;          // share of those spells it is awake
constexpr double Unevenness = 1.2;     // log-normal spread of what the steps of a spell erupt
constexpr double FieldReach[2] = {10000.0, 6000.0};   // m around its centre within which a conduit may come up, by Magma
constexpr double PlugRadius = 60.0;    // m for a conduit that brought up a cubic kilometre
constexpr double SamplePitch = 16000.0;
constexpr size_t Remembered = 8;       // single eruptions before the end of the record that a vent keeps for whoever asks what it did last

// What one planet cell erupted between the marks of the region's window, in m³.
struct Budget {
    std::vector<double> bound, volume;   // interval k runs from bound[k] to bound[k + 1]
    Magma magma{Magma::Basalt};
    double quiet{};                      // yr the melt stopped rising
};

Budget budgetOf(const Planet& planet, const Window& window, int cellX, int cellZ) {
    const int x = wrapIndex(cellX, PlanetCells), z = wrapIndex(cellZ, PlanetCells);
    const CrustColumn& column = planet.columns.at(x, z);
    Budget budget;
    if (column.meltTime < 0 || planet.time <= window.start) return budget;
    budget.quiet = double(column.meltTime) + PlanetStep;
    budget.magma = column.meltSource == MeltSource::Arc ? Magma::Andesite : Magma::Basalt;
    // The record travels with the crust: what a later mark holds more reached the surface in between.
    const auto eruptedBy = [&](double time) { return time >= planet.time ? double(column.erupted) : double(planet.records.at(x, z).past[size_t(pastMarkIndex(time))].erupted); };
    for (double time = window.start; time < planet.time; time += RegionAnchor) budget.bound.push_back(time);
    budget.bound.push_back(planet.time);
    for (size_t k = 0; k + 1 < budget.bound.size(); ++k)
        budget.volume.push_back(std::max(0.0, eruptedBy(budget.bound[k + 1]) - eruptedBy(budget.bound[k])) * PlanetCellArea);
    return budget;
}

double normalDraw(uint64_t base, int64_t a, int64_t b) {
    return std::sqrt(-2.0 * std::log(1.0 - unit(base, a, b, 1))) * std::cos(6.283185307179586 * unit(base, a, b, 2));
}

// The steps of the window a vent erupts in, their volumes, and where its conduit came up.
void schedule(uint64_t seed, Vent& vent, const Budget& budget, double share, const Window& window, double now) {
    const uint64_t base = stream(seed, VentStream) ^ vent.id;
    const int64_t cycle = 2 + int64_t(hash(base, 1) % 6), shift = int64_t(hash(base, 2) % uint64_t(cycle));
    double lastRate = 0;
    for (size_t k = 0; k < budget.volume.size(); ++k) {
        const double from = budget.bound[k], to = std::min(budget.bound[k + 1], std::max(budget.quiet, from + RegionStep));
        const int64_t first = int64_t(std::ceil(from / RegionStep - 1e-9)), last = int64_t(std::ceil(to / RegionStep - 1e-9));
        if (budget.volume[k] <= 0 || last <= first) continue;
        std::vector<double> weight(size_t(last - first));
        double total = 0;
        for (int64_t step = first; step < last; ++step) {
            const bool awake = unit(base, int64_t(std::floor(double(step) / SpellSteps)), 3) < Awake;
            total += weight[size_t(step - first)] = awake ? std::exp(Unevenness * normalDraw(base, step, 4)) : 0.0;
        }
        if (total <= 0) weight.assign(weight.size(), 1.0), total = double(weight.size());
        for (int64_t step = first; step < last; ++step) {
            const double volume = budget.volume[k] * share * weight[size_t(step - first)] / total;
            if (volume > 0) vent.episodes.push_back({double(step) * RegionStep, volume, 0.0, {}});
        }
        lastRate = budget.volume[k] * share / (double(last - first) * RegionStep);
    }
    // A window may end later than the planet's last step: melt that was rising then still does.
    vent.supply = budget.quiet >= now - 1.0 ? lastRate : 0.0;
    for (double time = now; time < window.end - 1.0 && vent.supply > 0; time += RegionStep) vent.episodes.push_back({time, vent.supply * RegionStep, 0.0, {}});

    // The supply of each step fills the chamber; what comes out in a step are the eruptions whose
    // turn comes within it. A great one has waited through steps that brought up little.
    std::vector<Episode> supplied;
    supplied.swap(vent.episodes);
    double held = 0, next = eruptionVolume(seed, vent, 0);
    for (const Episode& filling : supplied) {
        Episode episode{filling.time, 0.0, 0.0, {}};
        double filled = 0;
        while (held + filling.volume - filled >= next) {
            filled += next - held;
            held = 0;
            episode.volume += next;
            episode.paroxysm = std::max(episode.paroxysm, next);
            vent.recent.push_back({filling.time + RegionStep * filled / filling.volume, next});
            if (vent.recent.size() > Remembered) vent.recent.erase(vent.recent.begin());
            next = eruptionVolume(seed, vent, ++vent.count);
        }
        held += filling.volume - filled;
        if (episode.volume <= 0) continue;
        const int64_t turn = (int64_t(std::llround(episode.time / RegionStep)) + shift) / cycle;
        const double reach = FieldReach[size_t(vent.magma)] * std::pow(unit(base, turn, 5), vent.magma == Magma::Basalt ? 1.5 : 2.0);
        const double bearing = 6.283185307179586 * unit(base, turn, 6);
        episode.site = vent.centre + Vec2{std::cos(bearing), std::sin(bearing)} * reach;
        if (vent.feeders.empty() || length(vent.feeders.back().site - episode.site) > 1.0) vent.feeders.push_back({episode.site, 0.0f, episode.time, episode.time, 0.0});
        Feeder& feeder = vent.feeders.back();
        feeder.last = episode.time + RegionStep;
        feeder.volume += episode.volume;
        feeder.radius = float(std::clamp(PlugRadius * std::cbrt(feeder.volume / 1e9), 80.0, 600.0));
        vent.erupted += episode.volume;
        vent.episodes.push_back(episode);
    }
    vent.held = held;
}

} // namespace

std::vector<Vent> ventsNear(const Setting& setting, Vec2 low, Vec2 high, double reach) {
    const Planet& planet = *setting.planet;
    const Window window = windowAt(Level::Region, setting.age);
    std::vector<Vent> vents;
    if (planet.time <= window.start || planet.columns.size() == 0) return vents;
    // The planet cells the rectangle reads, through the same offset the landscape reads the planet by.
    std::set<std::pair<int, int>> cells;
    const Vec2 from = low - Vec2{reach, reach}, to = high + Vec2{reach, reach};
    const int countX = int(std::ceil((to.x - from.x) / SamplePitch)), countZ = int(std::ceil((to.z - from.z) / SamplePitch));
    for (int z = 0; z <= countZ; ++z) {
        for (int x = 0; x <= countX; ++x) {
            const Vec2 point = planetPoint(setting.seed, {std::min(to.x, from.x + x * SamplePitch), std::min(to.z, from.z + z * SamplePitch)});
            const int cx = floorToInt(point.x / PlanetCell), cz = floorToInt(point.z / PlanetCell);
            for (int dz = -1; dz <= 1; ++dz)
                for (int dx = -1; dx <= 1; ++dx) cells.insert({cz + dz, cx + dx});
        }
    }
    for (const auto& [cellZ, cellX] : cells) {
        const Budget budget = budgetOf(planet, window, cellX, cellZ);
        double total = 0;
        for (const double volume : budget.volume) total += volume;
        if (total <= 0) continue;
        const std::vector<Centre> centres = centresIn(planet, cellX, cellZ);
        double vigor = 0;
        for (const Centre& centre : centres) vigor += centre.vigor;
        for (const Centre& centre : centres) {
            const Vec2 at = worldPoint(setting.seed, centre.position);
            if (at.x < from.x || at.z < from.z || at.x > to.x || at.z > to.z) continue;
            Vent vent;
            vent.id = centre.id;
            vent.centre = at;
            vent.magma = budget.magma;
            vent.vigor = centre.vigor;
            vent.begin = window.start;
            vent.until = window.end;
            schedule(setting.seed, vent, budget, centre.vigor / vigor, window, planet.time);
            if (vent.erupted > 0) vents.push_back(std::move(vent));
        }
    }
    return vents;
}

} // namespace derivean::sim
