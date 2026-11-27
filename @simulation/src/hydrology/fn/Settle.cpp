#include "hydrology/fn/Water.hpp"

namespace derivean::sim {

double levelOf(const BasinTree& tree, int32_t id, double own) {
    const Basin& basin = tree.basins[size_t(id)];
    if (own >= basin.capacity) return basin.spill;
    double level = basin.base, left = own / tree.area;
    int32_t wet = basin.below;
    for (int32_t i = 0; i < basin.count; ++i) {
        const double next = tree.height[size_t(tree.members[size_t(basin.first + i)])];
        const double need = wet * (next - level);
        if (wet > 0 && need >= left) break;
        left -= need;
        level = next;
        ++wet;
    }
    return wet > 0 ? level + left / wet : level;
}

double volumeOf(const BasinTree& tree, int32_t id, double level) {
    const Basin& basin = tree.basins[size_t(id)];
    double volume = basin.below * (level - basin.base);
    for (int32_t i = 0; i < basin.count; ++i) {
        const double height = tree.height[size_t(tree.members[size_t(basin.first + i)])];
        if (height >= level) break;
        volume += level - height;
    }
    return volume * tree.area;
}

// A basin with no room of its own between its saddle and its sill holds nothing that would show
// whether its children are full, so fullness is read from the leaves up. Then, parents first: a
// basin lies under its parent's surface when the parent holds water of its own, is itself drowned,
// or when both children are full and so stand level across their saddle.
void refreshLakes(const BasinTree& tree, Lakes& lakes) {
    const size_t count = tree.basins.size();
    lakes.level.assign(count, 0.0);
    lakes.lakeOf.assign(count, 0);
    lakes.full.assign(count, 0);
    lakes.ranked.clear();
    for (size_t id = 1; id < count; ++id) {
        const Basin& basin = tree.basins[id];
        lakes.full[id] = lakes.own[id] >= basin.capacity &&
                         (basin.child < 0 || (lakes.full[size_t(basin.child)] && lakes.full[size_t(tree.basins[size_t(basin.child)].sibling)]));
    }
    std::vector<uint8_t> drowned(count, 0);
    for (size_t id = count; id-- > 1;) {
        const Basin& basin = tree.basins[id];
        const size_t parent = size_t(std::max(basin.parent, 0));
        drowned[id] = parent > 0 && (drowned[parent] || lakes.own[parent] > 0 || (lakes.full[id] && lakes.full[size_t(basin.sibling)]));
        lakes.lakeOf[id] = drowned[id] ? lakes.lakeOf[parent] : int32_t(id);
        lakes.level[id] = drowned[id] ? lakes.level[parent] : levelOf(tree, int32_t(id), lakes.own[id]);
        if (!drowned[id]) lakes.ranked.push_back(int32_t(id));
    }
    // Highest first, but level alone does not order them: on level ground a full lake overflows into
    // another that stands exactly as high. Each lake is therefore placed only after every lake that
    // overflows into it.
    std::sort(lakes.ranked.begin(), lakes.ranked.end(), [&](int32_t a, int32_t b) {
        return lakes.level[size_t(a)] != lakes.level[size_t(b)] ? lakes.level[size_t(a)] > lakes.level[size_t(b)] : a < b;
    });
    const auto target = [&](int32_t top) {
        const Basin& basin = tree.basins[size_t(top)];
        return lakes.full[size_t(top)] && basin.over ? lakes.lakeOf[size_t(basin.over)] : 0;
    };
    std::vector<int32_t> waits(count, 0), byLevel;
    byLevel.swap(lakes.ranked);
    for (const int32_t top : byLevel) ++waits[size_t(target(top))];
    std::vector<uint8_t> placed(count, 0);
    for (int pass = 0; pass < 2; ++pass) {
        for (const int32_t seed : byLevel) {
            // The second pass only ever places lakes if overflows ran in a ring, which they cannot.
            for (int32_t at = seed; at && !placed[size_t(at)] && (pass || waits[size_t(at)] == 0);) {
                placed[size_t(at)] = 1;
                lakes.ranked.push_back(at);
                at = target(at);
                if (at) --waits[size_t(at)];
            }
        }
    }
}

namespace {

// Water poured into the tree. Fullness only grows while it is poured, so a basin found full is
// remembered, and `skip` shortens chains of full basins: a shortcut stays true.
struct Pouring {
    const BasinTree& tree;
    const std::vector<double>& own;
    std::vector<uint8_t> brim;
    std::vector<int32_t> skip;

    Pouring(const BasinTree& tree, const std::vector<double>& own)
        : tree(tree), own(own), brim(tree.basins.size(), 0), skip(tree.basins.size(), -1) {}

    bool full(int32_t id) {
        if (brim[size_t(id)]) return true;
        const Basin& basin = tree.basins[size_t(id)];
        if (own[size_t(id)] < basin.capacity) return false;
        if (basin.child >= 0 && own[size_t(id)] <= 0 && !(full(basin.child) && full(tree.basins[size_t(basin.child)].sibling))) return false;
        return brim[size_t(id)] = 1;
    }

    // Where the water of a full basin goes next: up into the parent once the sibling is full too,
    // otherwise across the saddle.
    int32_t next(int32_t id) {
        if (skip[size_t(id)] >= 0) return skip[size_t(id)];
        const Basin& basin = tree.basins[size_t(id)];
        return basin.parent > 0 && full(basin.sibling) ? basin.parent : basin.over;
    }

