#pragma once

#include "atmosphere/type/SpherePoisson.hpp"

#include <vector>

namespace derivean::sim {

// The barotropic (single-layer, non-divergent) flow on the weather grid: relative vorticity ζ is
// carried by the wind of its own stream function, Δψ = ζ, and the planet's rotation enters through
// the Coriolis parameter f = 2Ω sin φ, so the carried quantity is the absolute vorticity ζ + f
// (Charney, Fjørtoft & von Neumann 1950). Nothing consumes it yet; S4 adds the closures today's
// flat-world circulation has (relaxation, friction, eddy growth, damping) on top of this core.
//
// The state (vorticity, before, stream, flux, started) is always one accepted step: a step or a new
// vorticity whose inversion does not converge leaves it as it was. Leapfrog and its filter need
// equally spaced time levels, so the step is part of the state: set it with setStep, which restarts
// the history when it changes; change vorticity only through setVorticity, which restarts it too.
struct SphereFlow {
    double rotation{};                 // Ω, 1/s
    std::vector<double> coriolis;      // f per cell, 1/s
    std::vector<double> vorticity;     // ζ now, 1/s
    std::vector<double> before;        // ζ one step back (leapfrog)
    std::vector<double> stream;        // ψ, m²/s, zero area mean
    std::vector<double> flux;          // per edge, ψ(from) − ψ(to): m²/s per metre of depth, out of left
    PoissonReport inversion;           // of the last attempted inversion, accepted or not
    double tolerance{1e-9};            // of each inversion, relative to the source
    int limit{60};                     // CG iterations per inversion
    double step{};                     // s, the spacing of the time levels; 0 until setStep
    bool started{};                    // false until the first step after a restart (a forward step)
    // Work space, one call at a time per flow: the candidate state of a step before it is accepted,
    // and the tendency's intermediates.
    std::vector<double> trialVorticity, trialBefore, trialStream, trialFlux;
    std::vector<double> tendency, absolute, atVertex;
};

} // namespace derivean::sim
