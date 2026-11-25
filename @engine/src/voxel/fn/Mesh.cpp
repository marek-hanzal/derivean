#include "voxel/type/Mesh.hpp"

#include <algorithm>

namespace derivean {
namespace {

bool solid(const Voxels& voxels, Bounds slice) {
    for (int y = slice.min.y; y < slice.max.y; ++y)
        for (int z = slice.min.z; z < slice.max.z; ++z)
            for (int x = slice.min.x; x < slice.max.x; ++x)
                if (voxels[{x, y, z}] == Material::Air) return false;
    return true;
}

void clear(Voxels& voxels, Bounds box) {
    for (int y = box.min.y; y < box.max.y; ++y)
        for (int z = box.min.z; z < box.max.z; ++z)
            std::fill_n(voxels.cells.begin() + voxels.index({box.min.x, y, z}), box.size().x, Material::Air);
}

} // namespace

FaceMesh meshFaces(const Voxels& voxels, Bounds inner) {
    const Cell size = inner.size();
    FaceMesh result;
    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3;
        const int v = (axis + 2) % 3;
        const int width = size[u];
        const int height = size[v];
        std::vector<int16_t> mask(size_t(width) * height);
        for (int plane = 0; plane <= size[axis]; ++plane) {
            Cell point = inner.min;
            point[axis] += plane;
            Cell step{};
            step[axis] = 1;
            for (int j = 0; j < height; ++j) {
                point[v] = inner.min[v] + j;
                for (int i = 0; i < width; ++i) {
                    point[u] = inner.min[u] + i;
                    const Material before = voxels[point - step];
                    const Material after = voxels[point];
                    int16_t entry = 0;
                    if (plane > 0 && after == Material::Air) entry = int16_t(before);
                    if (plane < size[axis] && before == Material::Air) entry = -int16_t(after);
                    mask[size_t(j) * width + i] = entry;
                }
            }
            for (int j = 0; j < height; ++j) {
                for (int i = 0; i < width; ++i) {
                    const auto start = mask.begin() + size_t(j) * width + i;
                    const int16_t value = *start;
                    if (value == 0) continue;
                    int w = 1, h = 1;
                    while (i + w < width && start[w] == value) ++w;
                    while (j + h < height) {
                        const auto row = start + h * width;
                        if (!std::all_of(row, row + w, [value](auto cell) { return cell == value; })) break;
                        ++h;
                    }
                    for (int k = 0; k < h; ++k) std::fill_n(start + k * width, w, int16_t(0));
                    result.insert(result.end(), {
                        int16_t(axis), int16_t(value > 0 ? 1 : -1), int16_t(plane), int16_t(i), int16_t(j),
                        int16_t(w), int16_t(h), int16_t(value > 0 ? value : -value)
                    });
                    i += w - 1;
                }
            }
        }
    }
    return result;
}

std::vector<Bounds> mergeSolids(Voxels voxels) {
    const Bounds area = voxels.bounds;
    std::vector<Bounds> result;
    for (int y = area.min.y; y < area.max.y; ++y) {
        for (int z = area.min.z; z < area.max.z; ++z) {
            for (int x = area.min.x; x < area.max.x; ++x) {
                const Cell p{x, y, z};
                if (voxels[p] == Material::Air) continue;
                Bounds box{p, p + Cell{1, 1, 1}};
                for (int axis : {0, 2, 1}) {
                    while (box.max[axis] < area.max[axis]) {
                        Bounds next{box.min, box.max};
                        next.min[axis] = box.max[axis];
                        ++next.max[axis];
                        if (!solid(voxels, next)) break;
                        ++box.max[axis];
                    }
                }
                clear(voxels, box);
                result.push_back(box);
                x = box.max.x - 1;
            }
        }
    }
    return result;
}

} // namespace derivean
