#include "geometry/fn/SphereGrid.hpp"
#include "material/constant/Materials.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/type/Globe.hpp"
#include "planet/type/Planet.hpp"
#include "strata/type/Column.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace derivean::sim;

// The rock at the top of the land and 150 m under it, and the basement of the land, read from the flat
// view as `terra planet` reads them from the flat history (tool/PlanetCommand.cpp saveGeology, without
// its heat and ore map), so the two compare against Earth's 64 (21) / 13 / 7 / 6. Each sample weighs the
// sphere's area it stands for, cos φ of its row (J's east factor / k; the meridian factor is the same
// on every row), so the rows near the poles, which the flat world stretches into full circles, do not
// count for more than they cover.
void printRocks(const Planet& view) {
    constexpr int Size = 1536;
    const double step = PlanetSpan / Size;
    double land = 0, kind[5] = {}, rock[2][5] = {};
    for (int z = 0; z < Size; ++z) {
        const double weight = bridgeJacobianAt((z + 0.5) * step).east / BridgeScale;
        for (int x = 0; x < Size; ++x) {
            const Vec2 at{(x + 0.5) * step, (z + 0.5) * step};
            if (view.elevation.at(int(at.x / PlanetCell), int(at.z / PlanetCell)) <= view.seaLevel) continue;
            const StrataColumn column = strataAt(view, at);
            land += weight;
            kind[size_t(column.basement)] += weight;
            for (int cut = 0; cut < 2; ++cut) {
                const float depth = std::max(0.0f, (cut ? 150.0f : 2.0f) + column.restored);
                const Material top = bedAt(view.seed, column, unitAt(column, depth, 0.0f), depth, column.material).rock;
                const Origin origin = definitionOf(top).origin;
                const bool carbonate = top == Material::Limestone || top == Material::Chalk;
                const bool plutonic = top == Material::Granite || top == Material::Peridotite || top == Material::Kimberlite;
                rock[cut][origin == Origin::Metamorphic ? 2 : origin == Origin::Igneous ? (plutonic ? 3 : 4) : 0] += weight;
                rock[cut][1] += carbonate ? weight : 0;
            }
        }
    }
    const auto share = [&](double count) { return 100.0 * count / std::max(1e-300, land); };
    for (int cut = 0; cut < 2; ++cut)
        std::printf("rock %s %%: sedimentary %.1f (carbonate %.1f) metamorphic %.1f plutonic %.1f volcanic %.1f   (Earth 64 (21) 13 7 6)\n",
                    cut ? "150 m under the land" : "at the top of the land", share(rock[cut][0]), share(rock[cut][1]), share(rock[cut][2]), share(rock[cut][3]),
                    share(rock[cut][4]));
    std::printf("basement of the land %%: primordial %.1f oceanic %.1f arc %.1f craton %.1f orogen %.1f\n", share(kind[0]), share(kind[1]), share(kind[2]),
                share(kind[3]), share(kind[4]));
}

// The grain of the shore against the mantle grid (S3-design.md §11 test 12, defect 5): at every cell
// where land meets sea across an edge, the direction of the shore from the Sobel gradient (Sobel and
// Feldman 1968, docs/credits/astronomy-geometry.md) of the elevation over the cell's 3 × 3 block of its
// face, as an angle against the face's grid lines. A shore
// with no preferred direction has 20° of 180° within ±5° of them, 11 %; the flat history's plates move
// along the axes and leave far more. Cells whose block leaves the face are skipped; areas weigh.
void printShoreGrain(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const int side = grid.side;
    double shore = 0, along = 0;
    for (int face = 0; face < 6; ++face)
        for (int j = 1; j + 1 < side; ++j)
            for (int i = 1; i + 1 < side; ++i) {
                const auto height = [&](int di, int dj) { return double(globe.elevation.value[size_t(indexOf(grid, Cell{face, grid.level, i + di, j + dj}))]) - globe.seaLevel; };
                const bool dry = height(0, 0) > 0;
                if ((height(1, 0) > 0) == dry && (height(-1, 0) > 0) == dry && (height(0, 1) > 0) == dry && (height(0, -1) > 0) == dry) continue;
                const double gi = height(1, -1) + 2 * height(1, 0) + height(1, 1) - height(-1, -1) - 2 * height(-1, 0) - height(-1, 1);
                const double gj = height(-1, 1) + 2 * height(0, 1) + height(1, 1) - height(-1, -1) - 2 * height(0, -1) - height(1, -1);
                if (gi == 0 && gj == 0) continue;
                const double angle = std::fmod(std::abs(std::atan2(gj, gi)) * 180 / 3.141592653589793, 90.0);
                const double area = grid.area[size_t(indexOf(grid, Cell{face, grid.level, i, j}))];
                shore += area;
                if (angle < 5 || angle > 85) along += area;
            }
    std::printf("shore within 5 deg of the mantle grid's lines %.1f %%   (no preferred direction 11 %%)\n", 100 * along / std::max(1e-300, shore));
}
