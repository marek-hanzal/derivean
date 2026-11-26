#pragma once

#include <vector>

namespace derivean::sim {

// A sparse symmetric matrix in rows (compressed sparse rows), the cells' Laplacian stiffness.
struct SphereMatrix {
    std::vector<int> start;       // row r holds entries start[r] .. start[r + 1] − 1
    std::vector<int> column;
    std::vector<double> value;
    std::vector<double> diagonal;
};

} // namespace derivean::sim
