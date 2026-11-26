#include "ground/fn/SiteView.hpp"

#include "core/type/Field.hpp"

namespace derivean::sim {

SiteView readSites(TileKey key, const TileLookup& lookup) {
    SiteView view;
    const int own = TileCells / Refinement;
    view.cell = specOf(Level::Site).cell;
    view.firstX = int64_t(key.x) * own - FallReachCells;
    view.firstZ = int64_t(key.z) * own - FallReachCells;
    view.side = own + 2 * FallReachCells;
    const size_t cells = size_t(view.side) * size_t(view.side);
    for (auto* field : {&view.ground, &view.cut, &view.coarse, &view.fine, &view.discharge, &view.shed}) field->assign(cells, 0.0f);
    view.flowTo.assign(cells, 255);
    view.rock.assign(cells, uint8_t(Material::Bedrock));
    view.wet.assign(cells, 0);
    view.known.assign(cells, 0);
    const Tile* tile = nullptr;
    TileKey held{Level::Site, INT32_MIN, INT32_MIN};
    for (int z = 0; z < view.side; ++z) {
        for (int x = 0; x < view.side; ++x) {
            const int64_t gx = view.firstX + x, gz = view.firstZ + z;
            const TileKey wanted{Level::Site, int32_t(std::floor(double(gx) / TileCells)), int32_t(std::floor(double(gz) / TileCells))};
            if (!(wanted == held)) tile = lookup(wanted), held = wanted;
            if (!tile) continue;
            const size_t from = size_t(gz - int64_t(wanted.z) * TileCells) * TileCells + size_t(gx - int64_t(wanted.x) * TileCells), to = view.index(x, z);
            view.coarse[to] = tile->now.sediment[Coarse][from];
            view.fine[to] = tile->now.sediment[Sand][from] + tile->now.sediment[Fines][from];
            view.ground[to] = tile->now.rock[from] + view.coarse[to] + view.fine[to];
            view.cut[to] = std::max(0.0f, tile->now.datum[from] - tile->now.rock[from]);
            view.discharge[to] = tile->discharge[from];
            view.shed[to] = tile->shed[from];
            view.flowTo[to] = tile->flowTo[from];
            view.rock[to] = tile->rock[from];
            view.wet[to] = tile->water[from] > view.ground[to] + 1e-3f;
            view.known[to] = 1;
        }
    }
    return view;
}

float SiteView::groundAt(Vec2 position) const {
    const double fx = position.x / cell - 0.5 - double(firstX), fz = position.z / cell - 0.5 - double(firstZ);
    const int ix = int(std::floor(fx)), iz = int(std::floor(fz));
    double wx[4], wz[4];
    cubicWeights(fx - ix, wx);
    cubicWeights(fz - iz, wz);
    double sum = 0;
    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i) sum += wx[i] * wz[j] * ground[index(std::clamp(ix - 1 + i, 0, side - 1), std::clamp(iz - 1 + j, 0, side - 1))];
    return float(sum);
}

IceSource iceSourceAt(const TileLookup& lookup, Vec2 position) {
    const TileKey key = tileAt(Level::Site, position);
    const Tile* tile = lookup(key);
    if (!tile || tile->upIce.empty()) return {};
    const int span = TileCells / Refinement;
    const double parent = specOf(Level::Local).cell;
    const int px = std::clamp(int(std::floor(position.x / parent)) - key.x * span, 0, span - 1);
    const int pz = std::clamp(int(std::floor(position.z / parent)) - key.z * span, 0, span - 1);
    return tile->upIce[size_t(pz) * size_t(span) + size_t(px)];
}

Upstream upstreamOf(const SiteView& view, Vec2 position, float distance) {
    int x = std::clamp(int(std::floor(position.x / view.cell)) - int(view.firstX), 0, view.side - 1);
    int z = std::clamp(int(std::floor(position.z / view.cell)) - int(view.firstZ), 0, view.side - 1);
    Upstream found{view.centre(x, z), Material(view.rock[view.index(x, z)]), 0};
    while (found.followed < distance) {
        // The donor that brings the most water is the channel.
        int best = -1;
        float most = 0;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (!view.inside(nx, nz) || !view.known[view.index(nx, nz)]) continue;
            const uint8_t to = view.flowTo[view.index(nx, nz)];
            if (to > 7 || NeighborX[to] != -NeighborX[k] || NeighborZ[to] != -NeighborZ[k]) continue;
            if (view.discharge[view.index(nx, nz)] > most) most = view.discharge[view.index(nx, nz)], best = k;
        }
        if (best < 0) break;
        x += NeighborX[best], z += NeighborZ[best];
        found.followed += float(view.cell) * NeighborReach[best];
        found.at = view.centre(x, z);
        found.rock = Material(view.rock[view.index(x, z)]);
    }
    return found;
}

} // namespace derivean::sim
