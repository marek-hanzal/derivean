#include "relief/type/Ground.hpp"

#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {

GroundLattice readGround(TileReader& reader, double minX, double minZ, double maxX, double maxZ) {
    GroundLattice lattice;
    lattice.spacing = specOf(reader.level).cell;
    lattice.shift = 0.5 * lattice.spacing;
    const auto below = [&](double position) { return int64_t(std::floor((position - lattice.shift) / lattice.spacing)); };
    const double extra = SlopeReach + SmoothReach;
    lattice.originX = below(minX - extra);
    lattice.originZ = below(minZ - extra);
    lattice.countX = int(below(maxX + extra) + 2 - lattice.originX);
    lattice.countZ = int(below(maxZ + extra) + 2 - lattice.originZ);
    const size_t nodes = size_t(lattice.countX) * size_t(lattice.countZ);
    for (auto* field : {&lattice.rock, &lattice.datum, &lattice.cover[0], &lattice.cover[1], &lattice.cover[2], &lattice.crust}) field->assign(nodes, 0.0f);
    lattice.crustKind.assign(nodes, uint8_t(Crust::None));
    lattice.coarseKind.assign(nodes, uint8_t(Material::Gravel));
    lattice.fineKind.assign(nodes, uint8_t(Material::Silt));
    lattice.bare.assign(nodes, 0);
    for (int z = 0; z < lattice.countZ; ++z) {
        const int64_t gz = lattice.originZ + z;
        const int32_t tz = int32_t(std::floor(double(gz) / TileCells));
        for (int x = 0; x < lattice.countX; ++x) {
            const int64_t gx = lattice.originX + x;
            const int32_t tx = int32_t(std::floor(double(gx) / TileCells));
            const Tile* tile = reader.tile(tx, tz);
            if (!tile) continue;
            const size_t from = size_t(gz - int64_t(tz) * TileCells) * TileCells + size_t(gx - int64_t(tx) * TileCells);
            const size_t node = size_t(z) * size_t(lattice.countX) + size_t(x);
            lattice.rock[node] = tile->now.rock[from];
            lattice.datum[node] = tile->now.datum[from];
            for (int c = 0; c < SedimentClasses; ++c) lattice.cover[c][node] = tile->now.sediment[c][from];
            if (tile->volcanic && !tile->volcanic->pile.empty()) {
                if (lattice.pile.empty()) lattice.pile.assign(nodes, PilePoint{});
                lattice.pile[node] = pilePoint(*tile, from);
            }
            if (!tile->detail) continue;
            const GroundDetail& detail = *tile->detail;
            lattice.crust[node] = detail.crust[from];
            lattice.crustKind[node] = detail.crustKind[from];
            lattice.coarseKind[node] = detail.coarseKind[from];
            lattice.fineKind[node] = detail.fineKind[from];
            lattice.bare[node] = detail.bare[from];
        }
    }
    smoothLie(lattice);
    return lattice;
}

void smoothLie(GroundLattice& lattice) {
    const size_t nodes = lattice.rock.size();
    // The mean of the rock surface over a square around each node. Heights are summed as whole
    // numbers of 1/4096 m, so a node's mean is the same whichever rectangle it is read through;
    // nodes nearer the lattice's edge than the square reaches are never read.
    const int reach = int(SmoothReach / lattice.spacing), window = 2 * reach + 1;
    std::vector<int64_t> fixed(nodes), rows(nodes, 0);
    for (size_t node = 0; node < nodes; ++node) fixed[node] = int64_t(std::llround(double(lattice.rock[node]) * 4096.0));
    lattice.lie.assign(nodes, 0.0f);
    for (int z = 0; z < lattice.countZ; ++z) {
        const int64_t* row = &fixed[size_t(z) * size_t(lattice.countX)];
        int64_t sum = 0;
        for (int x = 0; x < std::min(lattice.countX, window); ++x) sum += row[x];
        for (int x = reach; x < lattice.countX - reach; ++x) {
            rows[size_t(z) * size_t(lattice.countX) + size_t(x)] = sum;
            if (x + reach + 1 < lattice.countX) sum += row[x + reach + 1] - row[x - reach];
        }
    }
    for (int x = reach; x < lattice.countX - reach; ++x) {
        int64_t sum = 0;
        for (int z = 0; z < std::min(lattice.countZ, window); ++z) sum += rows[size_t(z) * size_t(lattice.countX) + size_t(x)];
        for (int z = reach; z < lattice.countZ - reach; ++z) {
            lattice.lie[size_t(z) * size_t(lattice.countX) + size_t(x)] = float(double(sum) / (4096.0 * window * window));
            if (z + reach + 1 < lattice.countZ) sum += rows[size_t(z + reach + 1) * size_t(lattice.countX) + size_t(x)] - rows[size_t(z - reach) * size_t(lattice.countX) + size_t(x)];
        }
    }
}

