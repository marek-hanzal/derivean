#pragma once

#include "measure/fn/ReliefStats.hpp"

#include <string>

// ReliefCommand.cpp — the report as text, and the ground coloured by how steep it is: pale under 5°,
// green to 15°, yellow to 30°, orange to 45°, red to 60°, violet above; water blue, streams dark blue.
void printRelief(const derivean::sim::Mosaic& mosaic, const derivean::sim::ReliefReport& report);
void writeSlopeMap(const derivean::sim::Mosaic& mosaic, const std::string& path);
