#pragma once

#include "chart/fn/Chart.hpp"

#include <cstdlib>
#include <iostream>

// Shared by the chart checks. A failed check reports to stderr and exits non-zero.
inline void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Chart: " << message << " (" << value << ")\n";
    std::exit(1);
}

inline derivean::sim::Vec2 centreOf(const derivean::sim::Chart& chart, int i, int j) {
    return {chart.origin.x + (i + 0.5) * chart.pitch, chart.origin.z + (j + 0.5) * chart.pitch};
}

// test/ChartWater.cpp: the water of charts finer than the cells they read. `site` and `local` are
// charts around the focus whose pixels partly come from the next coarser level.
void checkChartWater(const derivean::sim::World& world, derivean::sim::Vec2 focus, const derivean::sim::Chart& site, const derivean::sim::Chart& local);
