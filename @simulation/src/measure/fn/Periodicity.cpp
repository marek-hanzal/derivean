#include "measure/fn/Periodicity.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// Removes the running mean over a quarter of the profile and returns the variance of what is left.
double highPass(const std::vector<float>& profile, std::vector<double>& detail) {
    const int n = int(profile.size()), reach = std::max(1, n / 8);
    std::vector<double> sum(size_t(n) + 1, 0.0);
    for (int i = 0; i < n; ++i) sum[size_t(i) + 1] = sum[size_t(i)] + profile[size_t(i)];
    detail.resize(size_t(n));
    double mean = 0;
    for (int i = 0; i < n; ++i) {
        const int low = std::max(0, i - reach), high = std::min(n, i + reach + 1);
        detail[size_t(i)] = profile[size_t(i)] - (sum[size_t(high)] - sum[size_t(low)]) / double(high - low);
        mean += detail[size_t(i)];
    }
    mean /= n;
    double variance = 0;
    for (double& value : detail) {
        value -= mean;
        variance += value * value;
    }
    return variance / n;
}

// Height of a local maximum above the higher of the two saddles that separate it from higher ground.
double prominence(const std::vector<double>& detail, int peak) {
    const int n = int(detail.size());
    double saddle[2] = {detail[size_t(peak)], detail[size_t(peak)]};
    for (int side = 0; side < 2; ++side) {
        const int step = side ? 1 : -1;
        for (int i = peak + step; i >= 0 && i < n && detail[size_t(i)] <= detail[size_t(peak)]; i += step)
            saddle[side] = std::min(saddle[side], detail[size_t(i)]);
    }
    return detail[size_t(peak)] - std::max(saddle[0], saddle[1]);
}

void spread(const std::vector<double>& values, double& mean, double& cv) {
    mean = cv = 0;
    if (values.empty()) return;
    for (double value : values) mean += value;
    mean /= double(values.size());
    double variance = 0;
    for (double value : values) variance += (value - mean) * (value - mean);
    cv = mean > 0 ? std::sqrt(variance / double(values.size())) / mean : 0.0;
}

} // namespace

Periodicity periodicityOf(const std::vector<float>& profile, Mark mark) {
    Periodicity result;
    const int n = int(profile.size());
    if (n < 16) return result;
    std::vector<double> detail;
    const double variance = highPass(profile, detail);
    if (variance < 1e-12) return result;

    bool fell = false;
    double before = 1;
    for (int lag = 1; lag <= n / 2; ++lag) {
        double sum = 0;
        for (int i = 0; i + lag < n; ++i) sum += detail[size_t(i)] * detail[size_t(i + lag)];
        const double here = sum / (double(n - lag) * variance);
        fell = fell || here < 0;
        if (fell && here > result.strength && here >= before) result.strength = here, result.lag = lag;
        before = here;
    }
    result.strength = std::min(1.0, result.strength);

    if (mark == Mark::Troughs)
        for (double& value : detail) value = -value;
    std::vector<double> sorted = detail;
    std::sort(sorted.begin(), sorted.end());
    const double range = sorted[size_t(0.95 * (n - 1))] - sorted[size_t(0.05 * (n - 1))];
    std::vector<double> gaps, heights;
    int last = -1;
    for (int i = 1; i + 1 < n; ++i) {
        if (!(detail[size_t(i)] > detail[size_t(i - 1)] && detail[size_t(i)] >= detail[size_t(i + 1)])) continue;
        const double height = prominence(detail, i);
        if (height < 0.2 * range) continue;
        if (last >= 0) gaps.push_back(i - last);
        heights.push_back(height);
        last = i;
    }
    result.marks = int(heights.size());
    spread(gaps, result.spacing, result.spacingCv);
    spread(heights, result.height, result.heightCv);
    return result;
}

double cellRoughness(const std::vector<float>& profile) {
    const int n = int(profile.size());
    if (n < 16) return 0;
    double near = 0, far = 0;
    for (int i = 0; i + 1 < n; ++i) near += double(profile[size_t(i + 1)] - profile[size_t(i)]) * (profile[size_t(i + 1)] - profile[size_t(i)]);
    for (int i = 0; i + 8 < n; ++i) far += double(profile[size_t(i + 8)] - profile[size_t(i)]) * (profile[size_t(i + 8)] - profile[size_t(i)]);
    near /= n - 1, far /= n - 8;
    return far > 0 ? std::min(1.0, near / far) : 0.0;
}

int notchesIn(const std::vector<float>& profile, double depth, int reach) {
    const int n = int(profile.size());
    int count = 0;
    for (int i = reach; i + reach < n; ++i) {
        const float here = profile[size_t(i)];
        if (here > profile[size_t(i - 1)] || here >= profile[size_t(i + 1)]) continue;
        float before = here, after = here;
        for (int k = 1; k <= reach; ++k) before = std::max(before, profile[size_t(i - k)]), after = std::max(after, profile[size_t(i + k)]);
        count += before - here >= depth && after - here >= depth;
    }
    return count;
}

} // namespace derivean::sim
