#include "measure/fn/Plane.hpp"

#include "landscape/fn/Process.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Regularity.hpp"

namespace derivean::sim {
namespace {

constexpr int Chord = 16;         // cells a path is followed to read its heading
constexpr int GullyWindow = 16;   // cells along a contour within which crest and trough are paired
constexpr size_t MinContour = 32;  // cells a contour needs for its mean to be one
constexpr int AxisBand = 8;       // cells either side of the middle fall line that count as the valley of a swale

// The tile's own cells in the frame of the plane: cells along the fall line and across it, from the middle.
struct Frame {
    const Domain& domain;
    Vec2 fall;
    double middle;

    double along(int x, int z) const { return dot({x - middle, z - middle}, fall); }
    double across(int x, int z) const { return cross(fall, {x - middle, z - middle}); }
    float residual(int x, int z) const {
        const size_t i = domain.index(x, z);
        return domain.ground.rock[i] + domain.sedimentAt(i) - domain.trend[i];
    }
    // Bilinear residual at a fractional position; false outside the tile's own cells.
    bool read(Vec2 at, float& value) const {
        const int x = int(std::floor(at.x)), z = int(std::floor(at.z));
        if (!domain.owned(x, z) || !domain.owned(x + 1, z + 1)) return false;
        const double tx = at.x - x, tz = at.z - z;
        value = float(mix(mix(residual(x, z), residual(x + 1, z), tx), mix(residual(x, z + 1), residual(x + 1, z + 1), tx), tz));
        return true;
    }
};

// Contour-parallel lines every eight cells down the plane: their gullies and how regularly they repeat.
void readContours(const Frame& frame, PlaneReport& report) {
    // Lines start from a cell centre, so on a plane that falls along the grid every sample is one
    // cell's own value and a comb two cells wide is not averaged away.
    const Vec2 contour = perpendicular(frame.fall), centre{std::floor(frame.middle), std::floor(frame.middle)};
    std::vector<float> gullies, strengths, roughness, profile;
    int regular = 0, grid = 0;
    int64_t notches = 0, samples = 0;
    for (int down = -TileCells; down <= TileCells; down += 8) {
        profile.clear();
        for (int k = -TileCells; k <= TileCells; ++k) {
            float value;
            if (frame.read(centre + frame.fall * double(down) + contour * double(k), value)) profile.push_back(value);
            else if (!profile.empty()) break;
        }
        if (profile.size() < 64) continue;
        for (size_t start = 0; start + GullyWindow <= profile.size(); start += GullyWindow / 2) {
            const auto [low, high] = std::minmax_element(profile.begin() + long(start), profile.begin() + long(start + GullyWindow));
            gullies.push_back(*high - *low);
        }
        const Periodicity found = periodicityOf(profile, Mark::Troughs);
        strengths.push_back(float(found.strength));
        roughness.push_back(float(cellRoughness(profile)));
        notches += notchesIn(profile, NotchDepth * frame.domain.cell);
        samples += int64_t(profile.size());
        if (!repeats(found)) continue;
        ++regular;
        grid += found.spacing <= GridSpacing;
    }
    if (!strengths.empty()) {
        report.regularShare = double(regular) / double(strengths.size());
        report.gridShare = double(grid) / double(strengths.size());
    }
    if (notches > 0) report.notchSpacing = double(samples) / double(notches);
    report.gully = distributionOf(std::move(gullies));
    report.strength = distributionOf(std::move(strengths));
    report.roughness = distributionOf(std::move(roughness));
}

// Mean angle between the fall line and the paths of cells that carry twice their contour's mean flow.
void readHeadings(const Frame& frame, const Domain& flow, const FlowScratch& scratch, const std::vector<Mean>& contourFlow, double reach, PlaneReport& report) {
    const Domain& domain = frame.domain;
    Mean error;
    for (int z0 = Halo; z0 < Halo + TileCells; ++z0) {
        for (int x0 = Halo; x0 < Halo + TileCells; ++x0) {
            const Mean& contour = contourFlow[size_t(std::lround(frame.along(x0, z0) + reach))];
            if (contour.count < MinContour || flow.discharge[domain.index(x0, z0)] < 2.0 * contour.value()) continue;
            int x = x0, z = z0, taken = 0;
            for (; taken < Chord; ++taken) {
                const float* share = &scratch.share[domain.index(x, z) * 8];
                int main = -1;
                for (int k = 0; k < 8; ++k)
                    if (share[k] > 0 && (main < 0 || share[k] > share[main])) main = k;
                if (main < 0) break;
                x += NeighborX[main], z += NeighborZ[main];
                if (x <= 0 || z <= 0 || x >= domain.size - 1 || z >= domain.size - 1) break;
            }
            if (taken < Chord) continue;
            const Vec2 heading = normalized({double(x - x0), double(z - z0)});
            error.add(std::abs(std::atan2(cross(frame.fall, heading), dot(frame.fall, heading))) * DegreesPerRadian);
        }
    }
    report.headingError = error.value();
    report.paths = error.count;
}

} // namespace

PlaneReport measurePlane(const PlaneSpec& spec, const Domain& domain) {
    PlaneReport report;
    const Frame frame{domain, {std::cos(spec.azimuth), std::sin(spec.azimuth)}, 0.5 * (domain.size - 1)};
    const double reach = 0.5 * domain.size * (std::abs(frame.fall.x) + std::abs(frame.fall.z)) + 1;
    report.fall = spec.slope * domain.cell * TileCells * (std::abs(frame.fall.x) + std::abs(frame.fall.z));

    Domain flow = domain;
    FlowScratch scratch;
    floodLevels(flow, scratch);
    routeWater(flow, scratch);

    const size_t bins = size_t(2 * reach + 2);
    std::vector<Mean> contourHeight(bins), contourFlow(bins), crossHeight(bins);
    Mean all;
    const auto owned = [&](auto&& visit) {
        for (int z = Halo; z < Halo + TileCells; ++z)
            for (int x = Halo; x < Halo + TileCells; ++x) visit(x, z, size_t(std::lround(frame.along(x, z) + reach)));
    };
    owned([&](int x, int z, size_t contour) {
        contourHeight[contour].add(frame.residual(x, z));
        contourFlow[contour].add(flow.discharge[domain.index(x, z)]);
        all.add(frame.residual(x, z));
    });
    report.lowered = -all.value();

    // Lowering against discharge, both taken against the contour the cell lies on.
    double sumX = 0, sumY = 0, sumXX = 0, sumYY = 0, sumXY = 0;
    int pairs = 0;
    Mean axis, flank;
    owned([&](int x, int z, size_t contour) {
        const double across = frame.across(x, z), lower = contourHeight[contour].value() - frame.residual(x, z);
        crossHeight[size_t(std::lround(across + reach))].add(-lower);
        if (frame.along(x, z) > 0 && std::abs(across) <= AxisBand) axis.add(frame.residual(x, z));
        if (frame.along(x, z) > 0 && std::abs(across) >= 3 * AxisBand && std::abs(across) <= 12 * AxisBand) flank.add(frame.residual(x, z));
        const double water = flow.discharge[domain.index(x, z)], mean = contourFlow[contour].value();
        if (contourFlow[contour].count < MinContour || water <= 0 || mean <= 0) return;
        const double doubling = std::log2(water / mean);
        sumX += doubling, sumY += lower, sumXX += doubling * doubling, sumYY += lower * lower, sumXY += doubling * lower;
        ++pairs;
    });
    report.axisDepth = flank.value() - axis.value();
    if (pairs > 2) {
        const double varX = sumXX - sumX * sumX / pairs, varY = sumYY - sumY * sumY / pairs, cov = sumXY - sumX * sumY / pairs;
        report.incisionPerDoubling = varX > 0 ? cov / varX : 0.0;
        report.correlation = varX > 0 && varY > 0 ? cov / std::sqrt(varX * varY) : 0.0;
    }

    for (const Mean& bin : crossHeight)
        if (bin.count >= MinContour) report.cross.push_back(float(bin.value()));
    report.crossPattern = periodicityOf(report.cross, Mark::Troughs);
    const Distribution range = distributionOf(report.cross);
    report.crossRange = range.p95 - range.p05;

    // The foot: a contour three quarters of the way down the tile's own cells.
    const double footAlong = 0.75 * 0.5 * TileCells * (std::abs(frame.fall.x) + std::abs(frame.fall.z));
    std::vector<float> foot;
    double total = 0, middle = 0, offAxis = 0, width = 0;
    owned([&](int x, int z, size_t) {
        if (std::lround(frame.along(x, z)) != std::lround(footAlong)) return;
        const float water = flow.discharge[domain.index(x, z)];
        foot.push_back(water);
        total += water;
        offAxis += water * std::abs(frame.across(x, z));
        width += std::abs(frame.across(x, z));
        if (std::abs(frame.across(x, z)) <= AxisBand) middle += water;
    });
    std::sort(foot.begin(), foot.end(), std::greater<float>());
    double wettest = 0;
    for (size_t i = 0; i < foot.size() / 10; ++i) wettest += foot[i];
    if (total > 0) {
        report.footShare = wettest / total, report.axisShare = middle / total;
        report.gathered = 1.0 - (offAxis / total) / (width / double(foot.size()));
    }

    readContours(frame, report);
    readHeadings(frame, flow, scratch, contourFlow, reach, report);
    return report;
}

} // namespace derivean::sim
