# Atmosphere on the sphere (planet slice S4a)

Owns the new files of `@simulation/src/atmosphere/` whose names start with `Sphere` or `EddyBand` (`type/`, `fn/`, `constant/SphereAir.hpp`) and `@simulation/test/AtmosphereSphere*.{cpp,hpp}`. It is the numerical core of a barotropic atmosphere on S1's cubed sphere (`docs/handoff/geometry.md`), built ahead of slice S4, which switches the climate and the atmosphere over (plan: `.out/lead/handover/PLANET.md`, "S0 review outcome" wins; Codex's requirements: `.out/review-mailbox/from-codex/DREV-CLAUDE-002-01.txt` Q4 and `-03.txt` "Grad/div pro S4"). It is additive: nothing calls it, and today's weather on the flat 128² grid (`docs/handoff/atmosphere.md`) is unchanged.

## State

Built and tested: `simulation-tests atmospheresphere` passes in about 7 s (release, macOS arm64). Timed on 6 × 128² and 6 × 256² (below). Not wired into the stepped atmosphere, the climate, the bridge or the engine.

## The grid

The grid is the geometry's since S3a (`geometry/fn/SphereGrid`, `docs/handoff/geometry.md`, S3a): `makeSphereGrid(level)` builds cells, areas, centres, neighbours, oriented edges (left, right, from, to, length, normal) and vertices with their cells at one level of S1's quadtree; the level is the resolution parameter (7 → 6 × 128² cells of 72 km, 8 → 6 × 256² of 36 km, √ of the mean area). The weather's own discretisation on top of it is `makeSphereStencil(grid)` (`fn/SphereStencil`, `type/SphereStencil`): per edge `diamond`, `across`, `along`, `chord`, `offset` and the upstream cells `behindLeft`/`behindRight`, per vertex the least-squares weights. Operators, the flow and the Poisson solver take both, `(grid, stencil, …)`; the Laplacian's `SphereMatrix` is `type/SphereMatrix`. The move kept every number of this slice bit for bit (`.out/s3a/probe`).

- **Cells** are numbered `(face · side + j) · side + i` (`indexOf`, `cellAt`); a field is a `std::vector<double>` per cell (cell means, `CellField` in `fn/SphereOperators`); sin φ for f is the centre's z.
- **Edges**: one `SphereEdge` (geometry) per pair of neighbours (12 n²), `left` the lower index, the normal out of left. A flux is evaluated once per edge and applied to left and right with opposite signs, so face seams and cube vertices need no code of their own.
- **Vertices** (6 n² + 2, eight of them with three cells): the cells meeting there (geometry) with least-squares weights (`SphereStencil::weight`) that reproduce linear fields in the gnomonic tangent plane at the vertex. A plain mean is not enough: on a face edge the two faces' grid lines meet the edge obliquely (60° at a cube vertex), so the four centres' mean sits O(h) from the vertex, a quarter of a cell next to a cube vertex; with it the edge gradient had an O(1) error.
- **Edge gradient** (`gradientOf`): from the difference across the edge (right − left) and along it (to − from, vertex values), through the dual basis of the two chords in the tangent plane at the edge's midpoint (`across`, `along`). The chord between the two centres is not the normal (23° off at face 0, level 5, cell (29, 29), Codex's case; Putman & Lin 2007), and `across` carries that non-orthogonal term on every edge.
- **Diamonds**: the two geodesic triangles (left centre, from, to) and (right centre, to, from); they tile the sphere (tested to 1e-12).

## Operators (`fn/SphereOperators`)

- `divergenceOf(flux)`: Σ ± flux / area. Σ area · divergence over the closed grid cancels edge by edge (tested to 1e-13 of Σ |flux|).
- `streamFluxOf(ψ)`: the volume flux of the non-divergent wind v = up × ∇ψ through each edge, ψ(from) − ψ(to) at the edge's vertices. Round a cell these telescope: the wind's divergence is zero to rounding in every cell, and a uniform tracer stays uniform.
- `curlOf(edge vectors)`: circulation (vector · chord) over the area; curl(gradient) is zero to rounding.
- `makeLaplacian`: the stiffness K = Σ_edges diamond · Gᵀ G with G the edge gradient, Δψ = −Kψ / area (`laplacianOf`). It is the energy ½ Σ diamond |∇ψ|² of the discrete duality finite-volume method (Hermeline 2000; Domelevo & Omnes 2005) with the vertex unknowns replaced by their least-squares cell values, so it is cell-centred. Symmetric bit for bit by construction (rows add the same products in edge order), positive on every non-constant field, zero on constants (each G row sums to zero), 21 entries per row. Its Rayleigh quotient on Y₁ is 2/R² to 0.1 % at level 5.

