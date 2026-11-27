#pragma once

#include "Arguments.hpp"

#include "measure/type/Mosaic.hpp"
#include "simulation/type/World.hpp"

#include <chrono>
#include <string>

// Seconds since the previous call; the first call starts the clock.
struct Lap {
    std::chrono::steady_clock::time_point last{std::chrono::steady_clock::now()};
    double operator()() {
        const auto now = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(now - last).count();
        last = now;
        return seconds;
    }
};

// MeasureSite.cpp — one mosaic of a place: how high, how rough, how regular.
void printScale(const derivean::sim::Mosaic& mosaic, const std::string& out, const std::string& tag);
void printRegularity(const derivean::sim::Mosaic& mosaic, const std::string& out, const std::string& tag);

// MeasureEdges.cpp — where tiles and levels meet, and whether a tile's ledger adds up.
void printSeams(const derivean::sim::Mosaic& mosaic);
void printLevels(derivean::sim::World& world, const derivean::sim::Mosaic& child);
void printBudget(derivean::sim::World& world, const derivean::sim::Mosaic& mosaic);

// Measurements that need no place or build their own worlds.
int measurePlanes(const Arguments& arguments);        // MeasurePlane.cpp
int measureReferences(const Arguments& arguments);    // MeasurePlane.cpp
int measureConservation(const Arguments& arguments);  // MeasureConservation.cpp
int measureDeterminism(const Arguments& arguments);   // MeasureWorld.cpp
int measureHistory(const Arguments& arguments);       // MeasureHistory.cpp
int measureFuture(const Arguments& arguments);        // MeasureWorld.cpp
int measurePerformance(const Arguments& arguments);   // MeasurePerformance.cpp
int measureEarth(const Arguments& arguments);         // MeasureEarth.cpp
int measureFingerprint(const Arguments& arguments);   // MeasureFingerprint.cpp
int measureSphere(const Arguments& arguments);        // MeasureSphere.cpp
