#pragma once

#include "atmosphere/type/SphereMatrix.hpp"
#include "atmosphere/type/StencilMatrix.hpp"

#include <vector>

namespace derivean::sim {

// Splits an assembled Laplacian on a grid with `side` cells along a face edge. Should an interior row
// not have the 21 expected columns, every row stays a compressed row.
StencilMatrix makeStencilMatrix(const SphereMatrix& matrix, int side);

// result = K · field, the same bit for bit as applyMatrix on the SphereMatrix it came from.
void applyMatrix(const StencilMatrix& matrix, const std::vector<double>& field, std::vector<double>& result);

} // namespace derivean::sim