A flux-form alternative (`divergenceOf(normalFluxOf(gradientOf(ψ)))`) is also second order inside the faces but not symmetric, and its local error at face edges is larger (0.23 against 0.15 of the value for Y₄), so the symmetric K is the Laplacian.

### Errors

Largest errors, relative to the field's amplitude, by distance from the nearest face edge (cells touching a cube vertex are in the first column). Y₁ is the degree-1 harmonic √3 sin φ, Y₄ is degree 4, order 2.

| Laplacian K, local error | Y₁: on the edge | one cell in | ≥ 2 cells in | Y₄: on the edge | one cell in | ≥ 2 cells in |
| --- | --- | --- | --- | --- | --- | --- |
| level 5 (6 × 32²) | 0.106 | 0.019 | 1.7e-3 | 0.145 | 0.048 | 1.7e-2 |
| level 6 (6 × 64²) | 0.100 | 0.020 | 4.0e-4 | 0.150 | 0.052 | 4.3e-3 |
| level 7 (6 × 128²) | 0.096 | 0.020 | 9.6e-5 | 0.152 | 0.054 | 1.1e-3 |
| level 8 (6 × 256²) | 0.094 | 0.021 | 2.4e-5 | 0.153 | 0.055 | 2.6e-4 |

Inside the faces the operator is second order. In the two-cell band along the face edges its local (truncation) error stays at about a tenth of the value: there the edge gradient is first order (below), because the chord between the two centres misses the edge's midpoint by O(h) along the edge. That error does not reach the solution of the Poisson problem, which is second order everywhere, the cube vertices included (next section): the band it lives in narrows with the cells (the supraconvergence known for such schemes). Gradient of z at the edge midpoints, relative to |∇z|: 9.0e-4, 2.3e-4, 5.6e-5, 1.4e-5 inside the faces and 1.9e-3, 9.6e-4, 4.8e-4, 2.4e-4 on face edges at levels 5 to 8.

## Poisson (`fn/SpherePoisson`, `fn/SphereMultigrid`)

