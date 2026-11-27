#pragma once

#include "hydrology/fn/Ocean.hpp"
#include "hydrology/fn/Water.hpp"

#include <cstdlib>
#include <initializer_list>
#include <iostream>

namespace derivean::sim::sea {

inline void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Hydrology: " << message << " (" << value << ")\n";
    std::exit(1);
}

inline bool near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance * std::max(std::abs(b), 1e-30); }

constexpr double Cell = 1000.0;      // m
constexpr double Salinity = 30.0;    // kg/m³ of the seas the fixtures fill

// Lets time pass and integrates it at once, so that every call is a step of its own.
inline void drain(Ocean& ocean, double seconds) {
    drainOcean(ocean, seconds);
    settleOcean(ocean);
}

// The seas a fixture filled are settled as they stand: only the cells they were filled over hold
// water, so a basin no fill reached is dry however low it lies.
inline Ocean settled(Ocean ocean) {
    ocean.basins = buildBasins(ocean.grid);
    ocean.initial = stockOf(ocean.grid);
    WaterFlows unused;
    ocean.lakes = settle(ocean.grid, ocean.basins, {}, unused);
    ocean.initialCrust = crustOf(ocean.grid);
    ocean.anchor = anchorOf(ocean);
    return ocean;
}

// Flat-floored boxes with vertical walls in a ridge, on a wrapping grid: their wet areas do not
// change from the floor to the ridge. `level` above the floor fills a box as a sea.
struct Box {
    int x0, z0, x1, z1;       // cells, the far ends excluded
    double floor;             // m
    double level{-1e30};      // m
    double salinity{Salinity};   // kg/m³ of what it is filled with
};

inline Ocean boxes(int width, int height, double ridge, std::initializer_list<Box> list, double cell = Cell) {
    Ocean ocean;
    shapeGrid(ocean.grid, width, height, cell);
    ocean.grid.periodic = true;
    for (float& ground : ocean.grid.ground) ground = float(ridge);
    for (const Box& box : list) {
        for (int z = box.z0; z < box.z1; ++z) {
            for (int x = box.x0; x < box.x1; ++x) {
                const size_t cell = size_t(z) * size_t(width) + size_t(x);
                ocean.grid.ground[cell] = float(box.floor);
                ocean.grid.depth[cell] = std::max(0.0, box.level - box.floor);
                ocean.grid.salt[cell] = box.salinity * ocean.grid.depth[cell];
            }
        }
    }
    return settled(std::move(ocean));
}

inline int32_t cellAt(const Ocean& ocean, int x, int z) { return int32_t(z * ocean.grid.width + x); }

// Surface of the water over a cell; its ground where it is dry.
inline double surfaceAt(const Ocean& ocean, int32_t cell) { return double(ocean.grid.ground[size_t(cell)]) + ocean.grid.depth[size_t(cell)]; }

inline double strengthAt(const Ocean& ocean, int32_t cell) { return ocean.grid.salt[size_t(cell)] / ocean.grid.depth[size_t(cell)]; }

inline Strait channel(const Ocean& ocean, int ax, int az, int bx, int bz, double area, double sill) {
    return {cellAt(ocean, ax, az), cellAt(ocean, bx, bz), area, sill};
}

// The bodies still hold all the water and salt the fixture began with, none of it negative, and
// every integration closed each body that took part against its transfers.
inline void expectClosed(const Ocean& ocean, const char* where) {
    const Tally now = stockOf(ocean.grid);   // the crust's salt with it
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell)
        expect(ocean.grid.depth[cell] >= 0 && ocean.grid.salt[cell] >= 0, where, double(cell));
    expect(near(now.water, ocean.initial.water, 1e-12), "the straits made or lost water", now.water / ocean.initial.water - 1);
    expect(near(now.salt, ocean.initial.salt, 1e-12), "the straits made or lost salt", now.salt / ocean.initial.salt - 1);
    expect(ocean.unbooked.water <= 1e-12 && ocean.unbooked.salt <= 1e-12, "a body's change is not what its straits booked", std::max(ocean.unbooked.water, ocean.unbooked.salt));
}

} // namespace derivean::sim::sea
