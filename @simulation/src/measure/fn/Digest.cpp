#include "measure/fn/Digest.hpp"

#include <cstring>

namespace derivean::sim {
namespace {

uint64_t floats(const std::vector<float>& values, uint64_t seed) { return digestOf(values.data(), values.size() * sizeof(float), seed); }

} // namespace

uint64_t digestOf(const void* data, size_t bytes, uint64_t seed) {
    // FNV-1a over 64-bit words, then the tail byte by byte.
    uint64_t state = 0xcbf29ce484222325ULL ^ seed;
    const auto* cursor = static_cast<const unsigned char*>(data);
    for (; bytes >= 8; bytes -= 8, cursor += 8) {
        uint64_t word;
        std::memcpy(&word, cursor, 8);
        state = (state ^ word) * 0x100000001b3ULL;
        state ^= state >> 29;
    }
    for (; bytes > 0; --bytes, ++cursor) state = (state ^ *cursor) * 0x100000001b3ULL;
    return state;
}

TileDigest digestOf(const Tile& tile) {
    TileDigest digest;
    digest.ground = floats(tile.now.rock, 1);
    digest.ground = floats(tile.now.datum, digest.ground);
    for (const auto& layer : tile.now.sediment) digest.ground = floats(layer, digest.ground);
    digest.water = floats(tile.water, 2);
    digest.discharge = floats(tile.discharge, 3);
    return digest;
}

uint64_t digestOf(const Planet& planet) {
    uint64_t state = floats(planet.elevation.cells, 4);
    state = floats(planet.temperature.cells, state);
    state = floats(planet.precipitation.cells, state);
    // Columns carry padding, so their fields are fed one by one.
    for (const CrustColumn& column : planet.columns.cells) {
        const float fields[6] = {column.crust, column.oceanAge, column.load, column.tectonicGain, column.eroded, column.deposited};
        state = digestOf(fields, sizeof(fields), state);
    }
    return digestOf(&planet.seaLevel, sizeof(planet.seaLevel), state);
}

TileDifference differenceOf(const Tile& a, const Tile& b) {
    TileDifference difference;
    const auto ground = [](const Tile& tile, size_t i) { return tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i]; };
    for (size_t i = 0; i < a.water.size(); ++i) {
        bool same = a.now.rock[i] == b.now.rock[i] && a.now.datum[i] == b.now.datum[i] && a.water[i] == b.water[i] && a.discharge[i] == b.discharge[i];
        for (int c = 0; c < SedimentClasses; ++c) same = same && a.now.sediment[c][i] == b.now.sediment[c][i];
        if (same) continue;
        ++difference.cells;
        difference.ground = std::max(difference.ground, double(std::abs(ground(a, i) - ground(b, i))));
        difference.water = std::max(difference.water, double(std::abs(a.water[i] - b.water[i])));
        const double larger = std::max(std::abs(a.discharge[i]), std::abs(b.discharge[i]));
        if (larger > 0) difference.discharge = std::max(difference.discharge, std::abs(a.discharge[i] - b.discharge[i]) / larger);
    }
    return difference;
}

} // namespace derivean::sim
