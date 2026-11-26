// The unforced flow over weeks: the flux form keeps the integral of ζ + f but neither energy nor
// enstrophy, and grid-scale vorticity grows slowly at the cube vertices. This bounds that drift at
// today's measured size, so a change that makes it worse shows; the closures of S4 must damp it.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"

#include <cstdint>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

struct Integrals {
    double enstrophy{}, energy{};
    double square[3]{};   // Σ ζ² per place: inside, face-edge band, touching a cube vertex
};

Integrals integralsOf(const SphereGrid& grid, const SphereFlow& flow) {
    Integrals integrals;
    for (int c = 0; c < grid.cells; ++c) {
        const double area = grid.area[size_t(c)], zeta = flow.vorticity[size_t(c)];
        integrals.enstrophy += area * zeta * zeta;
        integrals.energy -= 0.5 * area * zeta * flow.stream[size_t(c)];
        integrals.square[bandOf(grid, c)] += zeta * zeta;
    }
    return integrals;
}

} // namespace

namespace derivean::sim::atmosphere_sphere_test {

// Level 4, Earth's rotation, grid-scale noise of 1e-5 1/s, 30 days of 30-minute steps (Courant
// below 0.05). Measured: enstrophy +18 %, energy +6 %, the root-mean-square vorticity × 1.36 in the
// cells touching cube vertices, × 1.18 along face edges, × 1.02 inside the faces. At level 5 the
// vertices' growth is × 1.10 (and +0.8 % enstrophy), so it falls with resolution.
void checkDrift() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> noise(size_t(grid.cells));
    std::uint64_t state = 42;
    for (double& value : noise) {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        value = (double(state >> 11) / double(1ull << 53) - 0.5) * 2e-5;
    }
    expect(setVorticity(flow, grid, stencil, poisson, noise), "the noise did not invert", flow.inversion.residual);
    setStep(flow, 1800);
    const Integrals start = integralsOf(grid, flow);
    const double absolute = areaMean(grid, flow.vorticity) + areaMean(grid, flow.coriolis);
    bool converged = true;
    for (int s = 0; s < 30 * 48; ++s) {
        converged = stepSphereFlow(flow, grid, stencil, poisson) && converged;
    }
    const Integrals end = integralsOf(grid, flow);
    const double enstrophy = end.enstrophy / start.enstrophy, energy = end.energy / start.energy;
    double growth[3];
    for (int place = 0; place < 3; ++place) growth[place] = std::sqrt(end.square[place] / start.square[place]);
    expect(converged, "an inversion did not converge over 30 days");
    expect(std::abs(areaMean(grid, flow.vorticity) + areaMean(grid, flow.coriolis) - absolute) < 1e-18,
           "absolute vorticity is not kept", areaMean(grid, flow.vorticity));
    expect(std::isfinite(enstrophy) && enstrophy < 1.25 && std::abs(energy - 1) < 0.1, "unforced enstrophy or energy drift grew", enstrophy);
    expect(growth[2] < 1.5 && growth[1] < 1.25 && growth[0] < 1.05, "grid-scale vorticity growth at cube vertices grew", growth[2]);
}

} // namespace derivean::sim::atmosphere_sphere_test
