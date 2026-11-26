#include "measure/fn/Seams.hpp"

#include "landscape/type/Terrain.hpp"
#include "measure/type/Distribution.hpp"

#include <vector>

namespace derivean::sim {
namespace {

double ratio(double edge, double inside) { return inside > 0 ? edge / inside : edge > 0 ? 1e9 : 1.0; }

} // namespace

SeamReport seamsOf(const Mosaic& mosaic, double river) {
    SeamReport report;
    const int side = mosaic.side();
    const auto dry = [&](int x, int z) { return mosaic.dry(x, z); };

    // Steps and bends along both axes. A pair straddles an edge when its second cell opens a tile.
    Mean stepAcross, stepInside, bendAcross, bendInside;
    for (int axis = 0; axis < 2; ++axis) {
        const int dx = axis == 0, dz = axis == 1;
        for (int z = dz; z + 2 * dz < side; ++z) {
            for (int x = dx; x + 2 * dx < side; ++x) {
                if (!dry(x - dx, z - dz) || !dry(x, z) || !dry(x + dx, z + dz) || !dry(x + 2 * dx, z + 2 * dz)) continue;
                const float before = mosaic.ground.at(x, z) - mosaic.ground.at(x - dx, z - dz);
                const float step = mosaic.ground.at(x + dx, z + dz) - mosaic.ground.at(x, z);
                const float after = mosaic.ground.at(x + 2 * dx, z + 2 * dz) - mosaic.ground.at(x + dx, z + dz);
                const bool edge = ((axis == 0 ? x : z) + 1) % TileCells == 0;
                (edge ? stepAcross : stepInside).add(std::abs(step));
                (edge ? bendAcross : bendInside).add(0.5 * (std::abs(step - before) + std::abs(after - step)));
            }
        }
    }
    report.stepAcross = stepAcross.value(), report.stepInside = stepInside.value();
    report.bendAcross = bendAcross.value(), report.bendInside = bendInside.value();
    report.stepRatio = ratio(report.stepAcross, report.stepInside);
    report.bendRatio = ratio(report.bendAcross, report.bendInside);

    Mean lakeNear, lakeFar;
    std::vector<float> keptAcross, keptInside;
    size_t lostAcross = 0, lostInside = 0;
    int lostEarlier = 0, lostLater = 0;
    const auto passAt = [&](int x, int z) { return passOf({mosaic.level, mosaic.first.x + x / TileCells, mosaic.first.z + z / TileCells}); };
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            const int lx = x % TileCells, lz = z % TileCells;
            const bool near = lx < 3 || lz < 3 || lx >= TileCells - 3 || lz >= TileCells - 3;
            (near ? lakeNear : lakeFar).add(mosaic.lake(x, z));

            const uint8_t to = mosaic.flowTo.at(x, z);
            if (to > 7 || mosaic.discharge.at(x, z) < river || mosaic.flooded(x, z)) continue;
            const int nx = x + NeighborX[to], nz = z + NeighborZ[to];
            if (nx < 0 || nz < 0 || nx >= side || nz >= side) continue;
            const bool crosses = nx / TileCells != x / TileCells || nz / TileCells != z / TileCells;
            float next = mosaic.discharge.at(nx, nz);
            if (crosses) {
                for (int dz = -2; dz <= 2; ++dz)
                    for (int dx = -2; dx <= 2; ++dx) {
                        const int ox = nx + dx, oz = nz + dz;
                        if (ox < 0 || oz < 0 || ox >= side || oz >= side || ox / TileCells != nx / TileCells || oz / TileCells != nz / TileCells) continue;
                        next = std::max(next, mosaic.discharge.at(ox, oz));
                    }
            }
            const float kept = next / mosaic.discharge.at(x, z);
            (crosses ? keptAcross : keptInside).push_back(kept);
            if (kept < 0.7f) ++(crosses ? lostAcross : lostInside);
            if (!crosses) continue;
            const bool earlier = passAt(nx, nz) < passAt(x, z);
            ++(earlier ? report.intoEarlier : report.intoLater);
            if (kept < 0.7f) ++(earlier ? lostEarlier : lostLater);
        }
    }
    report.lakeNear = lakeNear.value(), report.lakeFar = lakeFar.value();
    report.lakeRatio = ratio(report.lakeNear, report.lakeFar);
    report.crossings = int(keptAcross.size());
    report.lostAcross = keptAcross.empty() ? 0.0 : double(lostAcross) / double(keptAcross.size());
    report.lostInside = keptInside.empty() ? 0.0 : double(lostInside) / double(keptInside.size());
    report.keptAcross = medianOf(keptAcross), report.keptInside = medianOf(keptInside);
    report.keptRatio = report.crossings > 0 ? ratio(report.keptAcross, report.keptInside) : 1.0;
    if (report.intoEarlier > 0) report.lostIntoEarlier = double(lostEarlier) / report.intoEarlier;
    if (report.intoLater > 0) report.lostIntoLater = double(lostLater) / report.intoLater;
    return report;
}

} // namespace derivean::sim
