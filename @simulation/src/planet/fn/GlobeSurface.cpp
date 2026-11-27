#include "planet/fn/GlobeSurface.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"
#include "tectonics/type/Plates.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace derivean::sim {
namespace {

// The flat Surface.cpp's numbers.
constexpr double Denudation = 9 * Myr;
constexpr double SteepHeight = 2500.0;
constexpr int Hops = 3;
constexpr float WaveBase = 25.0f;
constexpr float ShelfDepth = 250.0f;
constexpr double SoluteShare = 0.07;
constexpr float CoolShare = 0.35f;
constexpr float PlatformDepth = 70.0f;
constexpr double WarmLatitude = 45.0 * 3.141592653589793 / 180;
// A unit centre's z is the sine of its latitude; no cell centre of levels 1 to 9 lies within 2e-6 of
// this, so the test agrees with the latitude's own (checked over every centre).
const double SinWarmLatitude = std::sin(WarmLatitude);
constexpr float StepFill = 1500.0f;

Facies faciesOf(float waterDepth, float slope, float supply, float rain) {
    if (waterDepth <= 0) {
        if (rain < 0.3f && slope < 0.0008f) return Facies{5, 110, 80, 0, 0, 60};
        const uint8_t gravel = uint8_t(std::clamp(40.0f + 30000.0f * slope, 20.0f, 150.0f));
        return Facies{gravel, uint8_t(165 - gravel / 2), uint8_t(90 - gravel / 2), 0, 0, 0};
    }
    if (waterDepth > ShelfDepth) return Facies{0, 30, 210, 15, 0, 0};
    if (waterDepth < 60.0f && supply >= 25.0f) return Facies{6, 236, 13, 0, 0, 0};
    if (supply < 150.0f) return Facies{0, 125, 130, 0, 0, 0};
    return Facies{15, 150, 90, 0, 0, 0};
}

double activityOf(const Globe& globe) {
    Epoch ramps;
    ramps.start = globe.epoch.start, ramps.end = globe.epoch.end;
    return epochActivity(ramps, globe.time);
}

// Lays `thickness` m on a cell into its pieces; returns the m³ laid (none where nothing covers it).
double lay(Globe& globe, int cell, double thickness, Facies facies) {
    const double area = globe.grid->area[size_t(cell)];
    if (thickness <= 0 || globe.coupling.covered[size_t(cell)] <= 0) return 0;
    forPiecesOver(globe.coupling, globe.pieces, cell, area, [&](CrustPiece& piece, double toPiece) {
        const double added = thickness * toPiece;
        giveFelsic(piece, added * piece.area);
        piece.column.deposited += float(added);
        addDeposit(piece.record.cover, globe.time, float(added), facies);
    });
    return thickness * area;
}

} // namespace

void refreshGlobeElevation(Globe& globe) {
    const float support = float(activityOf(globe));
    for (size_t cell = 0; cell < globe.elevation.value.size(); ++cell)
        globe.elevation.value[cell] = float(viewOf(globe.coupling, globe.pieces, int(cell), [](const CrustPiece& p) { return columnElevation(p.column); })) +
                                      globe.swell.value[cell] * support;
}

void solveGlobeSea(Globe& globe) {
    const std::vector<float>& height = globe.elevation.value;
    std::vector<int32_t> order(height.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int32_t a, int32_t b) { return height[size_t(a)] < height[size_t(b)] || (height[size_t(a)] == height[size_t(b)] && a < b); });
    // Below the next height the volume is areaBelow · L − Σ A h, linear in L.
    double areaBelow = 0, weighted = 0;
    globe.seaLevel = height[size_t(order[0])];
    for (size_t i = 0; i < order.size(); ++i) {
        const double h = height[size_t(order[i])], area = globe.grid->area[size_t(order[i])];
        areaBelow += area, weighted += area * h;
        const double next = i + 1 < order.size() ? double(height[size_t(order[i + 1])]) : 1e30;
        if (areaBelow * next - weighted >= globe.surfaceWater) {
            globe.seaLevel = (globe.surfaceWater + weighted) / areaBelow;
            return;
        }
    }
}