float GroundLattice::lieAt(double x, double z) const {
    const double fx = std::clamp((x - shift) / spacing - double(originX), 0.0, double(countX - 1) - 1e-6);
    const double fz = std::clamp((z - shift) / spacing - double(originZ), 0.0, double(countZ - 1) - 1e-6);
    const int ix = int(fx), iz = int(fz);
    const float tx = float(fx - ix), tz = float(fz - iz);
    const float* a = &lie[size_t(iz) * size_t(countX) + size_t(ix)];
    return (a[0] * (1 - tx) + a[1] * tx) * (1 - tz) + (a[countX] * (1 - tx) + a[countX + 1] * tx) * tz;
}

float GroundLattice::looseAt(double x, double z) const {
    const double fx = std::clamp((x - shift) / spacing - double(originX), 0.0, double(countX - 1) - 1e-6);
    const double fz = std::clamp((z - shift) / spacing - double(originZ), 0.0, double(countZ - 1) - 1e-6);
    const int ix = int(fx), iz = int(fz);
    const float tx = float(fx - ix), tz = float(fz - iz);
    const size_t a = size_t(iz) * size_t(countX) + size_t(ix);
    float loose = 0;
    for (const auto& layer : cover) loose += (layer[a] * (1 - tx) + layer[a + 1] * tx) * (1 - tz) + (layer[a + size_t(countX)] * (1 - tx) + layer[a + size_t(countX) + 1] * tx) * tz;
    return loose;
}

GroundPoint GroundLattice::at(double x, double z) const {
    const double fx = std::clamp((x - shift) / spacing - double(originX), 0.0, double(countX - 1) - 1e-6);
    const double fz = std::clamp((z - shift) / spacing - double(originZ), 0.0, double(countZ - 1) - 1e-6);
    const int ix = int(fx), iz = int(fz);
    const float tx = float(fx - ix), tz = float(fz - iz);
    const size_t a = size_t(iz) * size_t(countX) + size_t(ix), b = a + 1, c = a + size_t(countX), d = c + 1;
    const auto blend = [&](const std::vector<float>& field) {
        return (field[a] * (1 - tx) + field[b] * tx) * (1 - tz) + (field[c] * (1 - tx) + field[d] * tx) * tz;
    };
    GroundPoint point;
    point.rock = blend(rock);
    point.datum = std::max(point.rock, blend(datum));
    point.ground = point.rock;
    for (int k = 0; k < SedimentClasses; ++k) {
        point.cover[k] = blend(cover[k]);
        point.ground += point.cover[k];
    }
    point.crust = blend(crust);
    // Kinds do not blend: a point takes those of the node it is nearest to.
    const size_t nearest = (tz > 0.5f ? c : a) + (tx > 0.5f ? 1 : 0);
    point.crustKind = Crust(crustKind[nearest]);
    point.coarse = Material(coarseKind[nearest]);
    point.fine = Material(fineKind[nearest]);
    point.bare = bare[nearest] != 0;
    if (!pile.empty()) {
        // Under a volcanic pile the layering keeps its own top: only the pile's floor bounds it.
        // Its kinds come from the nearest node that holds one, so a margin reads as its pile.
        point.pile = pile[nearest];
        for (const size_t node : {a, b, c, d})
            if (point.pile.thickness <= 0 && pile[node].thickness > 0) point.pile = pile[node];
        point.pile.thickness = (pile[a].thickness * (1 - tx) + pile[b].thickness * tx) * (1 - tz) + (pile[c].thickness * (1 - tx) + pile[d].thickness * tx) * tz;
        point.datum = std::max(point.rock - point.pile.thickness, blend(datum));
    }
    // The fall line belongs to the slope, not to the step a single ground cell makes on it.
    const float dx = float((lieAt(x + SlopeReach, z) - lieAt(x - SlopeReach, z)) / (2 * SlopeReach));
    const float dz = float((lieAt(x, z + SlopeReach) - lieAt(x, z - SlopeReach)) / (2 * SlopeReach));
    point.slope = std::sqrt(dx * dx + dz * dz);
    point.fall = point.slope > 1e-5f ? Vec2{-dx / point.slope, -dz / point.slope} : Vec2{1, 0};
    return point;
}

} // namespace derivean::sim
