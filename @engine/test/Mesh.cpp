// The binary mesher and collider boxes against a cell-by-cell walk, the oracle: the
// same voxels must give the same quads and boxes in the same order, over chunks of every kind of ground.
#include "voxel/type/Mesh.hpp"

#include "Expect.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>

using namespace derivean;

namespace {

int32_t walkedOcclusion(const Voxels& voxels, Cell front, int u, int v) {
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

bool walkedAlongU(int32_t entry) { const int32_t shade = std::abs(entry) >> 8; return (shade & 0x33) == ((shade >> 2) & 0x33); }
bool walkedAlongV(int32_t entry) { const int32_t shade = std::abs(entry) >> 8; return (shade & 0x0f) == (shade >> 4); }

FaceMesh walkedFaces(const Voxels& voxels, Bounds inner) {
    const Cell size = inner.size();
    FaceMesh result;
    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3, v = (axis + 2) % 3, width = size[u], height = size[v];
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
                    const Material before = voxels[point - step], after = voxels[point];
                    int32_t entry = 0;
                    if (plane > 0 && after == Material::Air && before != Material::Air) entry = int32_t(before) + (walkedOcclusion(voxels, point, u, v) << 8);
                    if (plane < size[axis] && before == Material::Air && after != Material::Air) entry = -(int32_t(after) + (walkedOcclusion(voxels, point - step, u, v) << 8));
                    mask[size_t(j) * width + i] = entry;
                    any |= entry != 0;
                }
            }
            if (!any) continue;
            mergeMask(mask, width, height, walkedAlongU, walkedAlongV, [&](int i, int j, int w, int h, int32_t entry) {
                const int32_t face = std::abs(entry);
                result.insert(result.end(), {int16_t(axis), int16_t(entry > 0 ? 1 : -1), int16_t(plane), int16_t(i), int16_t(j),
                                             int16_t(w), int16_t(h), int16_t(face & 0xff), int16_t(face >> 8)});
            });
        }
    }
    return result;
}

bool walkedSolid(const Voxels& voxels, Bounds slice) {
    for (int y = slice.min.y; y < slice.max.y; ++y)
        for (int z = slice.min.z; z < slice.max.z; ++z)
            for (int x = slice.min.x; x < slice.max.x; ++x)
                if (voxels[{x, y, z}] == Material::Air) return false;
    return true;
}

std::vector<Bounds> walkedSolids(Voxels voxels) {
    const Bounds area = voxels.bounds;
    std::vector<Bounds> result;
    for (int y = area.min.y; y < area.max.y; ++y)
        for (int z = area.min.z; z < area.max.z; ++z)
            for (int x = area.min.x; x < area.max.x; ++x) {
                const Cell p{x, y, z};
                if (voxels[p] == Material::Air) continue;
                Bounds box{p, p + Cell{1, 1, 1}};
                for (int axis : {0, 2, 1})
                    while (box.max[axis] < area.max[axis]) {
                        Bounds next{box.min, box.max};
                        next.min[axis] = box.max[axis];
                        ++next.max[axis];
                        if (!walkedSolid(voxels, next)) break;
                        ++box.max[axis];
                    }
                for (int yy = box.min.y; yy < box.max.y; ++yy)
                    for (int zz = box.min.z; zz < box.max.z; ++zz)
                        std::fill_n(voxels.cells.begin() + voxels.index({box.min.x, yy, zz}), box.size().x, Material::Air);
                result.push_back(box);
                x = box.max.x - 1;
            }
    return result;
}

constexpr Material Layers[] = {Material::Bedrock, Material::Shale, Material::Soil, Material::Sand, Material::Gravel};

// Ground the way chunks hold it: a surface of a given slope and roughness with layered material, and optionally
// holes (caves, digging) and loose cells (stones, debris). Off-lattice origins and sizes test the word edges.
Voxels ground(std::mt19937& random, Bounds inner, double slope, double rough, double holes, double loose, int pad = 1) {
    Voxels voxels(inner.padded(pad));
    std::uniform_real_distribution<double> unit(0, 1);
    const double angle = unit(random) * 6.2832, phase = unit(random) * 100, base = inner.min.y + inner.size().y * (0.2 + 0.6 * unit(random));
    const Bounds box = voxels.bounds;
    for (int z = box.min.z; z < box.max.z; ++z)
        for (int x = box.min.x; x < box.max.x; ++x) {
            const double along = x * std::cos(angle) + z * std::sin(angle);
            const double top = base + slope * (along - inner.min.x) * 0.25 + rough * (std::sin(x * 0.37 + phase) + std::cos(z * 0.23 - phase) * std::sin(x * 0.11));
            for (int y = box.min.y; y < box.max.y; ++y) {
                const double depth = top - y;
                Material material = depth < 0 ? Material::Air : Layers[std::min(4, int(depth / 3))];
                if (material != Material::Air && unit(random) < holes) material = Material::Air;
                if (material == Material::Air && depth > -3 && unit(random) < loose) material = Layers[random() % 5];
                voxels[{x, y, z}] = material;
            }
        }
    return voxels;
}

