#include "groundwater/fn/Stages.hpp"

#include "groundwater/fn/Beds.hpp"
#include "groundwater/fn/Conductivity.hpp"
#include "groundwater/fn/Recharge.hpp"
#include "groundwater/fn/Swallow.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

// Cells between layering nodes: the local landscape's own, so a cell reads the beds it was cut in.
constexpr int Stride = 8;

int64_t floorDiv(int64_t value, int64_t by) { return value >= 0 ? value / by : -((-value + by - 1) / by); }

// The nine finished tiles a field reads, by global cell.
struct Around {
    TileKey key;
    const Tile* const* tiles;

    const Tile& at(int64_t x, int64_t z, size_t& index) const {
        const int64_t tx = floorDiv(x, TileCells), tz = floorDiv(z, TileCells);
        index = size_t(z - tz * TileCells) * TileCells + size_t(x - tx * TileCells);
        return *tiles[(tz - key.z + 1) * 3 + (tx - key.x + 1)];
    }
};

struct Node {
    StrataColumn strata;
    Geotherm heat;
};

// How much easier water moves along the two joint sets than across them, as a tensor of trace 2.
void spreadOf(const RockMass& mass, const MaterialDefinition& rock, float spread[3]) {
    const float joints = jointConductivity(rock, 0.5f * mass.openDepth, 0.5f);
    const float share = joints / (joints + rock.conductivity);
    float weight[JointSets], total = 0;
    for (int set = 0; set < JointSets; ++set) total += weight[set] = mass.sets[set].persistence / mass.sets[set].spacing;
    spread[0] = spread[2] = 1.0f - share, spread[1] = 0;
    for (int set = 0; set < JointSets; ++set) {
        const float c = float(std::cos(mass.sets[set].strike)), s = float(std::sin(mass.sets[set].strike)), part = 2.0f * share * weight[set] / total;
        spread[0] += part * c * c, spread[1] += part * c * s, spread[2] += part * s * s;
    }
}

} // namespace

