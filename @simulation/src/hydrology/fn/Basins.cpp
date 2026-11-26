#include "hydrology/fn/Water.hpp"

#include <array>
#include <cstring>
#include <limits>

namespace derivean::sim {
namespace {

// Cells by rising (height, index): a stable radix sort on the bits of the heights.
std::vector<int32_t> risingOrder(const WaterGrid& grid, const std::vector<float>& height) {
    std::vector<int32_t> order;
    order.reserve(height.size());
    for (size_t i = 0; i < height.size(); ++i)
        if (grid.kindOf(int32_t(i)) != CellKind::Outside) order.push_back(int32_t(i));
    std::vector<int32_t> work(order.size());
    const auto key = [&](int32_t cell) {
        uint32_t bits;
        std::memcpy(&bits, &height[size_t(cell)], sizeof(bits));
        return bits ^ ((bits >> 31) ? 0xffffffffu : 0x80000000u);
    };
    for (int shift = 0; shift < 32; shift += 11) {
        std::array<uint32_t, 2049> start{};
        for (const int32_t cell : order) ++start[((key(cell) >> shift) & 2047u) + 1];
        for (size_t digit = 1; digit < start.size(); ++digit) start[digit] += start[digit - 1];
        for (const int32_t cell : order) work[start[(key(cell) >> shift) & 2047u]++] = cell;
        order.swap(work);
    }
    return order;
}

} // namespace

// Cells are visited from the lowest up while a union-find keeps the connected pieces of the ground
// already visited. A cell with no lower neighbour starts a leaf; a cell that touches two pieces is
// the saddle where they close and a parent opens above them; a piece that reaches the rim closes
// against the outside. Each closing basin records where its overflow goes: across the saddle and
// down the far side.
BasinTree buildBasins(const WaterGrid& grid) {
    const size_t cells = grid.cells();
    constexpr double Endless = std::numeric_limits<double>::infinity();
    BasinTree tree;
    tree.area = grid.cell * grid.cell;
    tree.height.resize(cells);
    for (size_t i = 0; i < cells; ++i) tree.height[i] = grid.kindOf(int32_t(i)) == CellKind::Rim ? grid.held[i] : grid.ground[i];
    tree.order = risingOrder(grid, tree.height);
    std::vector<int32_t> rank(cells, std::numeric_limits<int32_t>::max());
    for (size_t i = 0; i < tree.order.size(); ++i) rank[size_t(tree.order[i])] = int32_t(i);
    tree.basinOf.assign(cells, 0);
    tree.receiver.assign(cells, -1);
    tree.leafOf.assign(cells, 0);
    tree.basins.emplace_back();

    std::vector<int32_t> top{0};     // union-find: the open basin each closed one now lies in
    std::vector<int32_t> size{0};    // cells of the whole piece
    std::vector<double> rise{0.0};   // m, summed height of a basin's own cells above its base
    const auto find = [&](int32_t basin) {
        while (top[size_t(basin)] != basin) {
            top[size_t(basin)] = top[size_t(top[size_t(basin)])];
            basin = top[size_t(basin)];
        }
        return basin;
    };
    const auto open = [&](double base, int32_t below) {
        Basin basin;
        basin.base = base;
        basin.below = below;
        basin.spill = basin.capacity = Endless;
        tree.basins.push_back(basin);
        top.push_back(int32_t(top.size()));
        size.push_back(below);
        rise.push_back(0.0);
        return int32_t(tree.basins.size() - 1);
    };
    const auto join = [&](int32_t cell, int32_t id) {
        tree.basinOf[size_t(cell)] = id;
        ++tree.basins[size_t(id)].count;
        ++size[size_t(id)];
        rise[size_t(id)] += double(tree.height[size_t(cell)]) - tree.basins[size_t(id)].base;
    };
    const auto close = [&](int32_t id, int32_t cell, double spill, int32_t parent, int32_t sibling, int32_t entry, int32_t feeder) {
        Basin& basin = tree.basins[size_t(id)];
        basin.spill = spill;
        basin.parent = parent;
        basin.sibling = sibling;
        basin.entry = entry;
        basin.feeder = feeder;
        basin.saddle = cell;
        basin.over = entry < 0 || grid.kindOf(entry) == CellKind::Rim ? 0 : tree.leafOf[size_t(entry)];
        basin.capacity = (double(basin.below + basin.count) * (spill - basin.base) - rise[size_t(id)]) * tree.area;
        top[size_t(id)] = parent;
    };

    int32_t offset[8];
    for (int k = 0; k < 8; ++k) offset[k] = NeighborZ[k] * grid.width + NeighborX[k];
    for (const int32_t cell : tree.order) {
        const float here = tree.height[size_t(cell)];
        const bool rim = grid.kindOf(cell) == CellKind::Rim;
        const int x = cell % grid.width, z = cell / grid.width;
        const bool inner = x > 0 && z > 0 && x + 1 < grid.width && z + 1 < grid.height;
        int32_t roots[8], gates[8];   // the pieces this cell touches and the lowest cell it touches each by
        int found = 0;
        int32_t receiver = -1;
        float steepest = -1;
        for (int k = 0; k < 8; ++k) {
            const int32_t other = inner ? cell + offset[k] : grid.neighbour(cell, k);
            if (other < 0 || rank[size_t(other)] > rank[size_t(cell)]) continue;
            const bool otherRim = grid.kindOf(other) == CellKind::Rim;
            if (rim && otherRim) continue;
            const float slope = (here - tree.height[size_t(other)]) / NeighborReach[k];
            if (slope > steepest) {
                steepest = slope;
                receiver = other;
            }
            const int32_t root = otherRim ? 0 : find(tree.basinOf[size_t(other)]);
            int at = 0;
            while (at < found && roots[at] != root) ++at;
            if (at == found) {
                roots[found] = root;
                gates[found++] = other;
            } else if (rank[size_t(other)] < rank[size_t(gates[at])]) {
                gates[at] = other;
            }
        }
        if (rim) {
            const int32_t feeder = grid.held[size_t(cell)] > grid.ground[size_t(cell)] ? cell : -1;
            for (int i = 0; i < found; ++i)
                if (roots[i] != 0) close(roots[i], cell, here, 0, -1, -1, feeder);
            continue;
        }
        tree.receiver[size_t(cell)] = receiver;
        if (found == 0) {
            const int32_t leaf = open(here, 0);
            join(cell, leaf);
            tree.leafOf[size_t(cell)] = leaf;
            continue;
        }
        tree.leafOf[size_t(cell)] = grid.kindOf(receiver) == CellKind::Rim ? 0 : tree.leafOf[size_t(receiver)];
        int outside = 0;
        while (outside < found && roots[outside] != 0) ++outside;
        if (outside < found) {
            for (int i = 0; i < found; ++i)
                if (roots[i] != 0) close(roots[i], cell, here, 0, -1, gates[outside], -1);
            continue;
        }
        int32_t merged = roots[0], gate = gates[0];
        for (int i = 1; i < found; ++i) {
            const int32_t parent = open(here, size[size_t(merged)] + size[size_t(roots[i])]);
            tree.basins[size_t(parent)].child = merged;
            close(merged, cell, here, parent, roots[i], gates[i], -1);
            close(roots[i], cell, here, parent, merged, gate, -1);
            if (rank[size_t(gates[i])] < rank[size_t(gate)]) gate = gates[i];
            merged = parent;
        }
        join(cell, merged);
    }

    int32_t next = 0;
    for (Basin& basin : tree.basins) {
        basin.first = next;
        next += basin.count;
        basin.count = 0;
    }
    tree.members.resize(size_t(next));
    for (const int32_t cell : tree.order) {
        if (grid.kindOf(cell) != CellKind::Open || tree.basinOf[size_t(cell)] == 0) continue;
        Basin& basin = tree.basins[size_t(tree.basinOf[size_t(cell)])];
        tree.members[size_t(basin.first + basin.count++)] = cell;
    }
    return tree;
}

} // namespace derivean::sim
