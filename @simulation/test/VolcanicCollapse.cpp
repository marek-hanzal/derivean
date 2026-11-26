#include "volcanic/fn/Emplace.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Volcanic collapse: " << message << " (" << value << ")\n";
    std::exit(1);
}

double sum(const std::vector<float>& values) {
    double total = 0;
    for (float value : values) total += value;
    return total;
}

} // namespace

// A roof that goes down under level ground and under a cone: the pit holds the chamber's volume
// either way, a block on the plain and a level floor across the cone's truncated top.
void testVolcanicCollapse() {
    constexpr double Volume = 40e9;   // m³, Crater Lake's order
    Emplacement field;
    field.resize(96, 96);
    field.cell = 250;
    field.origin = {-12000, -12000};
    const Vec2 site{125, -125};
    const std::vector<float> block = blockDrop(field, site, Volume, Magma::Andesite);
    const double area = field.cell * field.cell;
    expect(std::abs(sum(block) * area / Volume - 1) < 0.02, "the block does not hold the volume that left", sum(block) * area / Volume);
    std::fill(field.ground.begin(), field.ground.end(), 300.0f);
    std::vector<float> drop = founder(field, block);
    double apart = 0;
    for (size_t i = 0; i < block.size(); ++i) apart = std::max(apart, double(std::abs(drop[i] - block[i])));
    expect(apart < 0.01, "on level ground the pit is not the block", apart);
    // A cone 3 km high and 10 km in radius over the chamber: its top stands far above the block's depth.
    const double radius = calderaRadius(Volume, Magma::Andesite);
    for (int z = 0; z < field.height; ++z)
        for (int x = 0; x < field.width; ++x) field.ground[field.index(x, z)] = float(300.0 + std::max(0.0, 3000.0 * (1.0 - length(field.center(x, z) - site) / 10000.0)));
    drop = founder(field, block);
    expect(std::abs(sum(drop) / sum(block) - 1) < 1e-4, "the collapse into a cone does not hold the volume that left", sum(drop) / sum(block));
    double low = 1e30, high = -1e30, rim = 0, around = 0;
    for (int z = 0; z < field.height; ++z) {
        for (int x = 0; x < field.width; ++x) {
            const size_t i = field.index(x, z);
            const double distance = length(field.center(x, z) - site), after = field.ground[i] - drop[i];
            if (distance < 0.8 * radius) low = std::min(low, after), high = std::max(high, after);
            if (distance > radius + 2 * field.cell && distance < radius + 3 * field.cell) rim += field.ground[i], ++around;
        }
    }
    expect(high - low < 1.0, "the floor across a truncated cone is not level", high - low);
    rim /= around;
    expect(rim - high > 200.0, "the truncated cone has no pit below its rim", rim - high);
    expect(*std::max_element(drop.begin(), drop.end()) > 1.3f * *std::max_element(block.begin(), block.end()), "the summit drops no further than the block", 0);
}
