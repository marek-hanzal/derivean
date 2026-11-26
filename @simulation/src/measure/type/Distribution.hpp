#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace derivean::sim {

// Summary of a sample; all zero for an empty one.
struct Distribution {
    size_t count{};
    double mean{}, low{}, high{};
    double p05{}, p25{}, p50{}, p75{}, p95{}, p99{};
};

Distribution distributionOf(std::vector<float> values);

// Middle value; reorders `values`. 0 for an empty sample.
double medianOf(std::vector<float>& values);

struct Mean {
    double sum{};
    size_t count{};
    void add(double value) { sum += value, ++count; }
    double value() const { return count ? sum / double(count) : 0.0; }
};

// Counts over `bins` equal classes from `low`; values outside fall into the first or last class.
struct Histogram {
    double low{}, width{};
    std::vector<uint32_t> bins;
};

Histogram histogramOf(const std::vector<float>& values, double low, double width, int bins);

} // namespace derivean::sim
