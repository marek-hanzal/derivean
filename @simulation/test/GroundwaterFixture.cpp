#include "GroundwaterFixture.hpp"

#include <cstdio>
#include <cstdlib>

using namespace derivean::sim;

namespace {

WaterField fieldOf(TileKey key, int margin, const std::function<HandColumn(int, int)>& describe) {
    WaterField field;
    field.key = key;
    field.seed = 42;
    field.size = TileCells + 2 * margin, field.margin = margin;
    field.firstX = int64_t(key.x) * TileCells - margin, field.firstZ = int64_t(key.z) * TileCells - margin;
    field.seaLevel = 0;
    field.column.assign(field.cells(), AquiferColumn{});
    field.fissures.assign(field.cells(), Fissures{});
    for (int z = 0; z < field.size; ++z)
        for (int x = 0; x < field.size; ++x) {
            const HandColumn hand = describe(int(field.firstX) + x, int(field.firstZ) + z);
            AquiferColumn& column = field.column[field.index(x, z)];
            column.ground = column.bedrock = column.stage = hand.ground;
            column.rain = hand.rain, column.warmth = 8.0f;
            column.recharge = hand.rain * CellArea;
            column.base = hand.layers[hand.count - 1].bottom;
            column.shallowBase = hand.ground - 10.0f;
            column.heat.gradient = 0.022f;
            fillStoreys(column, hand.layers, hand.count);
        }
    return field;
}

} // namespace

WaterField handField(const std::function<HandColumn(int, int)>& describe) { return fieldOf({Level::Local, 0, 0}, 0, describe); }

WaterField handTile(TileKey key, const std::function<HandColumn(int, int)>& describe) { return fieldOf(key, Reach, describe); }

Layer handLayer(Material rock, float top, float bottom, float metresPerSecond, uint32_t unit) {
    Layer layer;
    layer.top = top, layer.bottom = bottom;
    layer.along = metresPerSecond * PerYear, layer.through = 0.1f * layer.along;
    layer.porosity = definitionOf(rock).porosity;
    layer.soluble = definitionOf(rock).solubility;
    layer.rock = rock;
    layer.unit = unit;
    return layer;
}

void expectWater(bool passed, const char* message, double value) {
    if (passed) return;
    std::fprintf(stderr, "ground water: %s (%g)\n", message, value);
    std::exit(1);
}