`solveSpherePoisson(poisson, grid, source, result, tolerance, limit)` solves Δψ = ζ by conjugate gradients (Hestenes & Stiefel 1952) on K ψ = −area · (ζ − its mean). K is symmetric and positive on the subspace without the constant, the source's area mean is the one part no field on a closed sphere has as a Laplacian: it is removed and reported (`PoissonReport::sourceMean`), every residual stays in that subspace, and the result's area mean is set to zero. The report carries iterations, the final relative residual ‖Kψ − b‖/‖b‖ from a fresh product (CG's recursive residual falls on to 1e-22 while the true one stops near 1e-13, so without it a tolerance below that would report success) and `converged`; a solve that misses its tolerance says so and keeps its best iterate. `result` on entry is the first guess.

The preconditioner is one multigrid V-cycle (Brandt 1977) over the coarser quadtree levels down to level 1: the same Laplacian rediscretised on each level, symmetric Gauss–Seidel (two forward sweeps before, two backward after), restriction by summing the four children (K is an integrated operator, so residuals add), prolongation by the parent's value, plain CG to rounding on the 24 cells of level 1. With the restriction the transpose of the prolongation and the sweeps mirrored, the cycle is symmetric and positive, as preconditioned CG needs (tested; `smoothing` must be at least 1, asserted). `makeSpherePoisson(grid, false)` uses the diagonal instead. One or three sweeps cost the same or more than two (14 / 8 / 7 iterations at level 7).

Iterations to 1e-10 on Y₁: diagonal 40, 79, 159, 321, 648 at levels 4 to 8 (twice per level, as CG on a Laplacian does); multigrid 8, 8, 9, 10, 10.

Solution error, largest over the cells, relative to the amplitude (inside a face / touching a face edge / at a cube vertex):

| level | Y₁ | Y₄ |
| --- | --- | --- |
| 5 | 1.9e-3 / 1.5e-3 / 1.7e-3 | 1.6e-2 / 1.4e-2 / 2.5e-3 |
| 6 | 4.8e-4 / 4.1e-4 / 4.6e-4 | 4.0e-3 / 3.5e-3 / 7.2e-4 |
| 7 | 1.2e-4 / 1.1e-4 / 1.2e-4 | 9.9e-4 / 8.5e-4 / 1.9e-4 |
| 8 | 3.0e-5 / 3.0e-5 / 3.3e-5 | 2.5e-4 / 2.1e-4 / 5.0e-5 |

No library is used for CG. Eigen (header-only) has a CG that takes a custom preconditioner, but the work here is the operator, the null mode and the multigrid on the quadtree, which are ours either way; the CG loop is 30 lines, and an `add_requires` would reach `@napi` and `@engine` for that. If S4 wants sparse direct or AMG solvers, Eigen or a similar library is the place to look, announced to the lead first.

## The barotropic flow (`fn/SphereFlow`)

`makeSphereFlow(grid, Ω)`, `setVorticity`, `stepSphereFlow(flow, grid, poisson, step)`: ∂ζ/∂t = −∇·(v (ζ + f)) with f = 2Ω sin φ (Charney, Fjørtoft & von Neumann 1950). `rotationRateAt(age)` = 2π / `deepTimeAt(age).siderealDay` from S2 (7.2921e-5 1/s today, tested).

- Flux form: each edge carries its volume flux (`streamFluxOf`) times the absolute vorticity at the edge's midpoint, so the area integral of ζ + f is kept to rounding (tested to 1e-14, after a day of the Rossby–Haurwitz wave: −3e-17) and that of ζ stays zero.
- The edge value is the mean of the two cells moved to the edge's midpoint by the edge gradient (`EdgeStencil::offset`). With the plain mean a steady state drifted fifty times faster, at first order: a solid rotation over cube vertices changed by 7.5e-2, 4.1e-2, 1.9e-2 of its vorticity in a day at levels 4 to 6; with the move 5.7e-3, 1.4e-3, 3.9e-4. The plain mean would keep the enstrophy exactly before time discretisation; with the move it is not kept: in the Rossby–Haurwitz wave it falls by 6.5e-4, 1.8e-4, 4.5e-5 a day at levels 4 to 6. Neither energy nor enstrophy is conserved (today's Arakawa Jacobian keeps both), and unforced grid-scale vorticity grows slowly where the metric is worst: in 30 days at level 4 from noise, enstrophy +18 %, energy +6 %, root-mean-square vorticity × 1.36 in the cells touching cube vertices, × 1.18 along face edges, × 1.02 inside the faces; at level 5 × 1.10 at the vertices and +0.8 % enstrophy (`test/AtmosphereSphereDrift.cpp` bounds it at these sizes).
- Leapfrog with the Robert–Asselin–Williams filter (Robert 1966, Asselin 1972, Williams 2009) at the flat world's 0.1 and 0.53; the first step is forward. After each step ψ is found with the last one as the first guess (`SphereFlow::tolerance` 1e-9, `limit` 60; the report is `flow.inversion`).
- `advectTracer`: a passive tracer in flux form, upwind plus a van Leer-limited (van Leer 1974) Lax–Wendroff correction along the grid line (Sweby 1984); the upstream cell is across the opposite edge of the upwind cell (`behindLeft`, `behindRight`), on face seams too.

