#include "ground/fn/Stages.hpp"

#include "ground/constant/Ground.hpp"
#include "ground/type/Detail.hpp"

namespace derivean::sim {
namespace {

constexpr float Wet = 0.02f;        // m of water over the ground that counts as standing
constexpr float BedDepth = 2.5f;    // depths of a stream that its bed can lie under its water beside the line it is routed by

} // namespace

void standWater(Domain& domain, FlowScratch& scratch, const std::vector<Clast>& stones, const GroundDetail& detail, Tile& tile) {
    const int size = domain.size;
    // Stones and spring deposits stand in the water's way: for the last routing they are part of the bed.
    std::vector<float> raised(domain.cells(), 0.0f);
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) raised[domain.index(x, z)] = detail.crust[size_t(z - Halo) * TileCells + size_t(x - Halo)];
    for (const Clast& stone : stones) {
        const int cx = Halo + int(stone.x / float(domain.cell)), cz = Halo + int(stone.z / float(domain.cell));
        const int reach = std::max(0, int(0.8f * stone.b / float(domain.cell)));
        for (int z = std::max(0, cz - reach); z <= std::min(size - 1, cz + reach); ++z)
            for (int x = std::max(0, cx - reach); x <= std::min(size - 1, cx + reach); ++x) {
                const size_t i = domain.index(x, z);
                raised[i] = std::max(raised[i], stone.y + 0.8f * stone.c - domain.ground.rock[i] - domain.sedimentAt(i));
            }
    }
    for (size_t i = 0; i < domain.cells(); ++i) domain.ground.rock[i] += raised[i];
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    widenChannels(domain);
    for (size_t i = 0; i < domain.cells(); ++i) domain.ground.rock[i] -= raised[i];
    extractTile(domain, scratch, tile);

    // Level of the water over each cell, or far below where there is none. From the line a stream
    // is routed by and from the cells the parent holds water over, the water reaches the cells beside
    // them: a stream covers its bed from bank to bank, a film over what stands a little higher, and a
    // lake reaches the shore the ground itself draws. It goes no farther than the stream is wide or
    // a parent cell is, and stands no deeper than a bed can lie under its line: water that would have
    // to do either is running off, not standing there.
    struct Reach {
        int32_t from{-1};   // cell the water comes from
        float level{};      // m, its surface there
        float film{};       // m above that surface which it still wets
        float deepest{};    // m of depth beyond which it does not stand
        float far{};        // cells from `from` it reaches
    };
    std::vector<float> stage(domain.cells(), -1e9f);
    std::vector<Reach> reach(domain.cells());
    std::vector<int32_t> front;
    for (size_t i = 0; i < domain.cells(); ++i) {
        const bool standing = domain.level[i] > domain.surface[i] + Wet;
        if (standing && domain.discharge[i] >= PoolSupply) stage[i] = domain.level[i];
        else if (!standing && domain.discharge[i] >= RunningFlow) {
            const float depth = streamDepth(domain.discharge[i]);
            stage[i] = domain.surface[i] + depth;
            reach[i] = {int32_t(i), stage[i], 0.5f * depth, BedDepth * depth, 0.5f * streamWidth(domain.discharge[i]) / float(domain.cell) + 1.0f};
            front.push_back(int32_t(i));
        }
        if (!domain.standing.empty() && domain.standing[i] > domain.surface[i] && domain.standing[i] > stage[i]) {
            if (reach[i].from < 0) front.push_back(int32_t(i));
            stage[i] = domain.standing[i];
            reach[i] = {int32_t(i), stage[i], 0.0f, 1e9f, float(Refinement)};
        }
    }
    for (size_t next = 0; next < front.size(); ++next) {
        const size_t at = size_t(front[next]);
        const Reach& mine = reach[at];
        const int x = int(at % size_t(size)), z = int(at / size_t(size)), fromX = mine.from % size, fromZ = mine.from / size;
        for (int k = 0; k < 8; k += 2) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            const float away = float((nx - fromX) * (nx - fromX) + (nz - fromZ) * (nz - fromZ));
            if (stage[other] > -1e8f || away > mine.far * mine.far) continue;
            if (domain.surface[other] >= mine.level + mine.film - 0.005f || mine.level - domain.surface[other] > mine.deepest) continue;
            stage[other] = std::max(mine.level, domain.surface[other] + Wet);
            reach[other] = mine;
            front.push_back(int32_t(other));
        }
    }
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            const float ground = tile.now.rock[to] + tile.now.sediment[0][to] + tile.now.sediment[1][to] + tile.now.sediment[2][to];
            tile.water[to] = stage[from] > -1e8f ? stage[from] : ground - 1.0f;
        }
}

} // namespace derivean::sim
