#include "volcanic/fn/Emplace.hpp"

#include "landscape/type/Terrain.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr float Least = 1e-3f;       // m of mobile layer below which a cell has nothing to pass on
constexpr float Tolerance = 0.02f;   // share of a flow unit by which a surface may stand above its rest

// Lava keeps running while its core is hot and stiffens fast once the crust has grown through it:
// the yield strength stays near its hot value for most of the way and reaches the cold one at the end.
float yieldAt(const Rheology& rheology, float heat) {
    const float spent = 1.0f - std::clamp(heat, 0.0f, 1.0f);
    return rheology.yieldHot * std::pow(rheology.yieldCold / rheology.yieldHot, spent * spent * spent);
}

} // namespace

void wake(Emplacement& field, Worklist& work, size_t cell) {
    if (field.queued[cell]) return;
    field.queued[cell] = 1;
    work.heap.push_back({field.ground[cell] + field.mobile[cell], int32_t(cell)});
    std::push_heap(work.heap.begin(), work.heap.end());
}

void feed(Emplacement& field, Worklist& work, size_t cell, float thickness) {
    if (field.mobile[cell] <= 0) work.touched.push_back(int32_t(cell));
    field.heat[cell] = (field.heat[cell] * field.mobile[cell] + thickness) / (field.mobile[cell] + thickness);
    field.mobile[cell] += thickness;
    wake(field, work, cell);
}

long relax(Emplacement& field, const Rheology& rheology, Worklist& work, long visits) {
    const int width = field.width;
    const float cell = float(field.cell), area = cell * cell, slack = std::max(Least, Tolerance * rheology.sheet);
    long used = 0;
    while (!work.heap.empty() && used < visits) {
        std::pop_heap(work.heap.begin(), work.heap.end());
        const size_t i = size_t(work.heap.back().second);
        work.heap.pop_back();
        field.queued[i] = 0;
        const float layer = field.mobile[i];
        if (layer < Least) continue;
        ++used;
        const int x = int(i % size_t(width)), z = int(i / size_t(width));
        const float top = field.ground[i] + layer;
        // The fall over one cell's width that the layer just holds.
        const float holds = yieldAt(rheology, field.heat[i]) / std::min(layer, rheology.sheet) * cell;
        // For each neighbour the level this cell would have to sink to before it stopped feeding it.
        float level[8];
        int order[8], count = 0;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (!field.contains(nx, nz)) continue;
            const size_t j = field.index(nx, nz);
            level[k] = field.ground[j] + field.mobile[j] + holds * NeighborReach[k];
            if (level[k] >= top - slack) continue;
            int at = count++;
            for (; at > 0 && level[order[at - 1]] > level[k]; --at) order[at] = order[at - 1];
            order[at] = k;
        }
        if (count == 0) continue;
        // The cell and the neighbours below it come to one level, each neighbour by its own
        // threshold lower: the lowest takes most, as a flow keeps to the fall line. If the layer
        // does not last that far, all of it goes and the neighbours share it the same way.
        float sum = top, common = top;
        int taking = 0;
        while (taking < count && level[order[taking]] < common) sum += level[order[taking++]], common = sum / float(taking + 1);
        if (common < field.ground[i]) {
            sum = layer, taking = 0;
            common = 1e30f;
            while (taking < count && level[order[taking]] < common) sum += level[order[taking++]], common = sum / float(taking);
        }
        float sent = 0;
        for (int n = 0; n < taking; ++n) {
            const int k = order[n];
            const float part = std::min(common - level[k], layer - sent);
            if (part <= 0) continue;
            const size_t j = field.index(x + NeighborX[k], z + NeighborZ[k]);
            sent += part;
            if (field.rest[j] == Rest::Closed) {
                field.lost += double(part) * area;
                continue;
            }
            const float way = NeighborReach[k] * cell / (field.submerged(j) ? rheology.coolLength * QuenchShare : rheology.coolLength);
            const float heat = field.heat[i] - way;
            if (heat <= 0) {
                // Spent: it sets as it arrives.
                if (field.rest[j] == Rest::Open) field.laid[j] += part, field.ground[j] += part;
                else field.passed += double(part) * area;
                continue;
            }
            if (field.mobile[j] <= 0) work.touched.push_back(int32_t(j));
            field.heat[j] = (field.heat[j] * field.mobile[j] + heat * part) / (field.mobile[j] + part);
            field.mobile[j] += part;
            wake(field, work, j);
        }
        field.mobile[i] = layer - sent;
        // A cell that has sunk may release what rested against it.
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (field.contains(nx, nz) && field.mobile[field.index(nx, nz)] >= Least) wake(field, work, field.index(nx, nz));
        }
    }
    return used;
}

void set(Emplacement& field, Worklist& work) {
    const double area = field.cell * field.cell;
    for (const int32_t at : work.touched) {
        const size_t i = size_t(at);
        const float layer = field.mobile[i];
        if (layer <= 0) continue;
        if (field.rest[i] == Rest::Open) field.laid[i] += layer, field.ground[i] += layer;
        else if (field.rest[i] == Rest::Passing) field.passed += double(layer) * area;
        else field.lost += double(layer) * area;
        field.mobile[i] = field.heat[i] = 0;
    }
    for (const auto& entry : work.heap) field.queued[size_t(entry.second)] = 0;
    work.heap.clear();
    work.touched.clear();
}

} // namespace derivean::sim
