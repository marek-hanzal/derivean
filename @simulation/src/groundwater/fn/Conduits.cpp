#include "core/fn/Hash.hpp"
#include "groundwater/fn/Caves.hpp"
#include "groundwater/fn/Conductivity.hpp"
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
constexpr float PlaneReach = 45.0f, FarOff = 30.0f;   // m

float radiusOf(double flux) { return std::min(MostConduit, ConduitScale * std::pow(float(flux) / PerYear, 0.4f)); }

// A karst node as a point of a passage.
struct Point {
    Vec2 at;
    float height{}, radius{}, strike{};
    uint8_t order{};
};

struct Maker {
    const WaterField& field;
    const KarstNet& net;
    GroundwaterTile& tile;
    CaveField& caves;
    std::vector<Point> point;

    const AquiferColumn& column(int32_t node) const { return field.column[size_t(node) / 2]; }
    Material rockOf(int32_t node) const { return node & 1 ? column(node).solubleRock : column(node).upperRock; }
    bool inside(int32_t node) const {
        const int x = int(size_t(node) / 2 % size_t(field.size)), z = int(size_t(node) / 2 / size_t(field.size));
        return field.owned(x - 1, z - 1) && field.owned(x + 1, z + 1);
    }

    // Where a node's passage runs: on the joint plane its way out follows, at the water table in
    // rock that stands in water and on the tight bed where the water runs on one.
    Point pointOf(int32_t node) const {
        const size_t cell = size_t(node) / 2;
        const AquiferColumn& here = column(node);
        Point made;
        made.radius = radiusOf(net.flux[size_t(node)]);
        made.at = field.centre(int(cell % size_t(field.size)), int(cell / size_t(field.size)));
        if (here.fissures >= 0) {
            const Fissures& fissures = field.fissures[cell];
            int set = std::abs(fissures.zone[0]) <= std::abs(fissures.zone[1]) ? 0 : 1, grade = std::abs(fissures.zone[set]) <= PlaneReach ? 2 : 0;
            if (const int32_t parent = net.parent[size_t(node)]; parent >= 0) grade = planeAlong(field, cell, size_t(parent) / 2, net.toward[size_t(node)], set);
            const float offset = grade == 2 ? fissures.zone[set] : fissures.corridor[set];
            if (std::abs(offset) <= PlaneReach)
                made.at = made.at + Vec2{-std::sin(fissures.strike[set]), std::cos(fissures.strike[set])} * double(std::clamp(offset, -FarOff, FarOff));
            made.order = uint8_t(grade == 2 ? 2 : std::abs(offset) <= PlaneReach ? 1 : 0), made.strike = fissures.strike[set];
        }
        // Just under the water table while that lies in the soluble rock; on its base where the water runs thin on it.
        const float bottom = (node & 1 ? here.solubleBottom : here.floor) + 0.8f * made.radius;
        const float top = (node & 1 ? here.solubleTop : here.floor + here.upperSoluble) - made.radius;
        made.height = std::clamp(net.head[size_t(node)] - made.radius - 0.5f, bottom, std::max(bottom, top));
        return made;
    }

    float grew(int32_t node, float radius) const {
        const float run = net.run[size_t(node)] / 1000.0f, pace = WallRetreat * definitionOf(rockOf(node)).solubility / 45.0f;
        return std::min(OldestCave, Breakthrough * (1.0f + run * run) + radius / std::max(1e-6f, pace));
    }

    void add(int32_t node, Vec2 a, float ay, Vec2 b, float by, float radius, float flux, uint8_t level, float abandoned, uint8_t order, uint64_t spring, int tag) {
        const float age = grew(node, radius);
        caves.conduits.push_back({a.x, ay, a.z, b.x, by, b.z, radius, flux / PerYear, age + abandoned, uint32_t(hash(stream(field.seed, CaveStream), node + int64_t(field.firstX) * 7919 + int64_t(field.firstZ) * 104729, level, tag))});
        ConduitRecord record;
        record.spring = spring;
        record.rock = rockOf(node), record.floor = node & 1 ? Material::Air : column(node).floorRock;
        record.strike = point[size_t(node)].strike, record.order = order, record.level = level;
        record.catchment = float(net.flux[size_t(node)]) / std::max(1e-4f, field.carried[size_t(node)].rate);
        record.grew = age, record.abandoned = abandoned;
        record.cover = column(node).ground - std::max(ay, by) - radius;
        tile.passages.push_back(record);
    }
};

} // namespace

