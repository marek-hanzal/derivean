#include "core/fn/Hash.hpp"
#include "groundwater/fn/Caves.hpp"
#include "groundwater/fn/Karst.hpp"
#include "groundwater/fn/Stages.hpp"
#include "groundwater/fn/Vents.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t CaveStream = 0x43415645;
constexpr float Breakthrough = 4000.0f;   // yr a fissure a kilometre long takes to open into a passage; grows with the square of its length
constexpr float OldestCave = 2.0e6f;      // yr
constexpr float LevelRise = 8.0f;         // m at least between a passage and the abandoned level above it
constexpr float LeastIncision = 1.0e-6f;  // m/yr of valley cutting below which no level was ever left dry
constexpr float LeastRoof = 2.0f;         // m of ground an abandoned level needs over it to have survived
constexpr float WidestShaft = 4.0f;       // m radius
// m above the local level's ground that a mouth rises to beside its spring. The finer levels put
// the vent's ground within metres of the local level's, so the passage meets it there and opens;
// above the ground the tube is air anyway.
constexpr float MouthRise = 12.0f;

float radiusOf(double flux) { return std::min(MostConduit, ConduitScale * std::pow(float(flux) / PerYear, 0.4f)); }

// A karst node as a point of a passage.
struct Point {
    Vec2 at;
    float height{}, radius{}, strike{};
    uint8_t order{};
};

// The water one outlet gathers: flow-weighted sums of what its nodes took in.
struct Outflow {
    int32_t node{};
    double flux{};
    Borne sum;
    float passages{}, grew{};
    int32_t spring{-1};   // index into the tile's springs; -1 where the water comes out unseen or not at all
};

struct Making {
    const WaterField& field;
    const KarstNet& net;
    GroundwaterTile& tile;
    CaveField& caves;
    std::vector<Point> point;
};

const AquiferColumn& columnOf(const Making& making, int32_t node) { return making.field.column[size_t(node) / 2]; }

Material rockOf(const Making& making, int32_t node) { return node & 1 ? columnOf(making, node).solubleRock : columnOf(making, node).upperSolubleRock; }

// A cell with at least one of the tile's own cells between it and the edge: no tube around it reaches a neighbouring tile.
bool inside(const Making& making, int32_t node) {
    const WaterField& field = making.field;
    const int x = int(size_t(node) / 2 % size_t(field.size)), z = int(size_t(node) / 2 / size_t(field.size));
    return field.owned(x - 1, z - 1) && field.owned(x + 1, z + 1);
}

// Where a node's passage runs: on the joint plane its way out follows, just under the water's
// stand while that lies in the soluble rock, on the rock's base where the water runs thin on it.
Point pointOf(const Making& making, int32_t node) {
    const WaterField& field = making.field;
    const size_t cell = size_t(node) / 2;
    const AquiferColumn& here = field.column[cell];
    Point made;
    // A passage stays inside the soluble rock it is cut in.
    const float thickness = node & 1 ? here.solubleTop - here.solubleBottom : here.upperSoluble;
    made.radius = std::min(radiusOf(making.net.flux[size_t(node)]), 0.5f * thickness);
    made.at = field.centre(int(cell % size_t(field.size)), int(cell / size_t(field.size)));
    if (here.fissured) {
        const Fissures& fissures = field.fissures[cell];
        int set = std::abs(fissures.zone[0]) <= std::abs(fissures.zone[1]) ? 0 : 1, grade = std::abs(fissures.zone[set]) <= PlaneReach ? 2 : 0;
        if (const int32_t parent = making.net.parent[size_t(node)]; parent >= 0) grade = planeAlong(field, cell, size_t(parent) / 2, making.net.toward[size_t(node)], set);
        const float offset = grade == 2 ? fissures.zone[set] : fissures.corridor[set];
        if (std::abs(offset) <= PlaneReach) {
            made.at = made.at + Vec2{-std::sin(fissures.strike[set]), std::cos(fissures.strike[set])} * double(offset);
            made.order = uint8_t(grade == 2 ? 2 : 1), made.strike = fissures.strike[set];
        }
    }
    const float bottom = (node & 1 ? here.solubleBottom : here.floor) + 0.8f * made.radius;
    const float top = (node & 1 ? here.solubleTop : here.floor + here.upperSoluble) - made.radius;
    made.height = std::clamp(making.net.head[size_t(node)] - made.radius - 0.5f, bottom, std::max(bottom, top));
    return made;
}