    // The first basin with room from `at` on; 0 when the water leaves the grid.
    int32_t room(int32_t at) {
        int32_t open = at;
        while (open != 0 && full(open)) open = next(open);
        while (at != open) {
            const int32_t following = next(at);
            skip[size_t(at)] = open;
            at = following;
        }
        return open;
    }

    // `store` is the vector `own` reads. Returns what left the grid.
    double pour(std::vector<double>& store, int32_t at, double volume) {
        while (volume > 0) {
            at = room(at);
            if (at == 0) return volume;
            const double space = tree.basins[size_t(at)].capacity - store[size_t(at)];
            if (volume < space) {
                store[size_t(at)] += volume;
                return 0.0;
            }
            store[size_t(at)] = tree.basins[size_t(at)].capacity;
            volume -= space;
        }
        return 0.0;
    }
};

} // namespace

int32_t pourTarget(const BasinTree& tree, const std::vector<double>& own, int32_t leaf) {
    Pouring pouring(tree, own);
    return pouring.room(leaf);
}

double pour(const BasinTree& tree, std::vector<double>& own, int32_t leaf, double volume) {
    Pouring pouring(tree, own);
    return pouring.pour(own, leaf, volume);
}

Lakes settle(WaterGrid& grid, const BasinTree& tree, std::vector<Tally> parcels, WaterFlows& flows) {
    const size_t count = tree.basins.size(), cells = grid.cells();
    const double area = tree.area;
    parcels.resize(count);
    for (size_t cell = 0; cell < cells; ++cell) {
        if (grid.depth[cell] <= 0 || grid.kindOf(int32_t(cell)) != CellKind::Open) continue;
        Tally& parcel = parcels[size_t(tree.leafOf[cell])];
        parcel.water += grid.depth[cell] * area;
        parcel.salt += grid.salt[cell] * area;
        grid.salt[cell] = 0;   // what is left on a cell from here on is crust
    }

    Lakes lakes;
    lakes.own.assign(count, 0.0);
    Pouring pouring(tree, lakes.own);
    double across = parcels[0].water;   // what leaves across the rim
    for (size_t leaf = 1; leaf < count; ++leaf) across += pouring.pour(lakes.own, int32_t(leaf), parcels[leaf].water);
    flows.outflow.water += across;
    flows.outflow.salt += parcels[0].salt;

    // Standing water of the rim tops up whatever lies open to it, at that water's own strength.
    std::vector<int32_t> feeder(count, -1);
    for (size_t id = count; id-- > 1;) {
        const Basin& basin = tree.basins[id];
        feeder[id] = basin.feeder >= 0 ? basin.feeder : basin.parent > 0 ? feeder[size_t(basin.parent)] : -1;
        if (feeder[id] < 0 || lakes.own[id] >= basin.capacity) continue;
        const double drawn = basin.capacity - lakes.own[id], salt = drawn * grid.salinity[size_t(feeder[id])];
        lakes.own[id] = basin.capacity;
        parcels[id].water += drawn;
        parcels[id].salt += salt;
        flows.fromBody.water += drawn;
        flows.fromBody.salt += salt;
    }
    refreshLakes(tree, lakes);

    // Per lake: what arrived in it, what it holds now, and so what passed through it.
    std::vector<Tally> arrived(count), stands(count);
    std::vector<int32_t> anchor(count, -1);
    for (size_t id = 1; id < count; ++id) {
        Tally& lake = arrived[size_t(lakes.lakeOf[id])];
        lake.water += parcels[id].water;
        lake.salt += parcels[id].salt;
    }
    for (const int32_t cell : tree.order) {
        if (grid.kindOf(cell) != CellKind::Open) continue;
        const int32_t basin = tree.basinOf[size_t(cell)];
        const double depth = basin ? std::max(0.0, lakes.level[size_t(basin)] - double(tree.height[size_t(cell)])) : 0.0;
        grid.depth[size_t(cell)] = depth;
        if (!basin) continue;
        const size_t lake = size_t(lakes.lakeOf[size_t(basin)]);
        if (anchor[lake] < 0) anchor[lake] = cell;
        if (depth <= 0) continue;
        stands[lake].water += depth * area;
        arrived[lake].salt += grid.salt[size_t(cell)] * area;   // crust the water now covers dissolves
        grid.salt[size_t(cell)] = 0;
    }
    // Highest lake first, so each has received all it is going to before it is mixed and passed on.
    for (const int32_t top : lakes.ranked) {
        const size_t lake = size_t(top);
        const Basin& basin = tree.basins[lake];
        const double through = lakes.full[lake] ? std::max(0.0, arrived[lake].water - stands[lake].water) : 0.0;
        const double whole = stands[lake].water + through;
        if (whole <= 0) {
            if (arrived[lake].salt > 0 && anchor[lake] >= 0) grid.salt[size_t(anchor[lake])] += arrived[lake].salt / area;
            continue;
        }
        const double strength = arrived[lake].salt / whole;
        stands[lake].salt = strength;
        if (through <= 0) continue;
        const size_t target = basin.over ? size_t(lakes.lakeOf[size_t(basin.over)]) : 0;
        if (target == 0) {
            flows.outflow.salt += through * strength;
        } else {
            arrived[target].water += through;
            arrived[target].salt += through * strength;
        }
    }
    for (size_t cell = 0; cell < cells; ++cell) {
        if (grid.depth[cell] <= 0) continue;
        grid.salt[cell] += stands[size_t(lakes.lakeOf[size_t(tree.basinOf[cell])])].salt * grid.depth[cell];
    }
    return lakes;
}

} // namespace derivean::sim