Tests (`test/AtmosphereSphereFlow.cpp`):
- solid rotation about an axis at right angles to a cube diagonal (its fastest circle runs through four cube vertices, two face centres and two face-edge midpoints): edge fluxes against the exact ψ(from) − ψ(to) are off by 0.4e-3, 1.4e-3 and 2.6e-3 of the largest flux at level 6 inside, on face edges and at cube vertices, falling at first order (the solution's second-order error bends at face edges); the steady state as above;
- the Rossby–Haurwitz wave of wavenumber 4 (Haurwitz 1940; Williamson et al. 1992 case 6, ω = K = 7.848e-6 1/s) with Earth's Ω turns east by 12.2° a day; after a day the stepped flow is off by 0.125, 0.033, 0.0085 of the wave's amplitude at levels 4 to 6 (second order) while the pattern has moved by 0.74;
- a cosine bell (Williamson case 1) carried once round in 12 days over the four cube vertices: mass kept to 1e-14, nothing below zero or above the starting peak, a uniform tracer uniform to 2e-16; l1 error 1.07, 0.57, 0.35 at levels 4 to 6 (the scheme is upwind across the grid lines, so the bell spreads where the wind crosses them at an angle).

## The eddy band (`fn/EddyBand`, `constant/SphereAir.hpp`)

**What today's filter does.** `shapeCirculation` (`atmosphere/fn/Circulation.cpp`) fills `PoissonSolver::pass` with a ring in wavenumber: gain max(0, 1 − ((|k| − 7) / 3.5)²) at |k| = hypot(x, z) cycles across `PlanetSpan` (16 777 216 m), for every wave with x ≥ 1. The waves that do not vary along X (x = 0, the zonal bands) are left out: the belts own them. Every `EddyEvery` = 4 wind steps (2 h) `refreshWind` asks `solvePoisson` for the vorticity filtered through it as `eddy`, and `stepCirculation` adds `growth(row) · eddy` to the tendency: vorticity of weather-system size (fastest at 2 397 km, the band from 4 793 to 1 598 km) grows at the rate the season's temperature gradient sets for its latitude (`EddyGrowth` × `storminess`) until the eddies' root-mean-square vorticity reaches `EddyLimit` (1.5e-5 1/s). Why: a single layer cannot be baroclinically unstable, so this closure stands in for the conversion of temperature contrast into weather systems (`docs/handoff/atmosphere.md`, §5).

**On the sphere.** The Laplacian's eigenfunctions are the spherical harmonics, with wavenumber √(l(l + 1)) / R, so the ring becomes a gain per degree l at the same physical wavenumber, applied to every order m ≠ 0 (m = 0, the zonal harmonics about Earth's axis, are the sphere's zonal bands). `SphereEddyWavelength` = `PlanetSpan` / 7 and `SphereEddyReach` = 0.5 reproduce today's ring in metres: degrees 8 to 24, peak 1.00 at 16, gain 0.03 at 8 and 0.13 at 24 (the gain and the band's bounds are tested against the flat formula for degrees 0 to 40, for these constants and for 4 000 km). `filterEddyBand` projects the field on the harmonics of the band by the grid's area quadrature (4π-normalised real harmonics, Holmes & Featherstone 2002 recursions, cells in blocks of 256) and synthesises gain · coefficient · Y. It is second order: a harmonic of the band comes back within 9.1e-3 at level 5 and 2.2e-3 at level 6, and harmonics outside the band (zonal, degree 1 to 7, 25 to 40) leak by as much. Constants give zero.

**Why this choice.** Spherical harmonics are what the flat FFT's waves are on a sphere: the same isotropic scale and the same exclusion of zonal waves, with nothing tuned anew. A filter built from the Laplacian (differences of Helmholtz solves, or a Chebyshev polynomial in K) would avoid the transform but cannot leave out m = 0 on its own and needs a polynomial of degree about 250 or tens of CG solves to resolve degree 8 against the grid's highest; the harmonics' cost is 55 ms at 6 × 128² and 220 ms at 6 × 256² single-threaded every 2 simulated hours, no tables (they would take 0.9 GB at 256²). **Open for S4: which band.** The constants carry today's metres so the switch changes the geometry, not the closure, but the flat world defines the band in cycles round its planet (`EddyWave`, 7), and that is not Earth's band either. Three readings on Earth's radius: today's metres, 2 397 km, degree 16, zonal wavenumber 12 at 45°; the flat world's 7 cycles round the planet, 5 700 km, degree 6.5, zonal wavenumber 5 at 45°; Earth's fastest-growing baroclinic waves, about 4 000 km (about four deformation radii of 1 000 km), degree 10, zonal wavenumber 7 at 45°. Earth is the reference, which points at the third; the choice changes the closure, so the lead or the owner makes it. `makeEddyBand` takes any wavelength.

## Timing

`.out/s4a/bench/Bench.cpp` against the private release library, single thread, wall clock, best of three, Apple M3 Max under a load average of 8 to 10 (other streams building). Output in `.out/s4a/timing.txt`.

| | 6 × 128² (level 7) | 6 × 256² (level 8) |
| --- | --- | --- |
| cells / edges | 98 304 / 196 608 | 393 216 / 786 432 |
| √ mean area / shortest edge | 72.0 / 55.3 km | 36.0 / 27.6 km |
| grid tables / Laplacian and coarser levels built in | 44 / 74 ms | 152 / 301 ms |
| memory: grid / matrices | 47 / 35 MB | 189 / 138 MB |
| Poisson, cold, to 1e-6 / 1e-9 / 1e-12, multigrid CG | 6 / 8 / 11 iterations, 80 / 104 / 138 ms | 6 / 9 / 11, 333 / 475 / 570 ms |
| the same with the diagonal preconditioner | 183 / 230 / 269 iterations, 204 / 257 / 300 ms | 374 / 464 / 543, 2.1 / 2.6 / 3.1 s |
| one barotropic step, warm start, tolerance 1e-9 / 1e-6 | 90 / 64 ms (6 / 4 iterations) | 367 / 270 ms (6 / 4) |
| eddy band filter | 55 ms | 220 ms |
| tracer step | 0.57 ms | 2.4–4.1 ms |
| step at outflow Courant 0.4, 100 m/s wind (Rossby–Haurwitz) | 253 s, 14 steps an hour | 126 s, 29 steps an hour |

The Poisson solve is nine tenths of a step. For Q3: today's whole atmosphere takes 5–6 ms per simulated hour on one core with two 30-minute wind steps; the barotropic core alone on the sphere takes about 0.9–1.3 s per simulated hour at 6 × 128² and 7.7–10.5 s at 6 × 256² on one core at that wind, because the explicit step is held to the cell size (cells of 55–131 km, not 131 km, and winds up to 100 m/s). An explicit step's cost goes as h⁻³: the cells go as h⁻², the steps per hour as h⁻¹. Where the cost can come down, none of it done: the Gauss–Seidel smoother is sequential (a coloured or Chebyshev–Jacobi smoother would run on `Workers`, as would the matrix product and the band's blocks); tolerance 1e-6 instead of 1e-9 saves a quarter; a semi-Lagrangian or semi-implicit step would lift the Courant limit; the band could be refreshed less often.

## Contract for the switch (S4)

- `makeSphereGrid(level)` and `makeSphereStencil(grid)` once per world; the level is the resolution choice (Q3) and nothing assumes one. Fields are per cell in the grid's order; the centre's z (sin φ) replaces `vertexLatitude`, and rows (`AirRows`) disappear: what today is per row (`season.jet`, `growth`, `drift`, the eddies' envelope) becomes a function of latitude per cell, or a zonal mean, which this slice does not provide (the m = 0 harmonics of `fn/SphereHarmonics` or area-weighted latitude bins would).
- `solvePoisson(..., &eddy)` becomes `solveSpherePoisson` plus `filterEddyBand` every `EddyEvery` wind steps.
- `stepCirculation`'s Jacobian becomes `stepSphereFlow`; S4 adds on top what today's step has besides advection: relaxation of the zonal mean, implicit friction (`dragKeep`), `growth · eddy`, biharmonic damping, the terrain's squeeze of the column. Check after each step that `flow.inversion.converged` holds.
- Damping must absorb the scheme's own grid-scale source at the cube vertices (The barotropic flow; `test/AtmosphereSphereDrift.cpp`), or the seams' edge gradient must become second order first. For the biharmonic term K A⁻¹ K (A the cell areas) is positive semi-definite and so dissipates in the area-weighted norm: safe as grid-scale damping with a coefficient that goes as h⁴. It is not a consistent ∇⁴: on Y₄ its error is 0.56 of the value inside the faces and 3.7 on face edges at level 5, growing fourfold per level, since K's local error at the seams is amplified by the second application. Neither it nor K / area may be read pointwise as a physical quantity (diagnostics, a curvature someone consumes); only solutions of K are second order.
- `SphereFlow`, `SpherePoisson` and `EddyBand` hold their work space: one call at a time per object. Spreading the work over `Workers` needs one object per thread or the buffers moved out.
- Winds for transport are the edge fluxes (`flow.flux`, m²/s per metre of depth); the carried fields use `advectTracer` or a better scheme. Cell-centre wind vectors (for `weatherAt`, the clouds, the renderer) need a reconstruction from the edge fluxes, not provided yet.
- The step is Courant-limited: choose the number of wind steps per hour from `outflowCourant(grid, flux, step)` ≤ 0.4.
- Ω from `rotationRateAt(age)` (S2's deep time), never `Coriolis` from `constant/Air.hpp`.

## Verify

```
cd @simulation && X=$(ls -d ~/.local/share/mise/installs/github-xmake-io-xmake/*/ | head -1)xmake && export XMAKE_CONFIGDIR=$PWD/../.out/s4a/xmake && $X config --yes --mode=release --builddir=../.out/s4a/build && $X build --yes simulation-tests
../.out/s4a/build/macosx/arm64/release/simulation-tests atmospheresphere
```

(`simulation-tests atmosphere` also runs today's slow `testAtmosphere`.) The timing: `clang++ -std=c++20 -O3 -I@simulation/src .out/s4a/bench/Bench.cpp .out/s4a/build/macosx/arm64/release/libderivean-simulation.a -o .out/s4a/bench/bench && .out/s4a/bench/bench` from the root.

`test/AtmosphereSphere.cpp`: grid counts, areas and diamonds tiling the sphere, vertex weights, the eight cube vertices; K symmetric bit for bit, constants in its null space, positive on random fields, the Y₁ Rayleigh quotient; divergence summing to zero, no divergence of stream fluxes, no curl of gradients; edge gradient and Laplacian on levels 5 to 7 by place. `AtmosphereSpherePoisson.cpp`: second-order solutions for Y₁ and Y₄ in every place on levels 5 to 7, multigrid iterations flat; a constant in the source reported and without effect, zero-mean result, no step from the answer, zero for zero; unconverged solves reported (also below the true residual's floor), diagonal slower, the V-cycle symmetric and positive. `AtmosphereSphereFlow.cpp`, `AtmosphereSphereDrift.cpp` and `AtmosphereSphereBand.cpp`: as above.

## Open

- The Laplacian's local error at face edges (about 0.1) and the first-order edge gradient and stream fluxes there: the solution is second order, but a wind derived from ψ near seams is first order (2.6e-3 of the largest flux at cube vertices, level 6), and the same error feeds the slow growth of grid-scale vorticity at the cube vertices. A quadratic reconstruction of the edge gradient would make it second order; whether the imprint matters is S4's to see in the weather.
- The tracer scheme diffuses across grid lines (l1 0.35 at level 6 for the cosine bell); humidity and heat in S4 may want FV3's dimensionally split PPM transport (Putman & Lin 2007).
- Cost (Timing): single-threaded, Poisson-bound, Courant-limited. Memory 330 MB at 6 × 256² (an edge is 48 bytes of geometry and 112 of `EdgeStencil`; chord and offset could be dropped or kept in floats), with about 150 MB more for a moment while `makeLaplacian` gathers its rows.
- The eddy band's centre and width are today's metres, not Earth's (The eddy band, open for S4).
- Williamson et al. cases 2 and 5 need the shallow-water equations (a height field), beyond a barotropic core.
- Linux, debug builds and `-ffp-contract=off` are not checked; the bit-for-bit symmetry of K does not depend on contraction (the same expression is evaluated for both entries).

## Sources

All listed in `docs/credits/atmosphere.md` ("The atmosphere on the sphere"):
- Charney, Fjørtoft & von Neumann (1950), the barotropic vorticity equation;
- Hermeline (2000), Domelevo & Omnes (2005), the discrete duality finite-volume Laplacian;
- Putman & Lin (2007), the non-orthogonality of gnomonic grids (also in `docs/credits/astronomy-geometry.md`);
- Hestenes & Stiefel (1952), conjugate gradients; Brandt (1977), multigrid;
- Holmes & Featherstone (2002), the Legendre recursions;
- Robert (1966), Asselin (1972), Williams (2009), the leapfrog filter (already credited for the flat world);
- van Leer (1974), Sweby (1984), the limited tracer flux;
- Neumaier (1974), the compensated area sums;
- Haurwitz (1940), the Rossby–Haurwitz wave; Williamson et al. (1992), the test cases.
