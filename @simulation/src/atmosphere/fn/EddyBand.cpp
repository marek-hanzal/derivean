#include "atmosphere/fn/EddyBand.hpp"

#include "atmosphere/fn/SphereHarmonics.hpp"
#include "geometry/constant/Sphere.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace derivean::sim {
namespace {

// Cells are worked through in blocks small enough that a block's recursion state stays in the first
// cache level while every (l, m) pair passes over it.
constexpr int Block = 256;

// Runs the recursion of P̄_lm over one block of cells, order by order, and hands each (l, m) with a
// positive gain to `use` with the block's P̄_lm · cos mλ and P̄_lm · sin mλ.
template <class Use>
void walkBlock(const EddyBand& band, int first, int count, Use use) {
    double diagonal[Block], cosM[Block], sinM[Block], before[Block], now[Block], next[Block], c[Block], s[Block];
    const int width = band.highest + 1;
    for (int k = 0; k < count; ++k) {
        diagonal[k] = 1;
        cosM[k] = 1;
        sinM[k] = 0;
    }
    for (int m = 1; m <= band.highest; ++m) {
        const double step = legendreDiagonal(m), upward = legendreNext(m);
        for (int k = 0; k < count; ++k) {
            const size_t cell = size_t(first + k);
            diagonal[k] *= step * band.cosine[cell];
            const double turned = cosM[k] * band.cosLongitude[cell] - sinM[k] * band.sinLongitude[cell];
            sinM[k] = sinM[k] * band.cosLongitude[cell] + cosM[k] * band.sinLongitude[cell];
            cosM[k] = turned;
            before[k] = 0;
            now[k] = diagonal[k];
        }
        for (int l = m; l <= band.highest; ++l) {
            if (l > m) {
                const double a = l == m + 1 ? upward : band.a[size_t(l * width + m)];
                const double b = l == m + 1 ? 0.0 : band.b[size_t(l * width + m)];
                for (int k = 0; k < count; ++k) {
                    next[k] = a * band.sine[size_t(first + k)] * now[k] - b * before[k];
                    before[k] = now[k];
                    now[k] = next[k];
                }
            }
            if (l < band.lowest) continue;
            for (int k = 0; k < count; ++k) {
                c[k] = now[k] * cosM[k];
                s[k] = now[k] * sinM[k];
            }
            use(l, m, c, s);
        }
    }
}

} // namespace

EddyBand makeEddyBand(const SphereGrid& grid, double wavelength, double reachShare) {
    EddyBand band;
    band.centre = 2 * std::numbers::pi / wavelength;
    band.reach = reachShare * band.centre;
    const double top = (band.centre + band.reach) * PlanetRadius;
    band.highest = int(std::floor(0.5 * (std::sqrt(1 + 4 * top * top) - 1)));
    band.gain.assign(size_t(band.highest + 1), 0.0);
    band.lowest = band.highest + 1;
    for (int l = 1; l <= band.highest; ++l) {
        const double offset = (std::sqrt(double(l) * (l + 1)) / PlanetRadius - band.centre) / band.reach;
        band.gain[size_t(l)] = std::max(0.0, 1 - offset * offset);
        if (band.gain[size_t(l)] > 0) band.lowest = std::min(band.lowest, l);
    }
    const int width = band.highest + 1;
    band.a.assign(size_t(width * width), 0.0);
    band.b = band.a;
    for (int m = 0; m <= band.highest; ++m)
        for (int l = m + 2; l <= band.highest; ++l) {
            band.a[size_t(l * width + m)] = legendreA(l, m);
            band.b[size_t(l * width + m)] = legendreB(l, m);
        }
    const double sphere = 4 * std::numbers::pi * PlanetRadius * PlanetRadius;
    for (int c = 0; c < grid.cells; ++c) {
        const Vec3 d = grid.centre[size_t(c)];
        const double around = std::hypot(d.x, d.y);
        band.sine.push_back(d.z);
        band.cosine.push_back(around);
        band.cosLongitude.push_back(around > 0 ? d.x / around : 1.0);
        band.sinLongitude.push_back(around > 0 ? d.y / around : 0.0);
        band.weight.push_back(grid.area[size_t(c)] / sphere);
    }
    band.cosineTerms.assign(band.a.size(), 0.0);
    band.sineTerms = band.cosineTerms;
    return band;
}

double eddyGain(const EddyBand& band, int degree) { return degree <= band.highest ? band.gain[size_t(degree)] : 0.0; }

void filterEddyBand(EddyBand& band, const std::vector<double>& field, std::vector<double>& result) {
    const int cells = int(field.size()), width = band.highest + 1;
    std::fill(band.cosineTerms.begin(), band.cosineTerms.end(), 0.0);
    std::fill(band.sineTerms.begin(), band.sineTerms.end(), 0.0);
    double weighted[Block];
    for (int first = 0; first < cells; first += Block) {
        const int count = std::min(Block, cells - first);
        for (int k = 0; k < count; ++k) weighted[k] = band.weight[size_t(first + k)] * field[size_t(first + k)];
        walkBlock(band, first, count, [&](int l, int m, const double* c, const double* s) {
            double sumC = 0, sumS = 0;
            for (int k = 0; k < count; ++k) {
                sumC += weighted[k] * c[k];
                sumS += weighted[k] * s[k];
            }
            band.cosineTerms[size_t(l * width + m)] += sumC;
            band.sineTerms[size_t(l * width + m)] += sumS;
        });
    }
    for (int l = 0; l <= band.highest; ++l)
        for (int m = 0; m <= band.highest; ++m) {
            band.cosineTerms[size_t(l * width + m)] *= band.gain[size_t(l)];
            band.sineTerms[size_t(l * width + m)] *= band.gain[size_t(l)];
        }
    result.assign(size_t(cells), 0.0);
    for (int first = 0; first < cells; first += Block) {
        const int count = std::min(Block, cells - first);
        double* out = &result[size_t(first)];
        walkBlock(band, first, count, [&](int l, int m, const double* c, const double* s) {
            const double termC = band.cosineTerms[size_t(l * width + m)], termS = band.sineTerms[size_t(l * width + m)];
            for (int k = 0; k < count; ++k) out[k] += termC * c[k] + termS * s[k];
        });
    }
}

} // namespace derivean::sim
