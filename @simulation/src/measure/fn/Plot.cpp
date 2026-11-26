#include "measure/fn/Plot.hpp"

#include "measure/constant/Angle.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

void paint(std::vector<uint8_t>& image, int width, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    const size_t at = (size_t(y) * size_t(width) + size_t(x)) * 3;
    image[at] = r, image[at + 1] = g, image[at + 2] = b;
}

} // namespace

std::vector<uint8_t> rosePlot(const Rose& rose, int size) {
    std::vector<uint8_t> image(size_t(size) * size_t(size) * 3, 255);
    const double middle = 0.5 * (size - 1), isotropic = 1.0 / RoseBins;
    const double largest = std::max(2.0 * isotropic, *std::max_element(rose.share.begin(), rose.share.end()));
    const double scale = (middle - 2) / largest;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double dx = x - middle, dy = y - middle, radius = std::sqrt(dx * dx + dy * dy);
            double angle = std::atan2(dy, dx) * DegreesPerRadian;
            angle -= 180.0 * std::floor(angle / 180.0);
            const double petal = rose.share[size_t(std::min(RoseBins - 1, int(angle / (180.0 / RoseBins))))] * scale;
            const double fromGrid = std::abs(angle - 45.0 * std::round(angle / 45.0)) / DegreesPerRadian * radius;
            if (radius <= petal) paint(image, size, x, y, 52, 96, 168);
            else if (fromGrid < 0.6 && radius < middle) paint(image, size, x, y, 200, 200, 200);
            if (std::abs(radius - isotropic * scale) < 0.8) paint(image, size, x, y, 210, 60, 50);
        }
    }
    return image;
}

std::vector<uint8_t> barPlot(const Histogram& histogram, int width, int height) {
    std::vector<uint8_t> image(size_t(width) * size_t(height) * 3, 255);
    if (histogram.bins.empty()) return image;
    const double largest = std::max<uint32_t>(1, *std::max_element(histogram.bins.begin(), histogram.bins.end()));
    for (int x = 0; x < width; ++x) {
        const size_t bin = size_t(int64_t(x) * int64_t(histogram.bins.size()) / width);
        const bool gap = size_t(int64_t(x + 1) * int64_t(histogram.bins.size()) / width) != bin;
        const int bar = gap ? 0 : int(std::lround(histogram.bins[bin] / largest * (height - 1)));
        for (int y = height - bar; y < height; ++y) paint(image, width, x, y, 52, 96, 168);
    }
    return image;
}

} // namespace derivean::sim
