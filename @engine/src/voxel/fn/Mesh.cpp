#include "voxel/type/Mesh.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

// Both meshers read solid cells as rows of bits, 64 cells a word, as binary greedy meshing does (cgerikj, Binary
// Greedy Meshing; docs/credits/rendering.md): a face is a solid bit whose neighbour bit on the next layer is clear,
// and a row of cells is tested or cleared a word at a time. They make the same faces and boxes in the same order as
// a cell-by-cell walk: the rectangles are still cut from the same mask by mergeMask (Lysenko 2012), and the boxes
// grow by the same rule. engine-tests' testMesh keeps the cell-by-cell walk and checks both against it.

namespace derivean {
namespace {

// The solid cells of a box, one bit a cell along `u`, a row of words for each cell along `v`, a layer of rows for
// each cell along the remaining axis; all counted from the box's minimum corner.
struct BitRows {
    int words{};
    int rows{};
    std::vector<uint64_t> bits;
};

BitRows bitRows(int layers, int rowsPerLayer, int cells) {
    const int words = (cells + 63) / 64;
    return {words, rowsPerLayer, std::vector<uint64_t>(size_t(layers) * size_t(rowsPerLayer) * size_t(words))};
}

size_t rowStart(const BitRows& rows, int layer, int at) { return (size_t(layer) * size_t(rows.rows) + size_t(at)) * size_t(rows.words); }
uint64_t* row(BitRows& rows, int layer, int at) { return rows.bits.data() + rowStart(rows, layer, at); }
const uint64_t* row(const BitRows& rows, int layer, int at) { return rows.bits.data() + rowStart(rows, layer, at); }

bool test(const uint64_t* row, int cell) { return (row[cell >> 6] >> (cell & 63)) & 1; }

// Bits [from, to) of word `word` in a row: all ones in that range.
uint64_t span(int word, int from, int to) {
    const int low = std::max(from - word * 64, 0), high = std::min(to - word * 64, 64);
    if (low >= high) return 0;
    const uint64_t below = high == 64 ? ~uint64_t(0) : (uint64_t(1) << high) - 1;
    return below & ~((uint64_t(1) << low) - 1);
}

bool allSet(const uint64_t* row, int from, int to) {
    for (int word = from >> 6; word <= (to - 1) >> 6; ++word)
        if (const uint64_t wanted = span(word, from, to); (row[word] & wanted) != wanted) return false;
    return true;
}

void clearSpan(uint64_t* row, int from, int to) {
    for (int word = from >> 6; word <= (to - 1) >> 6; ++word) row[word] &= ~span(word, from, to);
}

// The first set bit at or after `cell`, or `cells` when there is none.
int nextSet(const uint64_t* row, int cell, int cells) {
    for (int word = cell >> 6; word * 64 < cells; ++word) {
        const uint64_t left = row[word] & span(word, cell, cells);
        if (left) return word * 64 + std::countr_zero(left);
    }
    return cells;
}

// The first clear bit at or after `cell`, or `cells` when there is none.
int nextClear(const uint64_t* row, int cell, int cells) {
    for (int word = cell >> 6; word * 64 < cells; ++word) {
        const uint64_t left = ~row[word] & span(word, cell, cells);
        if (left) return word * 64 + std::countr_zero(left);
    }
    return cells;
}

// Solid cells of `box` along x alone: layers along z, rows along y.
BitRows rowsAlongX(const Voxels& voxels, Bounds box) {
    const Cell size = box.size();
    BitRows rows = bitRows(size.z, size.y, size.x);
    for (int y = 0; y < size.y; ++y)
        for (int z = 0; z < size.z; ++z) {
            const Material* cell = &voxels[box.min + Cell{0, y, z}];
            uint64_t* bits = row(rows, z, y);
            for (int x = 0; x < size.x; ++x) bits[x >> 6] |= uint64_t(cell[x] != Material::Air) << (x & 63);
        }
    return rows;
}

// Solid cells of `box` seen along each axis a: layers along a, rows along v = (a + 2) % 3, bits along u = (a + 1) % 3.
std::array<BitRows, 3> solidRows(const Voxels& voxels, Bounds box) {
    const Cell size = box.size();
    std::array<BitRows, 3> rows{bitRows(size.x, size.z, size.y), bitRows(size.y, size.x, size.z), bitRows(size.z, size.y, size.x)};
    for (int y = 0; y < size.y; ++y)
        for (int z = 0; z < size.z; ++z) {
            const Material* cell = &voxels[box.min + Cell{0, y, z}];
            uint64_t* alongX = row(rows[2], z, y);
            uint64_t bits = 0;
            for (int x = 0; x < size.x; ++x) {
                if (cell[x] != Material::Air) {
                    bits |= uint64_t(1) << (x & 63);
                    row(rows[0], x, z)[y >> 6] |= uint64_t(1) << (y & 63);
                    row(rows[1], y, x)[z >> 6] |= uint64_t(1) << (z & 63);
                }
                if ((x & 63) == 63 || x == size.x - 1) {
                    alongX[x >> 6] = bits;
                    bits = 0;
                }
            }
        }
    return rows;
}

// Occlusion of the four corners of a face from the eight cells round the air cell it looks into. The key holds
// three bits from each of the rows before, at and after that cell: bit 0 the cell before along u, 1 the cell
// itself (never read), 2 the one after. A corner counts the two edge cells and the diagonal it touches, 3 when both edges are solid.
constexpr std::array<uint8_t, 512> OcclusionOfKey = [] {
    std::array<uint8_t, 512> table{};
    for (int key = 0; key < 512; ++key) {
        const auto bit = [&](int rowAlongV, int alongU) { return (key >> (3 * rowAlongV + alongU)) & 1; };
        int packed = 0;
        for (int corner = 0; corner < 4; ++corner) {
            const int alongU = corner & 1 ? 2 : 0, alongV = corner & 2 ? 2 : 0;
            const int first = bit(1, alongU), second = bit(alongV, 1);
            packed |= (first && second ? 3 : first + second + bit(alongV, alongU)) << (2 * corner);
        }
        table[size_t(key)] = uint8_t(packed);
    }
    return table;
}();

// Bits cell - 1 to cell + 1 of a row.
uint32_t around(const uint64_t* row, int cell) {
    const int low = cell - 1;
    if ((low & 63) <= 61) return uint32_t(row[low >> 6] >> (low & 63)) & 7;
    return uint32_t(test(row, low)) | uint32_t(test(row, cell)) << 1 | uint32_t(test(row, cell + 1)) << 2;
}

// Occlusion of the four corners of a unit face that looks into the air cell at bit `cell` of row `at` in `layer`.
int32_t occlusion(const BitRows& rows, int layer, int at, int cell) {
    const uint32_t key = around(row(rows, layer, at - 1), cell) | around(row(rows, layer, at), cell) << 3 | around(row(rows, layer, at + 1), cell) << 6;
    return OcclusionOfKey[key];
}

// A mask entry is material + occlusion · 256, negative for a face that looks down its axis.
// Corners shade linearly across a quad, so it may only grow along an axis its occlusion is constant on.
bool evenAlongU(int32_t entry) { const int32_t shade = std::abs(entry) >> 8; return (shade & 0x33) == ((shade >> 2) & 0x33); }
bool evenAlongV(int32_t entry) { const int32_t shade = std::abs(entry) >> 8; return (shade & 0x0f) == (shade >> 4); }

} // namespace

FaceMesh meshFaces(const Voxels& voxels, Bounds inner) {
    const Bounds box = inner.padded(1);
    const std::array<BitRows, 3> solid = solidRows(voxels, box);
    const Cell size = inner.size();
    FaceMesh result;
    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3;
        const int v = (axis + 2) % 3;
        const int width = size[u];
        const int height = size[v];
        const BitRows& rows = solid[axis];
        std::vector<int32_t> mask(size_t(width) * height);
        // A step of one cell along the axis and along u, in the voxels' index.
        Cell step{}, alongU{};
        step[axis] = 1;
        alongU[u] = 1;
        const ptrdiff_t stepAt = ptrdiff_t(voxels.index(inner.min + step)) - ptrdiff_t(voxels.index(inner.min));
        const ptrdiff_t stepU = ptrdiff_t(voxels.index(inner.min + alongU)) - ptrdiff_t(voxels.index(inner.min));
        // Layer `plane` of the padded box holds the cells before the plane, `plane + 1` those after it; bit and row
        // k of the padded box are cell k - 1 of the inner one.
        for (int plane = 0; plane <= size[axis]; ++plane) {
            bool any = false;
            for (int j = 0; j < height; ++j) {
                const uint64_t* before = row(rows, plane, j + 1);
                const uint64_t* after = row(rows, plane + 1, j + 1);
                Cell start = inner.min;
                start[axis] += plane;
                start[v] += j;
                const Material* cells = voxels.cells.data() + voxels.index(start);
                for (int word = 0; word < rows.words; ++word) {
                    const uint64_t inside = span(word, 1, width + 1);
                    const uint64_t up = plane > 0 ? before[word] & ~after[word] & inside : 0;
                    const uint64_t down = plane < size[axis] ? after[word] & ~before[word] & inside : 0;
                    for (uint64_t faces = up | down; faces; faces &= faces - 1) {
                        const int bit = std::countr_zero(faces);
                        const int i = word * 64 + bit - 1;
                        const Material* point = cells + i * stepU;
                        int32_t entry;
                        if ((up >> bit) & 1) entry = int32_t(point[-stepAt]) + (occlusion(rows, plane + 1, j + 1, i + 1) << 8);
                        else entry = -(int32_t(point[0]) + (occlusion(rows, plane, j + 1, i + 1) << 8));
                        mask[size_t(j) * width + i] = entry;
                        any = true;
                    }
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

// Boxes grow from the lowest untaken solid cell along x, then z, then y, as far as every cell they would take is
// solid and not yet taken; taken cells are cleared from the bits.
std::vector<Bounds> mergeSolids(const Voxels& voxels) {
    const Bounds area = voxels.bounds;
    const Cell size = area.size();
    BitRows open = rowsAlongX(voxels, area);
    std::vector<Bounds> result;
    for (int y = 0; y < size.y; ++y) {
        for (int z = 0; z < size.z; ++z) {
            for (int x = nextSet(row(open, z, y), 0, size.x); x < size.x; x = nextSet(row(open, z, y), x, size.x)) {
                Bounds box{{x, y, z}, {nextClear(row(open, z, y), x, size.x), y + 1, z + 1}};
                while (box.max.z < size.z && allSet(row(open, box.max.z, y), box.min.x, box.max.x)) ++box.max.z;
                const auto layerFree = [&](int layer) {
                    for (int at = box.min.z; at < box.max.z; ++at)
                        if (!allSet(row(open, at, layer), box.min.x, box.max.x)) return false;
                    return true;
                };
                while (box.max.y < size.y && layerFree(box.max.y)) ++box.max.y;
                for (int layer = box.min.y; layer < box.max.y; ++layer)
                    for (int at = box.min.z; at < box.max.z; ++at) clearSpan(row(open, at, layer), box.min.x, box.max.x);
                result.push_back({area.min + box.min, area.min + box.max});
                x = box.max.x;
            }
        }
    }
    return result;
}

} // namespace derivean