// yr a passage took to open: until water broke through the fissures to the outlet, then for its walls to dissolve back.
float grewIn(const Making& making, int32_t node, float radius) {
    const float run = making.net.run[size_t(node)] / 1000.0f;
    const float pace = WallRetreat * definitionOf(rockOf(making, node)).solubility / definition::Limestone.solubility;
    return std::min(OldestCave, Breakthrough * (1.0f + run * run) + radius / std::max(1e-6f, pace));
}

// `kind` 0 for a passage, 1 for a shaft, 2 for a mouth.
void addPassage(Making& making, int32_t node, Vec2 a, float ay, Vec2 b, float by, float radius, float flux, uint8_t level, float abandoned, uint8_t order, uint64_t spring, int kind) {
    const WaterField& field = making.field;
    const size_t cell = size_t(node) / 2;
    const int64_t cellX = field.firstX + int64_t(cell % size_t(field.size)), cellZ = field.firstZ + int64_t(cell / size_t(field.size));
    const float age = grewIn(making, node, radius);
    const uint32_t id = uint32_t(hash(stream(field.seed, CaveStream), cellX, cellZ, (node & 1) + 2 * level + 8 * kind));
    making.caves.conduits.push_back({a.x, ay, a.z, b.x, by, b.z, radius, flux / PerYear, age + abandoned, id});
    const AquiferColumn& column = columnOf(making, node);
    ConduitRecord record;
    record.spring = spring;
    record.rock = rockOf(making, node), record.floor = node & 1 ? Material::Air : column.floorRock;
    record.strike = making.point[size_t(node)].strike, record.order = order, record.level = level;
    record.catchment = float(making.net.flux[size_t(node)]) / std::max(1e-4f, field.borne[size_t(node)].rate);
    record.grew = age, record.abandoned = abandoned;
    record.cover = column.ground - std::max(ay, by) - radius;
    record.opens = uint8_t(kind);
    making.tile.passages.push_back(record);
}

// What each outlet yields: a spring where it comes out on land, baseflow under open water, a seep
// where it is too little, underflow where it leaves across the tile's edge.
void openOutlets(Making& making, std::vector<Outflow>& outflows) {
    const WaterField& field = making.field;
    GroundwaterTile& tile = making.tile;
    for (Outflow& out : outflows) {
        const size_t cell = size_t(out.node) / 2;
        const int x = int(cell % size_t(field.size)), z = int(cell / size_t(field.size));
        const size_t own = size_t(z - field.margin) * TileCells + size_t(x - field.margin);
        const AquiferColumn& column = field.column[cell];
        const Outlet outlet = making.net.outlet[size_t(out.node)];
        if (outlet == Outlet::Underflow) {
            tile.budget.underflow += out.flux;
            continue;
        }
        tile.emergence[own] += float(out.flux);
        // Under standing water the conduit's water joins it unseen; a vent is water coming out on land.
        if (column.drain == Drain::Still || out.flux < LeastSpring) {
            (column.drain != Drain::None ? tile.budget.baseflow : tile.budget.seepage) += out.flux;
            continue;
        }
        VentWater water;
        water.kind = SpringKind::Karst;
        water.position = making.point[size_t(out.node)].at, water.elevation = column.ground;
        if (outlet == Outlet::Lip) lipOf(field, x, z, water.position, water.elevation);
        // The vent lies in the cell its water leaves the ground in, wherever the joint plane or the lip lies.
        const Vec2 corner = field.centre(x, z) - Vec2{0.5 * WaterCell, 0.5 * WaterCell};
        water.position = {std::clamp(water.position.x, corner.x + 0.5, corner.x + WaterCell - 0.5), std::clamp(water.position.z, corner.z + 0.5, corner.z + WaterCell - 0.5)};
        water.flow = float(out.flux);
        const float inverse = 1.0f / float(out.flux);
        water.borne = {out.sum.lime * inverse, out.sum.far * inverse, out.sum.high * inverse, out.sum.rate * inverse, out.sum.age * inverse, out.sum.deep * inverse};
        water.depth = water.borne.deep;
        water.cell = int32_t(own), water.storey = uint8_t(out.node & 1);
        water.aquifer = rockOf(making, out.node), water.floor = out.node & 1 ? Material::Air : column.floorRock;
        out.spring = int32_t(tile.springs.size());
        addSpring(field, column, water, tile);
    }
}

