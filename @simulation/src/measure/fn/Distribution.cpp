#include "measure/type/Distribution.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {

Distribution distributionOf(std::vector<float> values) {
    Distribution result;
    result.count = values.size();
    if (values.empty()) return result;
    std::sort(values.begin(), values.end());
    double sum = 0;
    for (float value : values) sum += value;
    result.mean = sum / double(values.size());
    result.low = values.front();
    result.high = values.back();
    const auto at = [&](double share) {
        const double position = share * double(values.size() - 1);
        const size_t below = size_t(position);
        const size_t above = std::min(values.size() - 1, below + 1);
        return values[below] + (values[above] - values[below]) * (position - double(below));
    };
    result.p05 = at(0.05), result.p25 = at(0.25), result.p50 = at(0.50);
    result.p75 = at(0.75), result.p95 = at(0.95), result.p99 = at(0.99);
    return result;
}

double medianOf(std::vector<float>& values) {
    if (values.empty()) return 0;
    std::nth_element(values.begin(), values.begin() + long(values.size() / 2), values.end());
    return values[values.size() / 2];
}

Histogram histogramOf(const std::vector<float>& values, double low, double width, int bins) {
    Histogram histogram{low, width, std::vector<uint32_t>(size_t(bins), 0)};
    for (float value : values) ++histogram.bins[size_t(std::clamp(int(std::floor((value - low) / width)), 0, bins - 1))];
    return histogram;
}

} // namespace derivean::sim
