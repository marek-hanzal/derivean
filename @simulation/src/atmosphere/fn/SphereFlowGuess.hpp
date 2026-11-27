#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SpherePoisson.hpp"
#include "geometry/type/SphereGrid.hpp"

namespace derivean::sim {

// The first guess of a step's ψ in flow.trialStream, from the accepted ψ levels and the candidate
// vorticity flow.trialVorticity (SphereFlowGuess.cpp); flow.product receives K ψₙ. Returns whether
// flow.trialProduct holds K of the guess (false when the guess is ψₙ itself).
bool guessStream(SphereFlow& flow, const SphereGrid& grid, const SpherePoisson& poisson);

} // namespace derivean::sim
