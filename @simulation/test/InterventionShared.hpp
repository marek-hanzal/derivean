#pragma once

#include "intervention/fn/Scenario.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

inline void expectLive(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Intervention: " << message << " (" << value << ")\n";
    std::exit(1);
}

// Budgets must close to this share of what moved.
inline constexpr double Closed = 1e-9;

inline void expectClosed(const derivean::sim::ScenarioReport& report) {
    expectLive(report.failure.empty(), report.failure.c_str());
    for (const derivean::sim::ScenarioStage& stage : report.stages) {
        expectLive(waterImbalance(stage.budget) <= Closed, "water is unaccounted for", waterImbalance(stage.budget));
        expectLive(saltImbalance(stage.budget) <= Closed, "salt is unaccounted for", saltImbalance(stage.budget));
        expectLive(solidImbalance(stage.budget) <= Closed, "solid is unaccounted for", solidImbalance(stage.budget));
    }
}

// Every bit of the live state: columns, water and salt of every live tile.
inline uint64_t digestOf(const derivean::sim::World& world) {
    uint64_t digest = 0xcbf29ce484222325ULL;
    const auto fold = [&](const void* data, size_t bytes) {
        const auto* at = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < bytes; ++i) digest = (digest ^ at[i]) * 0x100000001b3ULL;
    };
    for (const auto& entry : world.interventions.overlay.tiles) {
        const derivean::sim::EditedTile& tile = *entry.second;
        fold(tile.now.rock.data(), tile.now.rock.size() * sizeof(float));
        for (const auto& layer : tile.now.sediment) fold(layer.data(), layer.size() * sizeof(float));
        fold(tile.depth.data(), tile.depth.size() * sizeof(double));
        fold(tile.salt.data(), tile.salt.size() * sizeof(double));
    }
    return digest;
}

// InterventionReplay.cpp
void testDiversion(derivean::sim::World& world, const derivean::sim::StreamSite& site);
