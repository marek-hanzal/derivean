#include "landscape/fn/Build.hpp"

#include "climate/type/Climate.hpp"
#include "ground/fn/Stages.hpp"
#include "landscape/fn/Forcing.hpp"
#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {

void stepDomain(Domain& domain, FlowScratch& scratch, const Setting& setting) {
    const bool dated = domain.key.level == Level::Region, ground = domain.key.level == Level::Ground;
    domain.seaLevel = seaLevelAt(setting, dated ? domain.time + domain.step : setting.age);
    followNeighbours(domain, domain.time + domain.step);
    readClimate(domain, setting);
    raise(domain, scratch);
    erupt(domain, scratch, setting);
    if (ground) refreshPartings(domain, setting.seed);
    else refreshUnits(domain, setting.seed);
    // Slopes move first and rivers last: what a slope sheds into a valley within a step is met by
    // the water of the same step instead of being left behind as a dam.
    shapeSlopes(domain, scratch);
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    if (ground) widenChannels(domain);
    // Cold phases alternate with warm ones while a finer level settles, as glacials and
    // interglacials did; the last steps are warm, so rivers and lakes occupy what the ice left.
    // A ground tile is far smaller than a glacier and takes the ice of its parent as given.
    const Window window = windowAt(domain.key.level, setting.age);
    const bool cold = !dated && !ground && domain.taken < window.steps - window.steps / 5 && domain.taken % 5 >= 2;
    if (cold) {
        const float halfRange = float(zonalClimate(setting.seed, domain.origin.z + 0.5 * domain.size * domain.cell, setting.age).seasonal);
        const float deepest = -float(1.5 + 5.5 * smoothstep(3.0, -1.0, greenhouseOffset(setting.seed, setting.age)));
        glaciate(domain, scratch, halfRange, deepest, true);
        for (size_t i = 0; i < domain.cells(); ++i) domain.iceAge[i] = std::max(domain.iceAge[i], domain.ice[i]);
    }
    cutAndFill(domain, scratch);
    workShores(domain, scratch);
    blowLoose(domain, scratch);
    domain.time += domain.step;
    ++domain.taken;
}

std::shared_ptr<Tile> replayTile(const Setting& setting, TileKey key, const TileLookup& lookup) {
    Domain domain = buildDomain(setting, key, lookup);
    FlowScratch scratch;
    auto tile = std::make_shared<Tile>();
    const Window window = windowAt(key.level, setting.age);
    for (int step = 0; step < window.steps; ++step) stepDomain(domain, scratch, setting);
    // The blended edge goes back into the domain first, so the water described below stands on the
    // surface that is handed over.
    followNeighbours(domain, domain.endTime);
    settleSeams(domain, *tile);
    readClimate(domain, setting);
    tile->volcanic = volcanicOf(domain);
    if (key.level == Level::Ground) {
        // What lives on the ground at its own scale is made last, and the tile read off after it.
        refreshPartings(domain, setting.seed);
        finishGround(domain, scratch, setting, lookup, *tile);
        return tile;
    }
    refreshUnits(domain, setting.seed);
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    // Ice that the present climate still keeps.
    if (key.level != Level::Region)
        glaciate(domain, scratch, float(zonalClimate(setting.seed, domain.origin.z + 0.5 * domain.size * domain.cell, setting.age).seasonal), 0.0f, false);
    extractTile(domain, scratch, *tile);
    return tile;
}

} // namespace derivean::sim