double since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

} // namespace

void testMesh() {
    std::mt19937 random(20261127);
    std::vector<std::pair<Voxels, Bounds>> chunks;
    std::vector<bool> clean;   // ground without holes or loose cells, as most terrain chunks are
    const Bounds chunk{{-32, 64, 96}, {0, 96, 128}};
    // Flat, gentle, stepped and steep ground, rough or smooth, whole, dug and strewn: 360 chunks.
    for (double slope : {0.0, 0.3, 1.0, 3.0})
        for (double rough : {0.0, 2.0, 6.0})
            for (double holes : {0.0, 0.02, 0.3})
                for (double loose : {0.0, 0.05})
                    for (int copy = 0; copy < 5; ++copy) {
                        chunks.emplace_back(ground(random, chunk, slope, rough, holes, loose), chunk);
                        clean.push_back(holes == 0 && loose == 0);
                    }
    // Sizes off the 64-cell word and wider than a word, and the plain cases.
    for (const Bounds odd : {Bounds{{3, -5, 7}, {66, 12, 20}}, Bounds{{0, 0, 0}, {1, 1, 1}}, Bounds{{-70, 2, -3}, {1, 9, 64}}, Bounds{{5, 5, 5}, {37, 100, 9}}})
        for (double holes : {0.0, 0.5}) chunks.emplace_back(ground(random, odd, 1.0, 3.0, holes, 0.1), odd);
    // Volumes wider than the padded chunk, as far cube chunks hand them, and dense noise.
    for (int pad : {2, 3})
        for (double holes : {0.0, 0.5, 0.9}) chunks.emplace_back(ground(random, chunk, 1.0, 3.0, holes, 0.3, pad), chunk);
    for (Material fill : {Material::Air, Material::Bedrock}) {
        Voxels plain(chunk.padded(1));
        std::fill(plain.cells.begin(), plain.cells.end(), fill);
        chunks.emplace_back(std::move(plain), chunk);
    }

    clean.resize(chunks.size(), false);
    size_t quads = 0, boxes = 0, cleanCount = 0;
    double walkedMs = 0, binaryMs = 0, walkedBoxMs = 0, binaryBoxMs = 0;
    double cleanMs[4]{};
    for (size_t at = 0; at < chunks.size(); ++at) {
        const auto& [voxels, inner] = chunks[at];
        auto start = std::chrono::steady_clock::now();
        const FaceMesh walked = walkedFaces(voxels, inner);
        walkedMs += since(start);
        start = std::chrono::steady_clock::now();
        const FaceMesh binary = meshFaces(voxels, inner);
        binaryMs += since(start);
        expect(walked == binary, "The binary mesher must make the walked mesher's quads, in its order");
        quads += binary.size() / FaceWords;
        start = std::chrono::steady_clock::now();
        const std::vector<Bounds> walkedBoxes = walkedSolids(voxels);
        walkedBoxMs += since(start);
        start = std::chrono::steady_clock::now();
        const std::vector<Bounds> binaryBoxes = mergeSolids(voxels);
        binaryBoxMs += since(start);
        expect(walkedBoxes.size() == binaryBoxes.size() && std::equal(walkedBoxes.begin(), walkedBoxes.end(), binaryBoxes.begin(),
                   [](Bounds a, Bounds b) { return a.min == b.min && a.max == b.max; }),
               "The binary collider must make the walked collider's boxes, in its order");
        boxes += binaryBoxes.size();
        if (!clean[at]) continue;
        ++cleanCount;
        // Timed again alone, so a clean chunk's share is not hidden among the noisy ones.
        for (int pass = 0; pass < 4; ++pass) {
            start = std::chrono::steady_clock::now();
            if (pass == 0) (void)walkedFaces(voxels, inner);
            else if (pass == 1) (void)meshFaces(voxels, inner);
            else if (pass == 2) (void)walkedSolids(voxels);
            else (void)mergeSolids(voxels);
            cleanMs[pass] += since(start);
        }
    }
    const double count = double(chunks.size());
    std::cout << "mesh: " << chunks.size() << " chunks, " << quads << " quads, " << boxes << " boxes agree; per chunk mesh "
              << walkedMs / count << " -> " << binaryMs / count << " ms, boxes " << walkedBoxMs / count << " -> " << binaryBoxMs / count << " ms; clean ground (" << cleanCount << ") mesh "
              << cleanMs[0] / double(cleanCount) << " -> " << cleanMs[1] / double(cleanCount) << " ms, boxes "
              << cleanMs[2] / double(cleanCount) << " -> " << cleanMs[3] / double(cleanCount) << " ms\n";
}