void shapeGlobeSurface(Globe& globe, double dt) {
    const SphereGrid& grid = *globe.grid;
    const std::vector<CellRing>& rings = *globe.rings;
    const size_t cells = size_t(grid.cells);
    const float sea = float(globe.seaLevel);
    const float memory = float(std::exp(-dt / (10 * Myr)));
    const bool rains = globe.surfaceWater > 0;
    const std::vector<float>& elevation = globe.elevation.value;
    std::vector<int32_t> target(cells, -1);
    std::vector<float> slope(cells, 0.0f);
    std::vector<double> load(cells, 0.0);   // m³ in transit; nothing is left in transit between steps
    Sum eroded, solute;

    // Running water removes crust faster the higher and wetter the ground; a dry body keeps its relief.
    for (size_t cell = 0; cell < cells; ++cell) {
        const float height = elevation[cell];
        float lowest = height;
        double run = 1;
        for (const int32_t other : rings[cell])
            if (other >= 0 && elevation[size_t(other)] < lowest) lowest = elevation[size_t(other)], target[cell] = other;
        if (target[cell] >= 0) run = distanceBetween(grid.centre[cell], grid.centre[size_t(target[cell])]);
        slope[cell] = float((height - lowest) / run);
        double removed = 0;
        if (rains && height > sea) {
            const double above = height - sea;
            const double wetness = 0.35 + 0.65 * std::min(1.5f, globe.precipitation.value[cell]);
            const double crust = viewOf(globe.coupling, globe.pieces, int(cell), [](const CrustPiece& p) { return p.column.crust; });
            const double depth = std::min(crust, above * (1 + above / SteepHeight) * wetness * dt / Denudation);
            Sum taken;
            forPiecesOver(globe.coupling, globe.pieces, int(cell), grid.area[cell], [&](CrustPiece& piece, double toPiece) {
                const double volume = takeFelsic(piece, depth * toPiece * piece.area);
                const float off = float(volume / piece.area);
                const float fromCover = stripCover(piece.record.cover, globe.time, off);
                piece.column.exhumed += off - fromCover;
                piece.column.eroded += off;
                taken.add(volume);
            });
            const double volume = taken.value();
            load[cell] += volume * (1 - SoluteShare);
            solute.add(volume * SoluteShare);
            eroded.add(volume);
            removed = volume / grid.area[cell];
        }
        globe.erosion.value[cell] = globe.erosion.value[cell] * memory + (1 - memory) * float(removed / dt);
    }
    globe.ledger.eroded += eroded.value();

    // Loads walk downhill a few cells per step, settling in basins, on plains and under the sea.
    std::vector<double> laid(cells, 0.0), arriving(cells, 0.0);   // m, m³
    Sum deposited, lost;
    for (int hop = 0; hop <= Hops; ++hop) {
        std::fill(arriving.begin(), arriving.end(), 0.0);
        for (size_t cell = 0; cell < cells; ++cell) {
            if (load[cell] <= 0) continue;
            const double area = grid.area[cell];
            const float height = elevation[cell] + float(laid[cell] * IsostaticRise);
            const float water = sea - height;
            const int32_t next = target[cell];
            double settle;
            if (hop == Hops || next < 0) settle = load[cell];
            else if (water > 0) settle = std::min({load[cell], std::max(0.0, (water - WaveBase) / IsostaticRise) * area, double(StepFill) * area});
            else settle = load[cell] * 0.3 * smoothstep(0.004, 0.0005, slope[cell]);
            const double put = lay(globe, int(cell), settle / area, faciesOf(water, slope[cell], float(load[cell] / area), globe.precipitation.value[cell]));
            laid[cell] += put / area;
            deposited.add(put);
            if (hop == Hops || next < 0) lost.add(load[cell] - put);
            else arriving[size_t(next)] += load[cell] - put;
            load[cell] = 0;
        }
        for (size_t cell = 0; cell < cells; ++cell) load[cell] += arriving[cell];
    }

    // What rivers brought in solution comes out again as carbonate within the step, shared among the
    // shelves by the room their water leaves and by warmth; what they cannot take settles as
    // calcareous mud on the deep floor.
    const bool temperate = globe.surfaceKelvin < 330;
    const auto room = [&](size_t cell) {
        const float water = sea - elevation[cell];
        return !temperate || water <= PlatformDepth || water >= ShelfDepth ? 0.0 : double(water - PlatformDepth) / IsostaticRise;
    };
    require(grid.level <= 9, "the warm shelves' latitude test holds to level 9", grid.level);
    std::vector<uint8_t> warm(cells);
    double pulling = 0, shelfRoom = 0, deepArea = 0;
    for (size_t cell = 0; cell < cells; ++cell) {
        warm[cell] = std::abs(grid.centre[cell].z) < SinWarmLatitude;
        const double area = grid.area[cell];
        pulling += room(cell) * (warm[cell] ? 1.0 : CoolShare) * area;
        shelfRoom += room(cell) * area;
        if (sea - elevation[cell] >= ShelfDepth) deepArea += area;
    }
    const double dissolved = solute.value();
    const double perPull = pulling > 0 ? std::min(dissolved, shelfRoom) / pulling : 0.0;
    double onShelves = 0;
    for (size_t cell = 0; cell < cells; ++cell) onShelves += std::min(room(cell), perPull * room(cell) * (warm[cell] ? 1.0 : CoolShare)) * grid.area[cell];
    const double deepLayer = deepArea > 0 ? std::max(0.0, dissolved - onShelves) / deepArea : 0.0;
    Sum grown;
    for (size_t cell = 0; cell < cells; ++cell) {
        const bool deep = sea - elevation[cell] >= ShelfDepth;
        const double layer = deep ? deepLayer : std::min(room(cell), perPull * room(cell) * (warm[cell] ? 1.0 : CoolShare));
        const double put = lay(globe, int(cell), layer, deep ? Facies{0, 0, 60, 195, 0, 0} : Facies{0, 5, 20, 230, 0, 0});
        laid[cell] += put / grid.area[cell];
        grown.add(put);
        globe.deposition.value[cell] = globe.deposition.value[cell] * memory + (1 - memory) * float(laid[cell] / dt);
    }
    lost.add(dissolved - grown.value());
    globe.ledger.precipitated += grown.value();
    globe.ledger.deposited += deposited.value() + grown.value();
    globe.ledger.interior.felsic += lost.value();
}

void stampGlobeMarks(Globe& globe) {
    const size_t slot = size_t(pastMarkIndex(globe.time));
    const float support = float(activityOf(globe));
    for (CrustPiece& piece : globe.pieces) {
        const CrustColumn& c = piece.column;
        const int cell = indexOf(*globe.grid, cellOf(centreOf(*globe.grid, piece), globe.grid->level));
        const float elevation = columnElevation(c) + globe.swell.value[size_t(cell)] * support;
        piece.record.past[slot] = {elevation, c.crust, c.tectonicGain, c.eroded, c.deposited, c.melt, c.erupted};
    }
}

} // namespace derivean::sim
