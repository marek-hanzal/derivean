#pragma once

#include "atmosphere/type/EddyBand.hpp"
#include "atmosphere/type/LatitudeBands.hpp"
#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SpherePoisson.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <string>
#include <vector>

// What a day's diagnosis of the sphere's flow reads besides the flow (MeasureSphereDiagnose.cpp):
// the bands its jets are read on, an eddy band and a Poisson of its own, so that diagnosing touches
// nothing the run steps with.
struct SphereProbe {
    derivean::sim::LatitudeBands bands;
    derivean::sim::EddyBand band;
    derivean::sim::SpherePoisson poisson;
    std::vector<double> mean, eddy, stream;
};

// One line of the flow's integrals, jets, eddies, winds and the seams' grid noise on day `day`.
void printSphereDay(SphereProbe& probe, const derivean::sim::SphereGrid& grid, const derivean::sim::SphereFlow& flow, double day);

// ζ and the fluxes to a file and back, and two such files compared (relative L2 and maximum).
void writeSphereSnapshot(const std::string& path, const derivean::sim::SphereFlow& flow);
int compareSphereSnapshots(const std::string& a, const std::string& b);
