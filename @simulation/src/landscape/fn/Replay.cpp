#include "landscape/fn/Build.hpp"

#include "aeolian/fn/Wind.hpp"
#include "climate/type/Climate.hpp"
#include "ground/fn/Stages.hpp"
#include "groundwater/fn/Sinking.hpp"
#include "landscape/fn/Forcing.hpp"
#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {

void stepDomain(Domain& domain, FlowScratch& scratch, const Setting& setting) {
    const bool dated = domain.key.level == Level::Region, ground = domain.key.level == Level::Ground;
    domain.seaLevel = seaLevelAt(setting, dated ? domain.time + domain.step : setting.age);
    followNeighbours(domain, domain.time + domain.step);
    readClimate(domain, setting);
    prepareSinks(domain);
    raise(domain, scratch);
    erupt(domain, scratch, setting);
    if (ground) refreshPartings(domain, setting.seed);
    else refreshUnits(domain, setting.seed);
    // Slopes move first and rivers last: what a slope sheds into a valley within a step is met by
    // the water of the same step instead of being left behind as a dam.
    shapeSlopes(domain, scratch);
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    dissolveSinks(domain);
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
    sheetIce(domain, scratch, setting, IceStage::Step);
    cutAndFill(domain, scratch);
    workShores(domain, scratch);
    blowLoose(domain, scratch);
    // Banks of loose cover the water has cut into, or whose foot the wind has swept bare, slide in
    // the same step; left standing they are walls of sand tens of metres high on a Site tile. The
    // last step's cover is what the tile keeps, so nothing may move loose cover after this.
    if (!dated && !ground) settleDebris(domain, scratch);
    domain.time += domain.step;
    ++domain.taken;
}

std::shared_ptr<Tile> replayTile(const Setting& setting, TileKey key, const TileLookup& lookup) {
    Domain domain = buildDomain(setting, key, lookup);
    FlowScratch scratch;
    auto tile = std::make_shared<Tile>();
    const Window window = windowAt(key.level, setting.age);
    for (int step = 0; step < window.steps; ++step) stepDomain(domain, scratch, setting);
    // A region's sheets are part of its settling, so its edge is blended with them.
    if (key.level == Level::Region) sheetIce(domain, scratch, setting, IceStage::Leave, &lookup);
    // The blended edge goes back into the domain first, so the water described below stands on the
    // surface that is handed over.
    followNeighbours(domain, domain.endTime);
    // Following the finished neighbours has just moved cover: the wind's labels, which say what of
    // it the wind laid, are brought within what lies there before the seams are blended.
    if (domain.aeolian.labels.blown.size() == domain.cells()) tidyLabels(domain);
    settleSeams(domain, *tile);
    // At the finer levels the last sheet's change is a layer of its own over the blended edge:
    // neighbours blend the ground they both settled, and the sheet carries on across the edge from
    // what the finished neighbour's sheet recorded.
    if (key.level != Level::Region) sheetIce(domain, scratch, setting, IceStage::Leave, &lookup);
    // What the last sheet left rests at its angle of repose like any other loose cover: the tile
    // keeps no column of it.
    if (key.level != Level::Region && key.level != Level::Ground) {
        settleDebris(domain, scratch);
        if (domain.aeolian.labels.blown.size() == domain.cells()) tidyLabels(domain);
    }
    readClimate(domain, setting);
    tile->volcanic = volcanicOf(domain);
    if (key.level == Level::Ground) {
        // What lives on the ground at its own scale is made last, and the tile read off after it.
        refreshPartings(domain, setting.seed);
        finishGround(domain, scratch, setting, lookup, *tile);
        return tile;
    }
    refreshUnits(domain, setting.seed);
    prepareSinks(domain);
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    // Ice that the present climate still keeps.
    if (key.level != Level::Region)
        glaciate(domain, scratch, float(zonalClimate(setting.seed, domain.origin.z + 0.5 * domain.size * domain.cell, setting.age).seasonal), 0.0f, false);
    tile->sheet = sheetIce(domain, scratch, setting, IceStage::Present, &lookup);
    extractTile(domain, scratch, *tile);
    return tile;
}

} // namespace derivean::sim
