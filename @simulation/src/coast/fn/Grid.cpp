#include "coast/fn/Survey.hpp"

namespace derivean::sim {
namespace {

ShoreGrid emptyGrid(int side, double cell, double seaLevel) {
    ShoreGrid grid;
    grid.side = side, grid.cell = cell, grid.seaLevel = seaLevel;
    const size_t cells = size_t(side) * size_t(side);
    for (auto* field : {&grid.rock, &grid.cover[0], &grid.cover[1], &grid.cover[2], &grid.wave, &grid.notch, &grid.cut}) field->assign(cells, 0.0f);
    grid.water.assign(cells, NoWater);
    return grid;
}

} // namespace

ShoreGrid shoreGridOf(const Domain& domain) {
    ShoreGrid grid = emptyGrid(domain.size, domain.cell, domain.seaLevel);
    const Shore* shore = domain.shore && domain.shore->top.size() == domain.cells() ? domain.shore.get() : nullptr;
    for (size_t i = 0; i < domain.cells(); ++i) {
        grid.rock[i] = domain.ground.rock[i];
        for (int c = 0; c < SedimentClasses; ++c) grid.cover[c][i] = domain.ground.sediment[c][i];
        if (standsOver(domain, i, grid.top(i))) grid.water[i] = domain.level[i];
        if (!shore) continue;
        grid.notch[i] = shore->notch[i], grid.cut[i] = shore->cut[i];
        for (int k = 0; k < WaveDirections; ++k)
            grid.wave[i] = std::max(grid.wave[i], std::sqrt(std::max(shore->wave[i * WaveDirections + size_t(k)], shore->arrive[i * WaveDirections + size_t(k)])));
    }
    return grid;
}

ShoreGrid shoreGridOf(const std::vector<const Tile*>& tiles, int perSide) {
    const Tile& first = *tiles.front();
    ShoreGrid grid = emptyGrid(perSide * TileCells, specOf(first.key.level).cell, first.seaLevel);
    for (int tz = 0; tz < perSide; ++tz) {
        for (int tx = 0; tx < perSide; ++tx) {
            const Tile& tile = *tiles[size_t(tz) * size_t(perSide) + size_t(tx)];
            for (int z = 0; z < TileCells; ++z) {
                for (int x = 0; x < TileCells; ++x) {
                    const size_t from = size_t(z) * TileCells + size_t(x), to = grid.index(tx * TileCells + x, tz * TileCells + z);
                    grid.rock[to] = tile.now.rock[from];
                    for (int c = 0; c < SedimentClasses; ++c) grid.cover[c][to] = tile.now.sediment[c][from];
                    if (tile.water[from] > grid.top(to) + Film) grid.water[to] = tile.water[from];
                    if (!tile.shore) continue;
                    grid.notch[to] = tile.shore->notch[from], grid.cut[to] = tile.shore->cut[from];
                    for (int k = 0; k < WaveDirections; ++k) grid.wave[to] = std::max(grid.wave[to], WaveQuantum * float(tile.shore->height[from * WaveDirections + size_t(k)]));
                }
            }
        }
    }
    return grid;
}

} // namespace derivean::sim