// Levels the water table left behind while the valley at the outlet cut down: the same plan, higher and dry.
void addAbandoned(Making& making, int32_t node, Vec2 end, float endHeight, uint64_t spring) {
    const KarstNet& net = making.net;
    const int32_t parent = net.parent[size_t(node)], root = net.root[size_t(node)];
    const Point& here = making.point[size_t(node)];
    const AquiferColumn &rootColumn = columnOf(making, root), &column = columnOf(making, node), &next = columnOf(making, parent);
    const float rise = std::max(LevelRise, 3.0f * making.point[size_t(root)].radius + 4.0f);
    for (uint8_t level = 1; level <= 2; ++level) {
        const float radius = here.radius * (level == 1 ? 0.7f : 0.5f), lift = rise * level, abandoned = lift / rootColumn.incision;
        if (radius < LeastConduit || abandoned > OldestCave) return;
        const float top = std::max(here.height, endHeight) + lift + radius;
        if (top > std::min(column.solubleTop, next.solubleTop) || top > std::min(column.ground, next.ground) - LeastRoof) return;
        addPassage(making, node, here.at, here.height + lift, end, endHeight + lift, radius, 0, level, abandoned, here.order, spring, 0);
    }
}

} // namespace

void growConduits(const WaterField& field, GroundwaterTile& tile) {
    auto caves = std::make_shared<CaveField>();
    caves->key = field.key;
    const KarstNet net = drainKarst(field);
    Making making{field, net, tile, *caves, std::vector<Point>(net.order.empty() ? 0 : field.cells() * 2)};
    for (const int32_t node : net.order) making.point[size_t(node)] = pointOf(making, node);

    // Outlets first: every passage names the spring it drains to. What a system's water brings is
    // the mean of what its nodes took in, with the way through the passages added to its path.
    std::vector<Outflow> outflows;
    std::vector<int32_t> slot(making.point.size(), -1);
    for (size_t at = net.order.size(); at-- > 0;) {
        const int32_t node = net.order[at];
        if (net.parent[size_t(node)] >= 0) continue;
        slot[size_t(node)] = int32_t(outflows.size());
        outflows.push_back({node});
    }
    for (const int32_t node : net.order) {
        Outflow& out = outflows[size_t(slot[size_t(net.root[size_t(node)])])];
        const float taken = field.fate[size_t(node)].captured;
        const Borne& from = field.borne[size_t(node)];
        out.flux += taken;
        out.sum.lime += taken * from.lime, out.sum.far += taken * (from.far + net.run[size_t(node)]), out.sum.high += taken * from.high;
        out.sum.rate += taken * from.rate, out.sum.age += taken * from.age, out.sum.deep += taken * from.deep;
    }
    openOutlets(making, outflows);

    for (const int32_t node : net.order) {
        const Point& here = making.point[size_t(node)];
        const int32_t parent = net.parent[size_t(node)];
        Outflow& out = outflows[size_t(slot[size_t(net.root[size_t(node)])])];
        const uint64_t spring = out.spring >= 0 ? tile.springs[size_t(out.spring)].id : 0;
        if (here.radius < LeastConduit || !inside(making, node)) continue;
        Vec2 end = here.at;
        float endHeight = here.height;
        if (parent >= 0) {
            if (!inside(making, parent)) continue;
            end = making.point[size_t(parent)].at, endHeight = making.point[size_t(parent)].height;
        } else if (out.spring >= 0) {
            // The mouth: up to the spring, on a little beyond it where the passage comes from aside,
            // and rising, so that it reaches daylight whatever the finer ground does there.
            const Spring& vent = tile.springs[size_t(out.spring)];
            const Vec2 way = vent.position - here.at;
            end = vent.position + (length(way) > 1.0 ? normalized(way) * 12.0 : Vec2{});
            endHeight = vent.elevation + MouthRise;
        } else {
            continue;
        }
        addPassage(making, node, here.at, here.height, end, endHeight, here.radius, float(net.flux[size_t(node)]), 0, 0, here.order, spring, parent >= 0 ? 0 : 2);
        out.passages += float(std::hypot(length(end - here.at), double(endHeight - here.height)));
        out.grew = std::max(out.grew, tile.passages.back().grew);
        if ((node & 1) && parent >= 0 && columnOf(making, net.root[size_t(node)]).incision >= LeastIncision) addAbandoned(making, node, end, endHeight, spring);

        // A shaft where a stream goes underground into this passage: the stream's water enters the storey at the top of the column.
        const AquiferColumn& column = columnOf(making, node);
        const float shaft = std::min(WidestShaft, 0.6f * radiusOf(column.sunk));
        if (column.stream >= DrainingFlow && shaft >= LeastConduit && (node & 1) == (column.perched ? Upper : Lower))
            addPassage(making, node, here.at, here.height, here.at, column.ground + 1.0f, shaft, column.sunk, 0, 0, 0, spring, 1);
    }
    for (const Outflow& out : outflows)
        if (out.spring >= 0) tile.records[size_t(out.spring)].passages = out.passages, tile.records[size_t(out.spring)].grew = out.grew;
    indexCaves(*caves);
    tile.caves = std::move(caves);
}

} // namespace derivean::sim
