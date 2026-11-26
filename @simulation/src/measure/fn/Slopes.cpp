#include "measure/fn/ReliefStats.hpp"

#include "measure/constant/Angle.hpp"

namespace derivean::sim {
namespace {

double degrees(double rise) { return std::atan(rise) * DegreesPerRadian; }

} // namespace

Field<float> slopeField(const Mosaic& mosaic) {
    const int side = mosaic.side();
    Field<float> slope(side, side, -1.0f);
    for (int z = 1; z + 1 < side; ++z) {
        for (int x = 1; x + 1 < side; ++x) {
            bool dry = true;
            for (int dz = -1; dz <= 1 && dry; ++dz)
                for (int dx = -1; dx <= 1; ++dx) dry = dry && mosaic.dry(x + dx, z + dz);
            if (!dry) continue;
            const auto h = [&](int dx, int dz) { return double(mosaic.ground.at(x + dx, z + dz)); };
            const double gx = (h(1, -1) + 2 * h(1, 0) + h(1, 1) - h(-1, -1) - 2 * h(-1, 0) - h(-1, 1)) / (8 * mosaic.cell);
            const double gz = (h(-1, 1) + 2 * h(0, 1) + h(1, 1) - h(-1, -1) - 2 * h(0, -1) - h(1, -1)) / (8 * mosaic.cell);
            slope.at(x, z) = float(std::hypot(gx, gz));
        }
    }
    return slope;
}

Field<float> steepestFall(const Mosaic& mosaic) {
    const int side = mosaic.side();
    Field<float> fall(side, side, -1.0f);
    for (int z = 1; z + 1 < side; ++z) {
        for (int x = 1; x + 1 < side; ++x) {
            if (!mosaic.dry(x, z)) continue;
            float steepest = 0;
            for (int k = 0; k < 8; ++k)
                steepest = std::max(steepest, (mosaic.ground.at(x, z) - mosaic.top(x + NeighborX[k], z + NeighborZ[k])) / (NeighborReach[k] * float(mosaic.cell)));
            fall.at(x, z) = steepest;
        }
    }
    return fall;
}

SlopeStats slopeStats(const Field<float>& slope) {
    std::vector<float> angles;
    SlopeStats stats;
    for (const float rise : slope.cells) {
        if (rise < 0) continue;
        const double angle = degrees(rise);
        angles.push_back(float(angle));
        stats.above30 += angle > 30, stats.above45 += angle > 45, stats.above60 += angle > 60;
    }
    stats.cells = angles.size();
    if (angles.empty()) return stats;
    std::sort(angles.begin(), angles.end());
    const auto at = [&](double share) { return double(angles[size_t(share * double(angles.size() - 1))]); };
    double sum = 0;
    for (const float angle : angles) sum += angle;
    stats.mean = sum / double(angles.size());
    stats.p10 = at(0.10), stats.p50 = at(0.50), stats.p90 = at(0.90), stats.p99 = at(0.99);
    stats.above30 /= double(angles.size()), stats.above45 /= double(angles.size()), stats.above60 /= double(angles.size());
    return stats;
}

WindowRelief windowRelief(const Mosaic& mosaic, double window) {
    WindowRelief relief;
    relief.window = window;
    const int span = int(std::lround(window / mosaic.cell)), side = mosaic.side();
    if (span < 2 || span > side) return relief;
    std::vector<float> found;
    for (int z = 0; z + span <= side; z += std::max(1, span / 2)) {
        for (int x = 0; x + span <= side; x += std::max(1, span / 2)) {
            float low = 1e30f, high = -1e30f;
            for (int dz = 0; dz < span; ++dz)
                for (int dx = 0; dx < span; ++dx) low = std::min(low, mosaic.top(x + dx, z + dz)), high = std::max(high, mosaic.top(x + dx, z + dz));
            found.push_back(high - low);
        }
    }
    std::sort(found.begin(), found.end());
    relief.count = int(found.size());
    relief.p50 = found[found.size() / 2], relief.p90 = found[size_t(0.9 * double(found.size() - 1))], relief.high = found.back();
    return relief;
}

CoverShares coverShares(const Mosaic& mosaic, const Field<float>& slope, const Field<float>& fall) {
    CoverShares shares;
    const int side = mosaic.side();
    std::vector<float> talus;
    double cover = 0;
    size_t dry = 0;
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            Exposed exposed;
            exposed.coarse = mosaic.coarse.at(x, z);
            exposed.sand = mosaic.cover.at(x, z) - exposed.coarse;
            exposed.standing = mosaic.dry(x, z) ? 0.0f : 1.0f;
            exposed.ice = mosaic.ice.at(x, z);
            exposed.slope = std::max(0.0f, slope.at(x, z));
            const Face face = faceOf(exposed);
            // Without the split of the finer classes everything that is not coarse counts as fines.
            shares.share[size_t(face == Face::Sand ? Face::Fines : face)] += 1.0 / double(mosaic.ground.size());
            if (!mosaic.dry(x, z)) continue;
            cover += mosaic.cover.at(x, z), ++dry;
            // Debris lies at the angle it falls away at; the gradient over nine cells would read the wall behind it.
            if (fall.at(x, z) >= 0 && exposed.coarse >= 1 && exposed.coarse >= CoarseShare * mosaic.cover.at(x, z)) talus.push_back(float(degrees(fall.at(x, z))));
        }
    }
    shares.meanCover = dry ? cover / double(dry) : 0.0;
    shares.talusAngle = distributionOf(std::move(talus));
    return shares;
}

