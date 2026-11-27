#pragma once

#include "voxel/type/Volume.hpp"

#include <algorithm>

namespace derivean {

// Nine shorts per quad: axis, sign, plane, u, v, width, height, material, occlusion. Coordinates are
// in cells relative to the minimum corner of the meshed bounds; u and v run along the two axes
// after `axis`, in cyclic order. Occlusion holds two bits per corner, (u, v) low-low, high-low,
// low-high, high-high from the lowest bits up: how many of the three cells that touch the corner
// in the layer the face looks into are solid, 3 where both of its edge neighbours are.
// A quad covers cells of one material only, and of one occlusion wherever stretching it would
// change how the corners shade the cells between them.
inline constexpr int FaceWords = 9;
using FaceMesh = std::vector<int16_t>;

// `voxels` must hold every cell of `inner` padded by one, edges and corners included.
FaceMesh meshFaces(const Voxels& voxels, Bounds inner);

// Cuts a mask of `width` × `height` entries into rectangles of equal non-zero entries, clearing it, row by row
// as greedy meshing does (Lysenko 2012, docs/credits/rendering.md), and hands each to
// `emit(u, v, width, height, entry)`. `alongU` and `alongV` say whether a rectangle of that entry may grow along
// the axis.
template<typename AlongU, typename AlongV, typename Emit>
void mergeMask(std::vector<int32_t>& mask, int width, int height, AlongU alongU, AlongV alongV, Emit emit) {
    for (int j = 0; j < height; ++j)
        for (int i = 0; i < width; ++i) {
            const auto start = mask.begin() + size_t(j) * width + i;
            const int32_t value = *start;
            if (value == 0) continue;
            int w = 1, h = 1;
            if (alongU(value))
                while (i + w < width && start[w] == value) ++w;
            if (alongV(value))
                while (j + h < height && std::all_of(start + h * width, start + h * width + w, [value](int32_t cell) { return cell == value; })) ++h;
            for (int k = 0; k < h; ++k) std::fill_n(start + k * width, w, 0);
            emit(i, j, w, h, value);
            i += w - 1;
        }
}

// Solid cells of `voxels` as boxes, for colliders: each grows from the lowest cell not yet taken along x, then z,
// then y, as far as every cell it would take is solid and not taken.
std::vector<Bounds> mergeSolids(const Voxels& voxels);

} // namespace derivean
