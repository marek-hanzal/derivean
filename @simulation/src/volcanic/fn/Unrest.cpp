#include "volcanic/fn/Unrest.hpp"

#include "simulation/type/World.hpp"
#include "volcanic/fn/Source.hpp"
#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {
namespace {

constexpr double ChamberDepth[2] = {3000.0, 6000.0};   // m, by Magma
constexpr double Poisson = 0.25;
// CLOSED FORM with assumed coefficients: a chamber is ringed by small earthquakes in proportion to
// the melt that feeds it, and many times more as its roof nears failure.
constexpr double QuakesPerSupply = 2e-4;   // a year per m³/yr of supply while the chamber is far from failing
constexpr double FailureGain = 40.0;       // times that rate as it fails

} // namespace

Unrest ventUnrest(uint64_t seed, const Vent& vent, double time) {
    Unrest unrest;
    unrest.depth = ChamberDepth[size_t(vent.magma)];
    if (!vent.recent.empty()) unrest.sinceLast = std::max(0.0, time - vent.recent.back().time), unrest.lastVolume = vent.recent.back().volume;
    Eruption next;
    if (!nextEruption(seed, vent, next)) return unrest;
    unrest.supplied = true;
    double held = vent.held, from = vent.until;
    for (int64_t number = vent.count; next.time <= time; ++number) {
        // It has erupted since the record ended: the chamber starts to fill again from there.
        unrest.sinceLast = time - next.time, unrest.lastVolume = next.volume;
        held = 0, from = next.time;
        next.volume = eruptionVolume(seed, vent, number + 1);
        next.time += next.volume / vent.supply;
    }
    unrest.held = held + vent.supply * std::max(0.0, time - from);
    unrest.nextVolume = next.volume;
    unrest.untilNext = next.time - time;
    unrest.fill = std::clamp(unrest.held / next.volume, 0.0, 1.0);
    // A point source of that volume at the chamber's depth lifts the ground over it (Mogi).
    unrest.uplift = (1.0 - Poisson) * unrest.held / (3.141592653589793 * unrest.depth * unrest.depth);
    unrest.quakes = QuakesPerSupply * vent.supply * (1.0 + FailureGain * std::pow(unrest.fill, 6.0));
    return unrest;
}

double upliftAt(const Unrest& unrest, double distance) {
    const double ratio = distance / std::max(1.0, unrest.depth);
    return unrest.uplift / std::pow(1.0 + ratio * ratio, 1.5);
}

VolcanicRecord volcanicAt(World& world, Vec2 position, double calendarYears, double reach) {
    VolcanicRecord record;
    const TileKey key = tileAt(Level::Region, position);
    const std::shared_ptr<const Tile> tile = world.atlas.require(key);
    const double cell = specOf(Level::Region).cell;
    const size_t at = size_t(std::clamp(int(std::floor(position.z / cell)) - key.z * TileCells, 0, TileCells - 1)) * TileCells +
                      size_t(std::clamp(int(std::floor(position.x / cell)) - key.x * TileCells, 0, TileCells - 1));
    record.pile = pileColumn(*tile, at);
    if (!tile->volcanic) return record;
    const VolcanicTile& volcanic = *tile->volcanic;
    if (record.pile.thickness > 0) record.rock = pileBedAt(world.atlas.setting().seed, volcanic, record.pile, record.pile.thickness - 0.01f).rock;
    record.distance = reach;
    for (const Vent& vent : volcanic.vents) {
        const double distance = length(vent.feeders.back().site - position);
        if (distance >= record.distance) continue;
        record.found = true;
        record.vent = vent;
        record.site = vent.feeders.back().site;
        record.distance = distance;
    }
    if (record.found) record.unrest = ventUnrest(world.atlas.setting().seed, record.vent, record.vent.until + calendarYears);
    return record;
}

} // namespace derivean::sim
