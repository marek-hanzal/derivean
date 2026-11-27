#pragma once

#include "atmosphere/type/SpherePoisson.hpp"

#include <vector>

namespace derivean::sim {

// The barotropic (single-layer, non-divergent) flow on the weather grid: relative vorticity ζ is
// carried by the wind of its own stream function, Δψ = ζ, and the planet's rotation enters through
// the Coriolis parameter f = 2Ω sin φ, so the carried quantity is the absolute vorticity ζ + f
// (Charney, Fjørtoft & von Neumann 1950). Nothing consumes it yet; S4 adds the closures today's
// flat-world circulation has (relaxation, friction, eddy growth, damping) on top of this core, as a
// SphereForcing of the step.
//
// The state (vorticity, before, stream, flux, started, and the first guess's history: streamBack,
// streamBackTwo, productBack, productBackTwo, levels) is always one accepted step: a step or a new
// vorticity whose inversion does not converge leaves it as it was. Leapfrog and its filter need
// equally spaced time levels, so the step is part of the state: set it with setStep, which restarts
// the history when it changes; change vorticity and stream only through setVorticity, which restarts
// it too (`known` takes K ψₙ for granted). The hour takes a refused try back by these, the step,
// `known` with trialProduct and the last inversion's report (SphereFlowStart, fn/SphereHour.cpp's
// eachStateVector), so a field added to the state joins them there; the rest is work space or fixed.
struct SphereFlow {
    double rotation{};                 // Ω, 1/s
    std::vector<double> coriolis;      // f per cell, 1/s
    std::vector<double> vorticity;     // ζ now, 1/s
    std::vector<double> before;        // ζ one step back (leapfrog)
    std::vector<double> stream;        // ψ, m²/s, zero area mean
    // The accepted ψ one and two steps back and their K ψ, for the first guess of the next inversion;
    // `levels` counts the accepted ψ one step apart, stream included, the older ones with their K ψ
    // (1 to 3; 1 after a restart or a step without the projection). `product` is work space: K ψₙ from
    // the guess until the step is accepted. The K ψ are those of the Poisson the flow steps with and
    // CG's first residual is formed from them, so a flow keeps one Poisson (one K) from its start.
    std::vector<double> streamBack, streamBackTwo, productBack, productBackTwo, product;
    int levels{1};
    bool known{};                      // trialProduct holds K ψₙ, left by the inversion that gave ψₙ
    int projection{3};                 // accepted ψ the first guess combines, 0 to 3 (0: the last ψ as it is)
    std::vector<double> flux;          // per edge, ψ(from) − ψ(to): m²/s per metre of depth, out of left
    PoissonReport inversion;           // of the last attempted inversion, accepted or not
    double tolerance{1e-9};            // of each inversion, relative to the source
    int limit{60};                     // CG iterations per inversion
    double step{};                     // s, the spacing of the time levels; 0 until setStep
    bool started{};                    // false until the first step after a restart (a forward step)
    // Work space, one call at a time per flow: the candidate state of a step before it is accepted,
    // and the tendency's intermediates. atVertex is shared: tendencyOf fills it with the absolute
    // vorticity at the vertices and reads it, then invertTrial fills it with ψ for the stream flux.
    // trialProduct: K trialStream when the first guess formed it, K ψₙ while `known`.
    std::vector<double> trialVorticity, trialBefore, trialStream, trialFlux, trialProduct;
    std::vector<double> tendency, absolute, atVertex;
};

} // namespace derivean::sim
