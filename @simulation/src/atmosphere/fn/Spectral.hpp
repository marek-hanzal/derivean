#pragma once

#include "core/type/Field.hpp"

#include <vector>

namespace derivean::sim {

// Tables and work space for inverting the five-point Laplacian on a periodic square grid whose
// side is a power of two. The work space is overwritten by every solve and carries no state.
struct PoissonSolver {
    int size{};
    std::vector<int> reversed;          // bit-reversed index
    std::vector<double> cosine, sine;   // roots of unity over half a turn
    std::vector<double> inverse;        // per wave (z, x ≤ size / 2): 1 / (eigenvalue · size²); 0 for the mean
    std::vector<double> pass;           // per wave: share of the source the caller wants back as `band`
    std::vector<double> spectrum;       // size rows of size / 2 + 1 complex numbers
    std::vector<double> bandSpectrum;
    std::vector<double> line;           // one row of complex numbers
};

// `pass` starts as all zero; the caller shapes it. Wave (z, x) has z cycles along Z, counted
// 0 … size − 1 with the upper half standing for negative cycles, and x ≤ size / 2 cycles along X.
PoissonSolver makePoissonSolver(int size, double cell);

// Solves laplacian(result) = source for the zero-mean result; the mean of the source is dropped.
// With `band`, also returns the source filtered wave by wave through `pass`.
void solvePoisson(PoissonSolver& solver, const Field<double>& source, Field<double>& result, Field<double>* band = nullptr);

} // namespace derivean::sim