void growConduits(const WaterField& field, GroundwaterTile& tile) {
    auto caves = std::make_shared<CaveField>();
    caves->key = field.key;
    const KarstNet net = drainKarst(field);
    Maker maker{field, net, tile, *caves, std::vector<Point>(net.order.empty() ? 0 : field.cells() * 2)};
    for (const int32_t node : net.order) maker.point[size_t(node)] = maker.pointOf(node);

    // Outlets first: every passage names the spring it drains to. What a system's water carries is
    // the mean of what its nodes took in, with the way through the passages added to its path.
    struct Outflow {
        int32_t node{};
        double flux{};
        Carried sum;
        float passages{}, grew{};
        int32_t spring{-1};   // index into the tile's springs
    };
    std::vector<Outflow> outflows;
    std::vector<int32_t> slot(maker.point.size(), -1);
    for (size_t at = net.order.size(); at-- > 0;) {
        const int32_t node = net.order[at];
        if (net.parent[size_t(node)] >= 0) continue;
        slot[size_t(node)] = int32_t(outflows.size());
        outflows.push_back({node});
    }
    for (const int32_t node : net.order) {
        Outflow& out = outflows[size_t(slot[size_t(net.root[size_t(node)])])];
        const float taken = field.fate[size_t(node)].captured;
        const Carried& from = field.carried[size_t(node)];
        out.flux += taken;
        out.sum.lime += taken * from.lime, out.sum.far += taken * (from.far + net.run[size_t(node)]), out.sum.high += taken * from.high;
        out.sum.rate += taken * from.rate, out.sum.age += taken * from.age, out.sum.deep += taken * from.deep;
    }
    const int side = field.size - 2 * field.margin;
    for (Outflow& out : outflows) {
        const size_t cell = size_t(out.node) / 2;
        const int x = int(cell % size_t(field.size)), z = int(cell / size_t(field.size));
        const size_t own = size_t(z - field.margin) * size_t(side) + size_t(x - field.margin);
        const AquiferColumn& column = field.column[cell];
        tile.emergence[own] += float(out.flux);
        // Under a lake or the sea the water joins it unseen; a vent is water coming out on land.
        const bool drowned = column.drain == Drain::Lake || column.drain == Drain::Sea;
        if (drowned || out.flux < LeastSpring) {
            (column.drain != Drain::None ? tile.budget.baseflow : tile.budget.seepage) += out.flux;
            continue;
        }
        VentWater water;
        water.kind = SpringKind::Karst;
        water.position = maker.point[size_t(out.node)].at, water.elevation = column.ground;
        if (net.outlet[size_t(out.node)] == Outlet::Lip && column.drain == Drain::None) lipOf(field, x, z, water.position, water.elevation);
        water.flow = float(out.flux);
        const float inverse = 1.0f / float(out.flux);
        water.carried = {out.sum.lime * inverse, out.sum.far * inverse, out.sum.high * inverse, out.sum.rate * inverse, out.sum.age * inverse, out.sum.deep * inverse};
        water.cell = int32_t(own), water.storey = uint8_t(out.node & 1);
        water.aquifer = maker.rockOf(out.node), water.floor = out.node & 1 ? Material::Air : column.floorRock;
        out.spring = int32_t(tile.springs.size());
        addSpring(field, column, water, tile);
    }

    for (const int32_t node : net.order) {
        const Point& here = maker.point[size_t(node)];
        const int32_t parent = net.parent[size_t(node)], root = net.root[size_t(node)];
        Outflow& out = outflows[size_t(slot[size_t(root)])];
        const uint64_t spring = out.spring >= 0 ? tile.springs[size_t(out.spring)].id : 0;
        if (here.radius < LeastConduit || !maker.inside(node)) continue;
        Vec2 end = here.at;
        float endHeight = here.height;
        if (parent >= 0) {
            if (!maker.inside(parent)) continue;
            end = maker.point[size_t(parent)].at, endHeight = maker.point[size_t(parent)].height;
        } else if (out.spring >= 0) {
            // The mouth: out to the spring and a little beyond, so that the passage reaches daylight whatever the finer ground does there.
            const Spring& vent = tile.springs[size_t(out.spring)];
            const Vec2 way = vent.position - here.at;
            end = vent.position + (length(way) > 1.0 ? normalized(way) * 12.0 : Vec2{});
            endHeight = vent.elevation + 0.3f * here.radius;
        } else {
            continue;
        }
        maker.add(node, here.at, here.height, end, endHeight, here.radius, float(net.flux[size_t(node)]), 0, 0, here.order, spring, 0);
        out.passages += float(std::hypot(length(end - here.at), double(endHeight - here.height)));
        out.grew = std::max(out.grew, tile.passages.back().grew);

        // Levels the water table left behind while the valley at the outlet cut down: the same plan, higher and dry.
        const AquiferColumn &rootColumn = maker.column(root), &column = maker.column(node);
        if (!(node & 1) || parent < 0 || rootColumn.incision < LeastIncision) continue;
        const AquiferColumn& next = maker.column(parent);
        const float rise = std::max(LevelRise, 3.0f * maker.point[size_t(root)].radius + 4.0f);
        for (uint8_t level = 1; level <= 2; ++level) {
            const float radius = here.radius * (level == 1 ? 0.7f : 0.5f), lift = rise * level, abandoned = lift / rootColumn.incision;
            if (radius < LeastConduit || abandoned > OldestCave) break;
            const float top = std::max(here.height, endHeight) + lift + radius;
            if (top > std::min(column.solubleTop, next.solubleTop) || top > std::min(column.ground, next.ground) - LeastRoof) break;
            maker.add(node, here.at, here.height + lift, end, endHeight + lift, radius, 0, level, abandoned, here.order, spring, level);
        }
    }
    // Shafts: where a stream goes underground, down to the passage that takes it.
    for (const int32_t node : net.order) {
        const AquiferColumn& column = maker.column(node);
        const float radius = std::min(4.0f, 0.6f * radiusOf(column.sunk));
        if (radius < LeastConduit || !maker.inside(node) || ((node & 1) && field.karst[size_t(node) - 1])) continue;
        const Point& here = maker.point[size_t(node)];
        const Outflow& out = outflows[size_t(slot[size_t(net.root[size_t(node)])])];
        maker.add(node, here.at, here.height, here.at, column.ground + 1.0f, radius, column.sunk, 0, 0, 0, out.spring >= 0 ? tile.springs[size_t(out.spring)].id : 0, 3);
    }
    for (const Outflow& out : outflows)
        if (out.spring >= 0) tile.records[size_t(out.spring)].passages = out.passages, tile.records[size_t(out.spring)].grew = out.grew;
    indexCaves(*caves);
    tile.caves = std::move(caves);
}

} // namespace derivean::sim
