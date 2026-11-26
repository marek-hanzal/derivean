#pragma once

#include "groundwater/fn/Stages.hpp"

// Ground water fields built by hand: a whole tile with no neighbours, every column given by its layers.
struct HandColumn {
    float ground{};
    float rain{0.3f};              // m/yr entering the ground
    derivean::sim::Layer layers[4];
    int count{};
};

// A field of TileCells² columns at the world's origin with `margin` 0. `column(x, z)` describes each.
derivean::sim::WaterField handField(const std::function<HandColumn(int x, int z)>& column);

// One bed from `top` down to `bottom`, conducting `metresPerSecond` along the bedding and a tenth of it across.
derivean::sim::Layer handLayer(derivean::sim::Material rock, float top, float bottom, float metresPerSecond, uint32_t unit = 0);

void expectWater(bool passed, const char* message, double value = 0);
