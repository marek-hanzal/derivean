#include "atmosphere/fn/SphereHarmonics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace derivean::sim {

// Holmes & Featherstone (2002), eqs. 11–13, with t = sin φ and u = cos φ.
double legendreA(int l, int m) { return std::sqrt(double(2 * l - 1) * (2 * l + 1) / (double(l - m) * (l + m))); }
double legendreB(int l, int m) {
    return std::sqrt(double(2 * l + 1) * (l + m - 1) * (l - m - 1) / (double(l - m) * (l + m) * (2 * l - 3)));
}
double legendreDiagonal(int m) { return m == 1 ? std::sqrt(3.0) : std::sqrt(double(2 * m + 1) / (2 * m)); }
double legendreNext(int m) { return std::sqrt(double(2 * m + 3)); }

double harmonicAt(int degree, int order, double t, double longitude) {
    const int m = std::abs(order);
    const double u = std::sqrt(std::max(0.0, 1 - t * t));
    double diagonal = 1;
    for (int k = 1; k <= m; ++k) diagonal *= legendreDiagonal(k) * u;
    double before = 0, value = diagonal;
    if (degree > m) {
        before = value;
        value = legendreNext(m) * t * diagonal;
        for (int l = m + 2; l <= degree; ++l) {
            const double next = legendreA(l, m) * t * value - legendreB(l, m) * before;
            before = value;
            value = next;
        }
    }
    if (m == 0) return value;
    return value * (order > 0 ? std::cos(m * longitude) : std::sin(m * longitude));
}

} // namespace derivean::sim
