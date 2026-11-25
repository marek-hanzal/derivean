#pragma once

#include "voxel/constant/Materials.hpp"
#include "voxel/type/Grid.hpp"

#include <vector>

namespace derivean {

template<typename T>
struct Volume {
    Bounds bounds;
    Cell extent;
    std::vector<T> cells;

    explicit Volume(Bounds area)
        : bounds(area), extent(area.size()), cells(size_t(extent.x) * extent.y * extent.z) {}

    size_t index(Cell p) const {
        p = p - bounds.min;
        return (size_t(p.y) * extent.z + p.z) * extent.x + p.x;
    }

    T& operator[](Cell p) { return cells[index(p)]; }
    const T& operator[](Cell p) const { return cells[index(p)]; }
};

using Voxels = Volume<Material>;

} // namespace derivean
