#include "planet/fn/GlobeEvolve.hpp"

#include "climate/type/Climate.hpp"
#include "core/fn/Hash.hpp"
#include "geometry/constant/Sphere.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/fn/GlobeContacts.hpp"
#include "planet/fn/GlobeHeat.hpp"
#include "planet/fn/GlobeSurface.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "planet/fn/Seam.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

// The flat Evolve.cpp's numbers.
constexpr uint64_t WaterStream = 0x57415452;
constexpr int SeaLevelEvery = 4;     // planet steps
constexpr int ClimateEvery = 16;

void setWater(Globe& globe) {
    globe.surfaceKelvin = coolingSurface(globe.time);
    globe.surfaceWater = std::max(0.0, globe.ledger.waterInventory * condensedShare(globe.time));
}

void solveClimate(Globe& globe, Workers& workers) {
    Planet view = flatViewOf(globe);
    solvePlanetClimate(view, workers);
    readClimateBack(globe, view);
}

void forCovers(Globe& globe, void (*act)(Cover&, double)) {
    for (CrustPiece& piece : globe.pieces) act(piece.record.cover, globe.time);
}

} // namespace

Globe startGlobe(uint64_t seed, double time, int level, PlateSpeeds speeds) {
    Globe globe = makeGlobe(seed, level, speeds, time);
    globe.ledger.waterInventory = 4 * 3.141592653589793 * PlanetRadius * PlanetRadius * 4200.0 * ranged(hash(stream(seed, WaterStream), 1), 0.95, 1.05);
    couple(*globe.grid, globe.pieces, {}, false, globe.coupling);
    // The first epoch is the one tectonics starts in, as in the flat history.
    const double now = globe.time;
    globe.time = TectonicsStart;
    enterGlobeEpoch(globe);
    globe.time = now;
    setWater(globe);
    refreshGlobeElevation(globe);
    solveGlobeSea(globe);
    return globe;
}

void stepGlobe(Globe& globe, int64_t step, Workers& workers) {
    restoreKeyOrder(globe);
    const double before = globe.time;
    if (before >= globe.epoch.end) reorganisePlates(globe, int(step));
    if (strataSlot(before) != strataSlot(before - PlanetStep)) {
        forCovers(globe, retireOldest);
        forCovers(globe, buryCover);
    }

    setWater(globe);
    advancePlates(globe, PlanetStep, int(step));
    auto mark = std::chrono::steady_clock::now();
    traceGlobeContacts(globe);
    actOnGlobeContacts(globe, PlanetStep);
    relaxGlobeThickCrust(globe, PlanetStep);
    for (CrustPiece& piece : globe.pieces) piece.column.oceanAge += float(PlanetStep);
    clockPhase(globe, GlobePhase::Contacts, mark);

    refreshGlobeElevation(globe);
    if (step % SeaLevelEvery == 0) solveGlobeSea(globe);
    clockPhase(globe, GlobePhase::Elevation, mark);
    if (step % ClimateEvery == 0) solveClimate(globe, workers);
    clockPhase(globe, GlobePhase::Climate, mark);
    shapeGlobeSurface(globe, PlanetStep);

    globe.time = TectonicsStart + double(step + 1) * PlanetStep;
    globe.seaLevels.push_back(float(globe.seaLevel));
    if (std::fmod(globe.time, RegionAnchor) == 0) {
        refreshGlobeElevation(globe);
        stampGlobeMarks(globe);
    }
    clockPhase(globe, GlobePhase::Surface, mark);
}

void finishGlobe(Globe& globe, Workers& workers) {
    setWater(globe);
    refreshGlobeElevation(globe);
    solveGlobeSea(globe);
    solveClimate(globe, workers);
    refreshGlobeMeltHeat(globe);
    forCovers(globe, buryCover);
}

Globe evolveGlobe(uint64_t seed, double time, int level, PlateSpeeds speeds, Workers& workers) {
    Globe globe = startGlobe(seed, time, level, speeds);
    const int64_t steps = time < TectonicsStart ? 0 : int64_t((time - TectonicsStart) / PlanetStep);
    globe.seaLevels.reserve(size_t(steps) + 1);
    for (int64_t step = 0; step < steps; ++step) stepGlobe(globe, step, workers);
    finishGlobe(globe, workers);
    return globe;
}

} // namespace derivean::sim
