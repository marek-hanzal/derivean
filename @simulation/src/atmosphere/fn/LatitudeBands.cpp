#include "atmosphere/fn/LatitudeBands.hpp"

#include "geometry/constant/Sphere.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

namespace derivean::sim {

LatitudeBands makeLatitudeBands(const SphereGrid& grid, int count) {
    assert(count >= 2);
    constexpr double Pi = std::numbers::pi;
    LatitudeBands bands;
    bands.count = count;
    bands.width = Pi / count;
    bands.centre.resize(size_t(count));
    for (int b = 0; b < count; ++b) bands.centre[size_t(b)] = -Pi / 2 + (b + 0.5) * bands.width;
    bands.first.resize(size_t(grid.cells));
    bands.share.resize(size_t(grid.cells));
    bands.area.assign(size_t(count), 0.0);
    for (int c = 0; c < grid.cells; ++c) {
        const double latitude = std::asin(std::clamp(grid.centre[size_t(c)].z, -1.0, 1.0));
        const double place = std::clamp((latitude - bands.centre[0]) / bands.width, 0.0, double(count - 1));
        const int first = std::min(int(std::floor(place)), count - 2);
        const double share = place - first;
        bands.first[size_t(c)] = first;
        bands.share[size_t(c)] = share;
        const double area = grid.area[size_t(c)];
        const double south = area * (1 - share), north = area * share;
        bands.area[size_t(first)] += south;
        bands.area[size_t(first) + 1] += north;
    }
    double total = 0;
    for (double area : bands.area) {
        assert(area > 0);
        total += area;
    }
    bands.boundary.assign(size_t(count) + 1, 0.0);
    bands.rim.assign(size_t(count) + 1, 0.0);
    bands.boundary[0] = -1, bands.boundary[size_t(count)] = 1;
    double south = 0;
    for (int b = 1; b < count; ++b) {
        south += bands.area[size_t(b) - 1];
        const double sine = 2 * south / total - 1;
        bands.boundary[size_t(b)] = sine;
        bands.rim[size_t(b)] = 2 * Pi * PlanetRadius * std::sqrt(std::max(0.0, 1 - sine * sine));
    }
    return bands;
}

void bandMeans(const LatitudeBands& bands, const SphereGrid& grid, const std::vector<double>& field, std::vector<double>& mean) {
    assert(field.size() == bands.first.size());
    mean.assign(size_t(bands.count), 0.0);
    for (size_t c = 0; c < field.size(); ++c) {
        const double content = grid.area[c] * field[c];
        const double south = content * (1 - bands.share[c]), north = content * bands.share[c];
        mean[size_t(bands.first[c])] += south;
        mean[size_t(bands.first[c]) + 1] += north;
    }
    for (int b = 0; b < bands.count; ++b) mean[size_t(b)] /= bands.area[size_t(b)];
}

void spreadBands(const LatitudeBands& bands, const std::vector<double>& perBand, std::vector<double>& field) {
    assert(perBand.size() == size_t(bands.count));
    field.resize(bands.first.size());
    for (size_t c = 0; c < field.size(); ++c) {
        const double south = (1 - bands.share[c]) * perBand[size_t(bands.first[c])];
        const double north = bands.share[c] * perBand[size_t(bands.first[c]) + 1];
        field[c] = south + north;
    }
}

void zonalWindVorticity(const LatitudeBands& bands, const std::vector<double>& eastward, std::vector<double>& vorticity) {
    assert(eastward.size() == size_t(bands.count) + 1);
    vorticity.resize(size_t(bands.count));
    for (int b = 0; b < bands.count; ++b) {
        const double south = bands.rim[size_t(b)] * eastward[size_t(b)];
        const double north = bands.rim[size_t(b) + 1] * eastward[size_t(b) + 1];
        vorticity[size_t(b)] = (south - north) / bands.area[size_t(b)];
    }
}

} // namespace derivean::sim