ReliefReport reliefReport(const Mosaic& mosaic) {
    ReliefReport report;
    const Field<float> slope = slopeField(mosaic);
    report.slope = slopeStats(slope);
    const double windows[3] = {1000, 5000, 16000};
    for (int k = 0; k < 3; ++k) report.relief[k] = windowRelief(mosaic, windows[k]);
    const Field<float> fall = steepestFall(mosaic);
    report.cover = coverShares(mosaic, slope, fall);
    report.drainage = drainageDensity(mosaic);
    report.valleys = valleySections(mosaic);
    report.crests = crestSharpness(mosaic);
    report.cliffs = cliffInventory(mosaic, fall);
    return report;
}

Mosaic mosaicOf(const Domain& domain) {
    Mosaic mosaic;
    mosaic.level = domain.key.level;
    mosaic.first = domain.key;
    mosaic.tiles = 1;
    mosaic.cell = domain.cell;
    mosaic.seaLevel = domain.seaLevel;
    for (Field<float>* field : {&mosaic.ground, &mosaic.water, &mosaic.discharge, &mosaic.cover, &mosaic.coarse, &mosaic.datum, &mosaic.rain, &mosaic.warmth, &mosaic.ice, &mosaic.iceAge})
        *field = Field<float>(TileCells, TileCells);
    mosaic.flowTo = Field<uint8_t>(TileCells, TileCells, uint8_t(255));
    mosaic.rock = Field<uint8_t>(TileCells, TileCells);
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = domain.index(x + Halo, z + Halo);
            const float cover = domain.sedimentAt(i), ground = domain.ground.rock[i] + cover;
            mosaic.ground.at(x, z) = ground;
            mosaic.cover.at(x, z) = cover, mosaic.coarse.at(x, z) = domain.ground.sediment[Coarse][i];
            mosaic.datum.at(x, z) = domain.ground.datum[i];
            mosaic.water.at(x, z) = domain.level[i] > ground + 1e-3f ? domain.level[i] : ground - 1.0f;
            mosaic.discharge.at(x, z) = domain.discharge[i];
            mosaic.rain.at(x, z) = domain.rain[i], mosaic.warmth.at(x, z) = domain.warmthNow[i];
            mosaic.ice.at(x, z) = domain.ice[i], mosaic.iceAge.at(x, z) = domain.iceAge[i];
            mosaic.rock.at(x, z) = uint8_t(domain.unit[i].traits.dominant);
            float steepest = 0;
            for (int k = 0; k < 8; ++k) {
                const size_t other = domain.index(x + Halo + NeighborX[k], z + Halo + NeighborZ[k]);
                const float fall = (domain.level[i] - domain.level[other]) / NeighborReach[k];
                if (fall > steepest) steepest = fall, mosaic.flowTo.at(x, z) = uint8_t(k);
            }
        }
    }
    return mosaic;
}

} // namespace derivean::sim
