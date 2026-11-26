#include "voxel/type/Mesh.hpp"

#include <algorithm>
#include <cstdlib>

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

// Occlusion of the four corners of a unit face that looks into `front`, a cell of air.
int32_t occlusion(const Voxels& voxels, Cell front, int u, int v) {
    int32_t packed = 0;
    for (int corner = 0; corner < 4; ++corner) {
        Cell alongU{}, alongV{};
        alongU[u] = corner & 1 ? 1 : -1;
        alongV[v] = corner & 2 ? 1 : -1;
        const bool first = voxels[front + alongU] != Material::Air, second = voxels[front + alongV] != Material::Air;
        const int solid = first && second ? 3 : int(first) + int(second) + int(voxels[front + alongU + alongV] != Material::Air);
        packed |= solid << (2 * corner);
    }
    return packed;
}

// A mask entry is material + occlusion · 256, negative for a face that looks down its axis.
// Corners shade linearly across a quad, so it may only grow along an axis its occlusion is constant on.
bool evenAlongU(int32_t entry) { const int32_t shade = std::abs(entry) >> 8; return (shade & 0x33) == ((shade >> 2) & 0x33); }
bool evenAlongV(int32_t entry) { const int32_t shade = std::abs(entry) >> 8; return (shade & 0x0f) == (shade >> 4); }

} // namespace

FaceMesh meshFaces(const Voxels& voxels, Bounds inner) {
    const Cell size = inner.size();
    FaceMesh result;
    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3;
        const int v = (axis + 2) % 3;
        const int width = size[u];
        const int height = size[v];
        std::vector<int32_t> mask(size_t(width) * height);
        for (int plane = 0; plane <= size[axis]; ++plane) {
            Cell point = inner.min;
            point[axis] += plane;
            Cell step{};
            step[axis] = 1;
            bool any = false;
            for (int j = 0; j < height; ++j) {
                point[v] = inner.min[v] + j;
                for (int i = 0; i < width; ++i) {
                    point[u] = inner.min[u] + i;
                    const Material before = voxels[point - step];
                    const Material after = voxels[point];
                    int32_t entry = 0;
                    if (plane > 0 && after == Material::Air && before != Material::Air) entry = int32_t(before) + (occlusion(voxels, point, u, v) << 8);
                    if (plane < size[axis] && before == Material::Air && after != Material::Air) entry = -(int32_t(after) + (occlusion(voxels, point - step, u, v) << 8));
                    mask[size_t(j) * width + i] = entry;
                    any |= entry != 0;
                }
            }
            if (!any) continue;
            mergeMask(mask, width, height, evenAlongU, evenAlongV, [&](int i, int j, int w, int h, int32_t entry) {
                const int32_t face = std::abs(entry);
                result.insert(result.end(), {int16_t(axis), int16_t(entry > 0 ? 1 : -1), int16_t(plane), int16_t(i), int16_t(j),
                                             int16_t(w), int16_t(h), int16_t(face & 0xff), int16_t(face >> 8)});
            });
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
