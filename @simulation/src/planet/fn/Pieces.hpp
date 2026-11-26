#pragma once

#include "geometry/type/SpherePolygon.hpp"
#include "planet/type/Globe.hpp"

namespace derivean::sim {

// A globe at `level` holding the cooled primordial body at `time` (at most TectonicsStart): one piece
// per cell, born in it with Q = I, with the record of marks the closed-form cooling gives.
Globe makeGlobe(uint64_t seed, int level, PlateSpeeds speeds, double time);
template <class T> SphereField<T> sphereField(int level, T value = T{});

// The primordial column at a unit direction: the flat primordialColumn with its 2D noise replaced by
// 3D noise on the sphere at the same physical wavelength.
CrustColumn primordialColumnAt(uint64_t seed, Vec3 direction, int cell, double time);

// Where a piece is: its centre (Q · its birth cell's centre) and its footprint (Q · its birth cell).
Vec3 centreOf(const SphereGrid& grid, const CrustPiece& piece);
SpherePolygon footprintOf(const SphereGrid& grid, const CrustPiece& piece);

// The derived float thickness after a change of the stock.
void refreshColumn(CrustPiece& piece);

// Felsic crust out of and into a piece, m³. A debit is limited by what the piece holds and returns
// what it took, so the receiving account is credited with that, never with the request.
double takeFelsic(CrustPiece& piece, double volume);
void giveFelsic(CrustPiece& piece, double volume);

// A broken invariant of the history (a budget, a coverage, a plan): reported and fatal in every build,
// never normalised away.
[[noreturn]] void globeFailure(const char* what, double value = 0);
inline void require(bool holds, const char* what, double value = 0) {
    if (!holds) globeFailure(what, value);
}

// Puts pieces back into key order (a no-op when they are, which is how the history keeps them).
void sortPieces(std::vector<CrustPiece>& pieces);

// Compensated sum (Neumaier 1974) in the order given, so totals over pieces in key order do not
// depend on the thread count and their error does not grow with the number of pieces.
struct Sum {
    double total{}, carry{};
    void add(double value);
    double value() const { return total + carry; }
};
CrustStock totalStock(const std::vector<CrustPiece>& pieces);
double totalArea(const std::vector<CrustPiece>& pieces);

template <class T> SphereField<T> sphereField(int level, T value) {
    return SphereField<T>{level, std::vector<T>(size_t(6) << (2 * level), value)};
}

} // namespace derivean::sim
