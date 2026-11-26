#pragma once

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace derivean::sim::geometry_test {

inline void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Geometry: " << message << " (" << value << ")\n";
    std::exit(1);
}

inline bool near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }
inline bool same(Vec3 a, Vec3 b, double tolerance) { return length(a - b) <= tolerance; }

// Cells on every face at a level: the four face corners, edge cells, the centre and a scatter inside.
inline std::vector<Cell> samplesAt(int level) {
    const int last = cellsPerSide(level) - 1, mid = last / 2;
    const int picks[] = {0, 1, mid, last - 1, last, int(0.37 * last), int(0.81 * last)};
    std::vector<Cell> cells;
    for (int face = 0; face < FaceCount; ++face)
        for (int i : picks)
            for (int j : picks)
                if (i >= 0 && j >= 0 && i <= last && j <= last) cells.push_back({face, level, i, j});
    return cells;
}

inline constexpr int SampleLevels[] = {0, 1, 3, RegionLevel, LocalLevel, SiteLevel, GroundLevel, GroundCellLevel, MaxLevel};

void checkFrames();

} // namespace derivean::sim::geometry_test
