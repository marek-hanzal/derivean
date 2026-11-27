#include "atmosphere/fn/EddyBand.hpp"

#include "atmosphere/fn/SphereHarmonics.hpp"
#include "geometry/constant/Sphere.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>

namespace derivean::sim {
namespace {

// Groups are worked through in blocks small enough that a block's recursion state stays in the
// first cache level while every (l, m) pair passes over it.
constexpr int Block = 256;

// Cells whose |sin φ| differ by less than this from a group's first member join it. Cells the cube
// maps onto each other agree to one unit in the last place (2.2e-16 measured on levels 3 to 9);
// different latitudes lie further apart (1.2e-11 at level 9).
constexpr double SameLatitude = 1e-15;

// The recursion state of one block of groups for one order m, stepped from l = m upward.
struct Legendre {
    double before[Block], now[Block];
};

// Steps `state.now` from P̄_(l−1)m to P̄_lm for every group of the block (l > m), by the forward
// column recursion of SphereHarmonics.cpp (Holmes & Featherstone 2002, eqs. 11–13).
void stepDegree(const EddyBand& band, int first, int count, int l, int m, Legendre& state) {
    const int width = band.highest + 1;
    const double a = l == m + 1 ? legendreNext(m) : band.a[size_t(l * width + m)];
    const double b = l == m + 1 ? 0.0 : band.b[size_t(l * width + m)];
    for (int k = 0; k < count; ++k) {
        const double up = a * band.groupSine[size_t(first + k)] * state.now[k];
        const double back = b * state.before[k];
        const double next = up - back;
        state.before[k] = state.now[k];
        state.now[k] = next;
    }
}

// Starts order m on a block: P̄_mm from the groups' P̄_(m−1)(m−1), and cos mλ, sin mλ of the members
// from those of m − 1.
void startOrder(EddyBand& band, int first, int count, int m, Legendre& state) {
    const double step = legendreDiagonal(m);
    for (int k = 0; k < count; ++k) {
        double& diagonal = band.groupDiagonal[size_t(first + k)];
        diagonal *= step * band.groupCosine[size_t(first + k)];
        state.before[k] = 0;
        state.now[k] = diagonal;
    }
    for (int j = band.groupStart[size_t(first)]; j < band.groupStart[size_t(first + count)]; ++j) {
        const double c = band.memberCosM[size_t(j)], s = band.memberSinM[size_t(j)];
        const double cc = c * band.memberCos[size_t(j)], ss = s * band.memberSin[size_t(j)];
        const double sc = s * band.memberCos[size_t(j)], cs = c * band.memberSin[size_t(j)];
        band.memberCosM[size_t(j)] = cc - ss;
        band.memberSinM[size_t(j)] = sc + cs;
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
    std::vector<int> order(size_t(grid.cells));
    std::iota(order.begin(), order.end(), 0);
    auto latitude = [&](int c) { return std::abs(grid.centre[size_t(c)].z); };
    std::stable_sort(order.begin(), order.end(), [&](int p, int q) { return latitude(p) < latitude(q); });
    // Groups by |sin φ|, then each group's southern members moved behind its northern ones.
    std::vector<int> grouped;
    for (size_t k = 0; k < order.size(); ++k) {
        const int c = order[k];
        if (k == 0 || latitude(c) - latitude(grouped[size_t(band.groupStart.back())]) >= SameLatitude)
            band.groupStart.push_back(int(grouped.size()));
        grouped.push_back(c);
    }
    band.groupStart.push_back(int(grouped.size()));
    const double sphere = 4 * std::numbers::pi * PlanetRadius * PlanetRadius;
    for (size_t g = 0; g + 1 < band.groupStart.size(); ++g) {
        const auto first = grouped.begin() + band.groupStart[g], last = grouped.begin() + band.groupStart[g + 1];
        const auto south = std::stable_partition(first, last, [&](int c) { return grid.centre[size_t(c)].z >= 0; });
        band.groupSouth.push_back(int(south - grouped.begin()));
        const Vec3 d = grid.centre[size_t(*first)];
        band.groupSine.push_back(std::abs(d.z));
        band.groupCosine.push_back(std::hypot(d.x, d.y));
    }
    for (int c : grouped) {
        const Vec3 d = grid.centre[size_t(c)];
        const double around = std::hypot(d.x, d.y);
        band.memberCell.push_back(c);
        band.memberWeight.push_back(grid.area[size_t(c)] / sphere);
        band.memberCos.push_back(around > 0 ? d.x / around : 1.0);
        band.memberSin.push_back(around > 0 ? d.y / around : 0.0);
    }
    band.cosineTerms.assign(band.a.size(), 0.0);
    band.sineTerms = band.cosineTerms;
    band.memberValue.assign(size_t(grid.cells), 0.0);
    band.memberCosM = band.memberSinM = band.memberResult = band.memberValue;
    const size_t groups = band.groupSine.size();
    band.groupDiagonal.assign(groups, 0.0);
    band.groupEvenC = band.groupOddC = band.groupEvenS = band.groupOddS = band.groupDiagonal;
    band.legendre.assign(size_t(std::max(0, band.highest - band.lowest + 1)) * groups, 0.0);
    return band;
}

double eddyGain(const EddyBand& band, int degree) { return degree <= band.highest ? band.gain[size_t(degree)] : 0.0; }

// Order by order, m outermost, since the gain does not mix orders. Analysis: the coefficient of
// (l, m) is Σ over groups of P̄_lm(group) times the group's sum of w cos mλ (and w sin mλ), its
// southern members' sum with the sign (−1)^(l+m), summed block by block. Synthesis then reads the
// same order's P̄_lm and cos mλ, sin mλ, kept from the analysis, and adds per member cos mλ and sin mλ
// times the sums over l of coefficient · P̄_lm, split by the parity of l + m so the sign is applied
// once per order. The recursion and the rotation of the members' angles run once per order.
// Every product is its own statement, so no compiler fuses one into a multiply-add: which of two
// products a contraction picks depends on the surrounding code, and the result would change with it
// (docs/windows.md, determinism).
void filterEddyBand(EddyBand& band, const std::vector<double>& field, std::vector<double>& result) {
    const int width = band.highest + 1, groups = int(band.groupSine.size());
    std::fill(band.cosineTerms.begin(), band.cosineTerms.end(), 0.0);
    std::fill(band.sineTerms.begin(), band.sineTerms.end(), 0.0);
    std::fill(band.memberResult.begin(), band.memberResult.end(), 0.0);
    for (size_t j = 0; j < band.memberCell.size(); ++j) band.memberValue[j] = band.memberWeight[j] * field[size_t(band.memberCell[j])];
    std::fill(band.groupDiagonal.begin(), band.groupDiagonal.end(), 1.0);
    std::fill(band.memberCosM.begin(), band.memberCosM.end(), 1.0);
    std::fill(band.memberSinM.begin(), band.memberSinM.end(), 0.0);
    Legendre state;
    for (int m = 1; m <= band.highest; ++m) {
        const int low = std::max(m, band.lowest);
        for (int first = 0; first < groups; first += Block) {
            const int count = std::min(Block, groups - first);
            startOrder(band, first, count, m, state);
            for (int k = 0; k < count; ++k) {
                const int g = first + k, south = band.groupSouth[size_t(g)];
                double northC = 0, southC = 0, northS = 0, southS = 0;
                for (int j = band.groupStart[size_t(g)]; j < south; ++j) {
                    const double termC = band.memberValue[size_t(j)] * band.memberCosM[size_t(j)];
                    const double termS = band.memberValue[size_t(j)] * band.memberSinM[size_t(j)];
                    northC += termC;
                    northS += termS;
                }
                for (int j = south; j < band.groupStart[size_t(g) + 1]; ++j) {
                    const double termC = band.memberValue[size_t(j)] * band.memberCosM[size_t(j)];
                    const double termS = band.memberValue[size_t(j)] * band.memberSinM[size_t(j)];
                    southC += termC;
                    southS += termS;
                }
                band.groupEvenC[size_t(g)] = northC + southC;
                band.groupOddC[size_t(g)] = northC - southC;
                band.groupEvenS[size_t(g)] = northS + southS;
                band.groupOddS[size_t(g)] = northS - southS;
            }
            for (int l = m; l <= band.highest; ++l) {
                if (l > m) stepDegree(band, first, count, l, m, state);
                if (l < low) continue;
                double* kept = band.legendre.data() + size_t(l - low) * size_t(groups) + size_t(first);
                const double* c = ((l + m) % 2 == 0 ? band.groupEvenC : band.groupOddC).data() + first;
                const double* s = ((l + m) % 2 == 0 ? band.groupEvenS : band.groupOddS).data() + first;
                double sumC = 0, sumS = 0;
                for (int k = 0; k < count; ++k) {
                    kept[k] = state.now[k];
                    const double termC = state.now[k] * c[k], termS = state.now[k] * s[k];
                    sumC += termC;
                    sumS += termS;
                }
                band.cosineTerms[size_t(l * width + m)] += sumC;
                band.sineTerms[size_t(l * width + m)] += sumS;
            }
        }
        if (low > band.highest) continue;
        for (int l = low; l <= band.highest; ++l) {
            band.cosineTerms[size_t(l * width + m)] *= band.gain[size_t(l)];
            band.sineTerms[size_t(l * width + m)] *= band.gain[size_t(l)];
        }
        for (int first = 0; first < groups; first += Block) {
            const int count = std::min(Block, groups - first);
            double evenC[Block] = {}, oddC[Block] = {}, evenS[Block] = {}, oddS[Block] = {};
            for (int l = low; l <= band.highest; ++l) {
                const double termC = band.cosineTerms[size_t(l * width + m)], termS = band.sineTerms[size_t(l * width + m)];
                const double* kept = band.legendre.data() + size_t(l - low) * size_t(groups) + size_t(first);
                double* c = (l + m) % 2 == 0 ? evenC : oddC;
                double* s = (l + m) % 2 == 0 ? evenS : oddS;
                for (int k = 0; k < count; ++k) {
                    const double partC = termC * kept[k], partS = termS * kept[k];
                    c[k] += partC;
                    s[k] += partS;
                }
            }
            for (int k = 0; k < count; ++k) {
                const int g = first + k, south = band.groupSouth[size_t(g)];
                const double northC = evenC[k] + oddC[k], northS = evenS[k] + oddS[k];
                const double southC = evenC[k] - oddC[k], southS = evenS[k] - oddS[k];
                for (int j = band.groupStart[size_t(g)]; j < south; ++j) {
                    const double partC = band.memberCosM[size_t(j)] * northC, partS = band.memberSinM[size_t(j)] * northS;
                    band.memberResult[size_t(j)] += partC + partS;
                }
                for (int j = south; j < band.groupStart[size_t(g) + 1]; ++j) {
                    const double partC = band.memberCosM[size_t(j)] * southC, partS = band.memberSinM[size_t(j)] * southS;
                    band.memberResult[size_t(j)] += partC + partS;
                }
            }
        }
    }
    result.assign(band.memberCell.size(), 0.0);
    for (size_t j = 0; j < band.memberCell.size(); ++j) result[size_t(band.memberCell[j])] = band.memberResult[j];
}

} // namespace derivean::sim
