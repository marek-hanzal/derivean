#pragma once

#include "atmosphere/type/SpherePoisson.hpp"

#include <vector>

namespace derivean::sim {

// The barotropic (single-layer, non-divergent) flow on the weather grid: relative vorticity ζ is
// carried by the wind of its own stream function, Δψ = ζ, and the planet's rotation enters through
// the Coriolis parameter f = 2Ω sin φ, so the carried quantity is the absolute vorticity ζ + f
// (Charney, Fjørtoft & von Neumann 1950). Nothing consumes it yet; S4 adds the closures today's
// flat-world circulation has (relaxation, friction, eddy growth, damping) on top of this core.
struct SphereFlow {
    double rotation{};                 // Ω, 1/s
    std::vector<double> coriolis;      // f per cell, 1/s
    std::vector<double> vorticity;     // ζ now, 1/s
    std::vector<double> before;        // ζ one step back (leapfrog)
    std::vector<double> stream;        // ψ, m²/s, zero area mean
    std::vector<double> flux;          // per edge, ψ(from) − ψ(to): m²/s per metre of depth, out of left
    PoissonReport inversion;           // of the current stream function
    double tolerance{1e-9};            // of each inversion, relative to the source
    int limit{60};                     // CG iterations per inversion
    bool started{};                    // false until the first step (which is a forward step)
    std::vector<double> tendency, next, absolute, atVertex;   // work space: one step at a time per flow
};

} // namespace derivean::sim
