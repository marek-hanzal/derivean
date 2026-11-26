#include "measure/fn/Regularity.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

bool wet(const Field<float>& surface, const Field<float>* water, int x, int z) { return water && water->at(x, z) > surface.at(x, z); }

// Reads the surface along a straight line by bilinear interpolation; false if the line leaves the
// field or crosses water.
bool readLine(const Field<float>& surface, const Field<float>* water, Vec2 start, Vec2 step, int count, std::vector<float>& profile) {
    profile.resize(size_t(count));
    for (int k = 0; k < count; ++k) {
        const double x = start.x + step.x * k, z = start.z + step.z * k;
        const int ix = int(std::floor(x)), iz = int(std::floor(z));
        if (ix < 0 || iz < 0 || ix + 1 >= surface.width || iz + 1 >= surface.height) return false;
        if (wet(surface, water, ix, iz) || wet(surface, water, ix + 1, iz) || wet(surface, water, ix, iz + 1) || wet(surface, water, ix + 1, iz + 1)) return false;
        const double tx = x - ix, tz = z - iz;
        profile[size_t(k)] = float(mix(mix(surface.at(ix, iz), surface.at(ix + 1, iz), tx), mix(surface.at(ix, iz + 1), surface.at(ix + 1, iz + 1), tx), tz));
    }
    return true;
}

} // namespace

CombReport combOf(const Field<float>& surface, double cell, const Field<float>* water, int window) {
    CombReport report;
    std::vector<float> strengths, roughness, spacings, depths, profile;
    int regular = 0, grid = 0, notched = 0;
    int64_t notches = 0;
    const double shallow = 0.002 * cell;
    const double middle = 0.5 * (window - 1);
    for (int z0 = 0; z0 + window <= surface.height; z0 += window / 2) {
        for (int x0 = 0; x0 + window <= surface.width; x0 += window / 2) {
            // Mean gradient of the window by least squares; the cross terms vanish on a square.
            double slopeX = 0, slopeZ = 0, weight = 0;
            for (int z = 0; z < window; ++z)
                for (int x = 0; x < window; ++x) {
                    const double height = surface.at(x0 + x, z0 + z);
                    slopeX += (x - middle) * height, slopeZ += (z - middle) * height;
                    weight += (x - middle) * (x - middle);
                }
            const Vec2 fall = normalized({slopeX / weight, slopeZ / weight});
            const Vec2 contour = perpendicular(fall);
            const Vec2 centre{x0 + middle, z0 + middle};
            for (int line = -1; line <= 1; ++line) {
                const Vec2 start = centre + fall * (line * window / 4.0) - contour * middle;
                if (!readLine(surface, water, start, contour, window, profile)) continue;
                const Periodicity found = periodicityOf(profile, Mark::Troughs);
                strengths.push_back(float(found.strength));
                roughness.push_back(float(cellRoughness(profile)));
                const int cuts = notchesIn(profile, NotchDepth * cell);
                notches += cuts;
                notched += cuts * GridSpacing >= window;
                if (!repeats(found) || found.height < shallow) continue;
                ++regular;
                grid += found.spacing <= GridSpacing;
                spacings.push_back(float(found.spacing));
                depths.push_back(float(found.height));
            }
        }
    }
    report.profiles = int(strengths.size());
    if (report.profiles > 0) {
        report.regularShare = double(regular) / report.profiles;
        report.gridShare = double(grid) / report.profiles;
        report.notchedShare = double(notched) / report.profiles;
        if (notches > 0) report.notchSpacing = double(report.profiles) * window / double(notches);
    }
    report.strength = distributionOf(std::move(strengths));
    report.roughness = distributionOf(std::move(roughness));
    report.spacing = medianOf(spacings);
    report.depth = medianOf(depths);
    return report;
}

SawtoothReport sawtoothOf(const Field<float>& surface, double cell, const Field<float>* water, int window) {
    SawtoothReport report;
    const double shallow = 0.002 * cell;
    std::vector<float> scatter, spacings, heights, profile;
    int regular = 0;
    constexpr double Diagonal = 0.70710678;
    const Vec2 directions[4] = {{1, 0}, {0, 1}, {Diagonal, Diagonal}, {Diagonal, -Diagonal}};
    for (const Vec2 direction : directions) {
        for (int z0 = 0; z0 < surface.height; z0 += window / 4) {
            for (int x0 = 0; x0 < surface.width; x0 += window / 4) {
                if (!readLine(surface, water, {double(x0), double(z0)}, direction, window, profile)) continue;
                const Periodicity found = periodicityOf(profile, Mark::Crests);
                if (found.marks < 5 || found.height < shallow) continue;
                scatter.push_back(float(found.spacingCv));
                if (found.spacingCv > SawSpacing || found.heightCv > SawHeight) continue;
                ++regular;
                spacings.push_back(float(found.spacing));
                heights.push_back(float(found.height));
            }
        }
    }
    report.lines = int(scatter.size());
    if (report.lines > 0) report.regularShare = double(regular) / report.lines;
    report.spacingCv = distributionOf(std::move(scatter));
    report.spacing = medianOf(spacings);
    report.height = medianOf(heights);
    return report;
}

double straightShoreShare(const Field<uint8_t>& land, bool wraps, int run) {
    const int width = land.width, height = land.height;
    const auto dry = [&](int x, int z) {
        if (wraps) return land.at(wrapIndex(x, width), wrapIndex(z, height)) != 0;
        return land.contains(x, z) && land.at(x, z) != 0;
    };
    const auto wet = [&](int x, int z) { return (wraps || land.contains(x, z)) && !dry(x, z); };
    std::vector<uint8_t> straight(land.size(), 0);
    constexpr int SideX[4] = {0, 0, -1, 1}, SideZ[4] = {-1, 1, 0, 0};
    for (int side = 0; side < 4; ++side) {
        // Water to the north or south makes runs along a row; to the west or east, along a column.
        const bool alongX = SideX[side] == 0;
        const int lines = alongX ? height : width, length = alongX ? width : height;
        for (int line = 0; line < lines; ++line) {
            int count = 0;
            for (int k = 0; k <= length; ++k) {
                const int x = alongX ? k : line, z = alongX ? line : k;
                const bool shore = k < length && dry(x, z) && wet(x + SideX[side], z + SideZ[side]);
                if (shore) {
                    ++count;
                    continue;
                }
                if (count >= run)
                    for (int back = 1; back <= count; ++back) straight[land.index(alongX ? k - back : line, alongX ? line : k - back)] = 1;
                count = 0;
            }
        }
    }
    size_t shore = 0, along = 0;
    for (int z = 0; z < height; ++z)
        for (int x = 0; x < width; ++x) {
            if (!dry(x, z) || !(wet(x + 1, z) || wet(x - 1, z) || wet(x, z + 1) || wet(x, z - 1))) continue;
            ++shore;
            along += straight[land.index(x, z)];
        }
    return shore ? double(along) / double(shore) : 0.0;
}

} // namespace derivean::sim