void readColumns(const Setting& setting, TileKey key, const Tile* const around[9], WaterField& field) {
    field.key = key;
    field.seed = setting.seed;
    field.workers = setting.workers;
    field.firstX = int64_t(key.x) * TileCells - field.margin, field.firstZ = int64_t(key.z) * TileCells - field.margin;
    field.seaLevel = float(around[4]->seaLevel);
    const int size = field.size;
    field.column.assign(field.cells(), AquiferColumn{});
    field.fissures.assign(field.cells(), Fissures{});
    const Around ground{key, around};

    // The surface first: a column's slope and the lie of its beds need its neighbours'.
    std::vector<float> top(field.cells()), datum(field.cells());
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            size_t from;
            const Tile& tile = ground.at(field.firstX + x, field.firstZ + z, from);
            top[field.index(x, z)] = tile.now.rock[from] + tile.now.sediment[Coarse][from] + tile.now.sediment[Sand][from] + tile.now.sediment[Fines][from];
            datum[field.index(x, z)] = tile.now.datum[from];
        }

    // Layering and heat vary over kilometres: read at nodes, as the landscape reads its layering.
    const int nodes = size / Stride + 2;
    const int64_t nodeX = floorDiv(field.firstX, Stride), nodeZ = floorDiv(field.firstZ, Stride);
    std::vector<Node> node(size_t(nodes) * size_t(nodes));
    const auto nodeCentre = [](int64_t x, int64_t z) { return Vec2{(double(x * Stride) + 0.5) * LocalCell, (double(z * Stride) + 0.5) * LocalCell}; };
    const auto readNodes = [&](int begin, int end) {
        for (int n = begin; n < end; ++n) {
            const Vec2 point = planetPoint(setting.seed, nodeCentre(nodeX + n % nodes, nodeZ + n / nodes));
            node[size_t(n)] = {strataAt(*setting.planet, point), geothermAt(*setting.planet, point)};
        }
    };
    if (field.workers) field.workers->parallelFor(nodes * nodes, readNodes, 64);
    else readNodes(0, nodes * nodes);

    forRows(field, [&](int begin, int end) {
        BedWalk walk;
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < size; ++x) {
                const size_t i = field.index(x, z);
                const int64_t cellX = field.firstX + x, cellZ = field.firstZ + z;
                size_t from;
                const Tile& tile = ground.at(cellX, cellZ, from);
                AquiferColumn& column = field.column[i];
                const auto beside = [&](const std::vector<float>& values, int dx, int dz) {
                    return values[field.index(std::clamp(x + dx, 0, size - 1), std::clamp(z + dz, 0, size - 1))];
                };
                const float slopeX = (beside(top, 1, 0) - beside(top, -1, 0)) / float(2 * LocalCell), slopeZ = (beside(top, 0, 1) - beside(top, 0, -1)) / float(2 * LocalCell);

                // Layering under the cell: its node's, with thicknesses blended from the four around it.
                const int nx = int(floorDiv(cellX, Stride) - nodeX), nz = int(floorDiv(cellZ, Stride) - nodeZ);
                const float tx = float(cellX - (nodeX + nx) * Stride) / Stride, tz = float(cellZ - (nodeZ + nz) * Stride) / Stride;
                const Node* corner = &node[size_t(nz) * size_t(nodes) + size_t(nx)];
                const StrataColumn* strata[4] = {&corner[0].strata, &corner[1].strata, &corner[nodes].strata, &corner[nodes + 1].strata};
                const float weight[4] = {(1 - tx) * (1 - tz), tx * (1 - tz), (1 - tx) * tz, tx * tz};
                BedColumn bed;
                bed.seed = setting.seed;
                bed.strata = blendThickness(corner[0].strata, strata, weight);
                bed.node = &corner[0].strata;
                bed.material = corner[0].strata.material + (field.centre(x, z) - nodeCentre(nodeX + nx, nodeZ + nz));
                bed.fold = foldAt(setting.seed, corner[0].strata, bed.material);
                bed.rockTop = tile.now.rock[from], bed.datum = tile.now.datum[from];
                for (int c = 0; c < SedimentClasses; ++c) bed.cover[c] = tile.now.sediment[c][from];
                walk = BedWalk{};
                walkBeds(bed, walk);

                column.ground = top[i];
                column.rain = tile.rain[from], column.warmth = tile.warmth[from];
                column.stream = tile.discharge[from];
                column.incision = std::max(0.0f, tile.erosionRate[from] - tile.depositionRate[from]);
                column.flowTo = tile.flowTo[from];
                const bool flooded = tile.water[from] > column.ground + 1e-3f;
                const bool soluble = solubleRock(definitionOf(walk.top));
                column.drain = flooded ? (tile.water[from] <= field.seaLevel + 0.5f ? Drain::Sea : Drain::Lake)
                             : column.stream >= (soluble ? SwallowLimit : StreamFlow) ? Drain::Stream : Drain::None;
                column.stage = flooded ? tile.water[from] : column.ground;
                Soak soak{column.rain, column.warmth, std::hypot(slopeX, slopeZ), {bed.cover[0], bed.cover[1], bed.cover[2]}, tile.ice[from], Material(tile.rock[from])};
                column.recharge = flooded ? 0.0f : runoffOf(column.rain, column.warmth) * infiltrationShare(soak) * CellArea;
                column.base = walk.base, column.shallowBase = walk.shallowBase;
                column.crush = walk.mass.crush;
                fillStoreys(column, walk.layers, walk.count);

                spreadOf(walk.mass, definitionOf(walk.top), column.spread);
                const float fall = std::tan(walk.mass.dip);
                column.dipX = fall * float(std::cos(walk.mass.dipDirection)) - (beside(datum, 1, 0) - beside(datum, -1, 0)) / float(2 * LocalCell);
                column.dipZ = fall * float(std::sin(walk.mass.dipDirection)) - (beside(datum, 0, 1) - beside(datum, 0, -1)) / float(2 * LocalCell);
                const auto blend = [&](float Geotherm::*member) {
                    return weight[0] * (corner[0].heat.*member) + weight[1] * (corner[1].heat.*member) + weight[2] * (corner[nodes].heat.*member) + weight[3] * (corner[nodes + 1].heat.*member);
                };
                column.heat = {blend(&Geotherm::gradient), blend(&Geotherm::magmatic), blend(&Geotherm::sinceMelt), blend(&Geotherm::centre)};
                if (walk.soluble) {
                    column.fissures = int32_t(i);
                    field.fissures[i] = fissuresAt(setting.seed, walk.solubleMass, bed.material, std::max(0.0f, walk.solubleDepth - 2.0f));
                }
                // What the rock alone says a stream loses here; Sinks.cpp works out how much of the stream is left to lose.
                if (soluble) {
                    const float broken = fractureShare(setting.seed, walk.mass, bed.material, std::max(0.0f, bed.datum - bed.rockTop), float(LocalCell));
                    column.swallow = swallowRate({walk.top, bed.cover[0] + bed.cover[1] + bed.cover[2], broken, walk.mass.openDepth, walk.mass.dip});
                }
            }
        }
    });
}

} // namespace derivean::sim
