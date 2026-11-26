#include "measure/fn/Orientation.hpp"

#include "landscape/type/Terrain.hpp"
#include "measure/constant/Angle.hpp"

#include <cmath>

namespace derivean::sim {

Rose roseOf(const std::vector<float>& angles) {
    Rose rose;
    rose.segments = int(angles.size());
    if (angles.empty()) return rose;
    double axis = 0, diagonal = 0;
    for (const float raw : angles) {
        const double angle = raw - 180.0 * std::floor(raw / 180.0);
        rose.share[size_t(std::min(RoseBins - 1, int(angle / (180.0 / RoseBins))))] += 1;
        const double fromGrid = std::abs(angle - 45.0 * std::round(angle / 45.0));
        if (fromGrid > GridBand) continue;
        (int(std::lround(angle / 45.0)) % 2 == 0 ? axis : diagonal) += 1;
    }
    const double count = double(angles.size());
    for (double& share : rose.share) share /= count;
    const double isotropic = 2 * GridBand / 90.0;   // share of the half circle near either pair of directions
    rose.axisExcess = axis / count / isotropic;
    rose.diagonalExcess = diagonal / count / isotropic;
    rose.gridExcess = 0.5 * (rose.axisExcess + rose.diagonalExcess);
    return rose;
}

double excessSigma(double excess, int segments, int chord, double bandShare) {
    const double independent = double(segments) / chord;
    if (independent < 1) return 0;
    return (excess - 1) * bandShare / std::sqrt(bandShare * (1 - bandShare) / independent);
}

std::vector<float> valleyAzimuths(const Mosaic& mosaic, double river, int chord) {
    std::vector<float> angles;
    const int side = mosaic.side();
    for (int z0 = 0; z0 < side; ++z0) {
        for (int x0 = 0; x0 < side; ++x0) {
            if (mosaic.discharge.at(x0, z0) < river || mosaic.flooded(x0, z0)) continue;
            int x = x0, z = z0, taken = 0;
            for (; taken < chord; ++taken) {
                const uint8_t to = mosaic.flowTo.at(x, z);
                if (to > 7) break;
                const int nx = x + NeighborX[to], nz = z + NeighborZ[to];
                if (nx < 0 || nz < 0 || nx >= side || nz >= side || mosaic.flooded(nx, nz)) break;
                x = nx, z = nz;
            }
            // A path that turns back on itself across a tile edge has no direction to give.
            if (taken < chord || std::hypot(x - x0, z - z0) < 0.5 * chord) continue;
            angles.push_back(float(std::atan2(double(z - z0), double(x - x0)) * DegreesPerRadian));
        }
    }
    return angles;
}

} // namespace derivean::sim
