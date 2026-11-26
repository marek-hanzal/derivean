#pragma once

#include "landscape/fn/Process.hpp"

#include <functional>

// A ground domain built by hand: one rock, no layering, a surface and a mantle given cell by cell.
// Its rim is open where `ground` is lowest along it. Every per-cell array a ground stage reads exists.
struct GroundSpec {
    derivean::sim::Material rock{derivean::sim::Material::Granite};
    std::function<float(int x, int z)> ground;   // m, top of the rock
    float coarse{}, sand{}, fines{};             // m of mantle everywhere
    float rain{0.8f}, warmth{8.0f};              // m/yr; °C at the ground
    float frost{};                               // share of the year in the cracking window
};

derivean::sim::Domain groundDomain(const GroundSpec& spec);

void expectGround(bool passed, const char* message, double value = 0);

bool sameTile(const derivean::sim::Tile& a, const derivean::sim::Tile& b);
