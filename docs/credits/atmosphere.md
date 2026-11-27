# Credits: atmosphere, haze, clouds and snow

Part of [Credits](../credits.md); the entry format is explained there. The sky's light as drawn is under [rendering](rendering.md).

## The stepped atmosphere

### Arakawa 1966: the energy-conserving Jacobian
Arakawa, A. (1966). Computational design for long-term numerical integration of the equations of fluid motion: two-dimensional incompressible flow. Part I. *Journal of Computational Physics* 1(1), 119–143. doi:10.1016/0021-9991(66)90015-5
- **Model:** advection of vorticity by a Jacobian that keeps energy and enstrophy from piling up at the grid scale.
- **Used in:** `@simulation/src/atmosphere/fn/Circulation.cpp:62` (cited by name only).

### Robert 1966, Asselin 1972 and Williams 2009: the leapfrog filter
Robert, A. J. (1966). The integration of a low order spectral form of the primitive meteorological equations. *Journal of the Meteorological Society of Japan, Ser. II* 44(5), 237–245. doi:10.2151/jmsj1965.44.5_237

Asselin, R. (1972). Frequency filter for time integrations. *Monthly Weather Review* 100(6), 487–490. doi:10.1175/1520-0493(1972)100<0487:FFFTI>2.3.CO;2

Williams, P. D. (2009). A proposed modification to the Robert–Asselin time filter. *Monthly Weather Review* 137(8), 2538–2546. doi:10.1175/2009MWR2724.1
- **Model:** the Robert–Asselin–Williams filter damping the computational mode of the leapfrog step, strength 0.1.
- **Used in:** `@simulation/src/atmosphere/fn/Circulation.cpp:15` (cited by name only); on the sphere `@simulation/src/atmosphere/fn/SphereFlow.cpp:17`, `SphereFlow.hpp:36`; `docs/handoff/atmosphere-sphere.md:86`.
- **Checked by:** `@simulation/test/AtmosphereSphereStep.cpp:78` (a window ending in a shorter step, with the restart, against equal steps).
- **Limits:** the leapfrog and the filter take the three time levels as equally spaced; on the sphere the step is part of the flow's state and a change of it restarts with a forward step (`@simulation/src/atmosphere/fn/SphereFlow.hpp:24`).

### Alduchov and Eskridge 1996: the Magnus formula
Alduchov, O. A., Eskridge, R. E. (1996). Improved Magnus form approximation of saturation vapor pressure. *Journal of Applied Meteorology* 35(4), 601–609. doi:10.1175/1520-0450(1996)035<0601:IMFAOS>2.0.CO;2
- **Model:** saturation vapour density by the Magnus form, tabulated at compile time.
- **Used in:** `@simulation/src/climate/fn/Moisture.hpp:10`, `:35`. The code names only "the Magnus formula"; its coefficients (610.94 Pa, 17.625, 243.04 °C) are this paper's.

### WMO International Cloud Atlas: the ten genera
World Meteorological Organization. *International Cloud Atlas: Manual on the Observation of Clouds and Other Meteors* (WMO-No. 407). WMO, Geneva; online edition 2017 onwards. https://cloudatlas.wmo.int/ (unverified: edition year)
- **Model:** the ten cloud genera and their typical base and top heights.
- **Used in:** `@simulation/src/atmosphere/type/Cloud.hpp:12`, `@game/src/clouds/fn/findWeather.ts:5`, `@game/contract/GameBridge.ts:267`; `docs/handoff/brief/clouds.md:21`.
- **Checked by:** `@simulation/test/Clouds.cpp:70`, one scene per genus.

## Light through clean air

### Bucholtz 1995: Rayleigh extinction of clean air
Bucholtz, A. (1995). Rayleigh-scattering calculations for the terrestrial atmosphere. *Applied Optics* 34(15), 2765. doi:10.1364/AO.34.002765
- **Model:** 1.16 × 10⁻⁵ /m at 550 nm at sea level.
- **Used in:** `@simulation/src/atmosphere/constant/Light.hpp:13`; through `@napi/src/addon.cpp:131` into the renderer.
- **Checked by:** `@simulation/test/Haze.cpp:148`, clean air sees 337 km.
- **Approximations:** the dispersion of the refractive index (about +0.05 on the exponent) is left out (`Light.hpp:15`).

### Rayleigh 1871: the inverse fourth power
Strutt, J. W. (Lord Rayleigh) (1871). On the light from the sky, its polarization and colour. *Philosophical Magazine*, 4th series, 41. Reprinted in *Scientific Papers* vol. 1, Cambridge University Press, 87–103, doi:10.1017/CBO9780511703966.009. (unverified: Philosophical Magazine pages)
- **Model:** scattering by clean air as λ⁻⁴.
- **Used in:** `@simulation/src/atmosphere/constant/Light.hpp:14`.

### Mie 1908: scattering by particles
Mie, G. (1908). Beiträge zur Optik trüber Medien, speziell kolloidaler Metallösungen. *Annalen der Physik* 330(3), 377–445. doi:10.1002/andp.19083300302
- **Model:** mass extinction, spectrum and single-scattering albedo of each aerosol kind, computed offline and stored as constants.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:11`, `:14`, `:16`.
- **Note:** the Mie code or tables the constants were computed with are not recorded ([gaps](../credits.md#gaps)).

## Haze: optics, growth and removal

### Petters and Kreidenweis 2007: κ hygroscopicity
Petters, M. D., Kreidenweis, S. M. (2007). A single parameter representation of hygroscopic growth and cloud condensation nucleus activity. *Atmospheric Chemistry and Physics* 7(8), 1961–1971. doi:10.5194/acp-7-1961-2007
- **Model:** a particle at relative humidity h holds 1 + κ·h/(1 − h) times its dry volume.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:12`; `docs/handoff/atmosphere.md:19`.

### Köhler 1936: droplet activation
Köhler, H. (1936). The nucleus in and the growth of hygroscopic droplets. *Transactions of the Faraday Society* 32, 1152–1161. doi:10.1039/TF9363201152
- **Model:** every haze kind is large enough to activate at a cloud's supersaturation (κ-Köhler), so it rains out with the low band's water.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:44` (cited by name only).

### Koehler et al. 2009: κ of mineral dust
Koehler, K. A., Kreidenweis, S. M., DeMott, P. J., et al. (2009). Hygroscopicity and cloud droplet activation of mineral dust aerosol. *Geophysical Research Letters* 36(8). doi:10.1029/2009GL037348
- **Model:** κ = 0.03 for dust.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:32`.

### Zieger et al. 2017: κ of sea salt
Zieger, P., Väisänen, O., Corbin, J. C., et al. (2017). Revising the hygroscopicity of inorganic sea salt particles. *Nature Communications* 8, 15883. doi:10.1038/ncomms15883
- **Model:** κ = 1.1 for sea salt.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:34`.

### Dubovik et al. 2002: absorption of Saharan dust
Dubovik, O., Holben, B., Eck, T. F., et al. (2002). Variability of absorption and optical properties of key aerosol types observed in worldwide locations. *Journal of the Atmospheric Sciences* 59(3), 590–608. doi:10.1175/1520-0469(2002)059<0590:VOAAOP>2.0.CO;2
- **Model:** refractive index of dust, 1.53 + 0.0010i to 0.0025i by colour.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:19`; `docs/handoff/atmosphere.md:19`.

### Stokes 1851: settling
Stokes, G. G. (1851). On the effect of the internal friction of fluids on the motion of pendulums. *Transactions of the Cambridge Philosophical Society* 9(2), 8–106.
- **Model:** fall speed of a swollen particle in air.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:47` (cited by name only).

### Wesely et al. 1985: dry deposition
Wesely, M. L., Cook, D. R., Hart, R. L., Speer, R. E. (1985). Measurements and parameterization of particulate sulfur dry deposition over grass. *Journal of Geophysical Research* 90(D1), 2131–2143. doi:10.1029/JD090iD01p02131
- **Model:** deposition velocity of 0.002 u* for accumulation-mode sulphate.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:48`.

## Haze: sources

### Fécan, Marticorena and Bergametti 1999: moisture and the dust threshold
Fécan, F., Marticorena, B., Bergametti, G. (1999). Parametrization of the increase of the aeolian erosion threshold wind friction velocity due to soil moisture for arid and semi-arid areas. *Annales Geophysicae* 17(1), 149–157. doi:10.1007/s00585-999-0149-7
- **Model:** the threshold friction velocity rises with the gravimetric moisture of a sand without clay.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:19`, `:104`; `docs/handoff/atmosphere.md:19`.

### Gong 2003: sea spray
Gong, S. L. (2003). A parameterization of sea-salt aerosol source function for sub- and super-micron particles. *Global Biogeochemical Cycles* 17(4). doi:10.1029/2003GB002079
- **Model:** spray flux integrated over wet radii up to 4 µm at 80 % humidity, as U^3.41.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:24`, `@simulation/src/atmosphere/fn/Haze.cpp:34`.

### Nightingale et al. 2000 and Saltzman et al. 1993: dimethyl sulphide from the sea
Nightingale, P. D., Malin, G., Law, C. S., et al. (2000). In situ evaluation of air-sea gas exchange parameterizations using novel conservative and volatile tracers. *Global Biogeochemical Cycles* 14(1), 373–387. doi:10.1029/1999GB900091

Saltzman, E. S., King, D. B., Holmen, K., Leck, C. (1993). Experimental determination of the diffusion coefficient of dimethylsulfide in water. *Journal of Geophysical Research* 98(C9), 16481–16486. doi:10.1029/93JC01858
- **Model:** the transfer velocity against wind, scaled to the Schmidt number of DMS in sea water.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:30`, `:31`, `:110`.

### Lana et al. 2011: DMS in the sea
Lana, A., Bell, T. G., Simó, R., et al. (2011). An updated climatology of surface dimethlysulfide concentrations and emission fluxes in the global ocean. *Global Biogeochemical Cycles* 25(1). doi:10.1029/2010GB003850 (the title's spelling is the journal's)
- **Model:** Earth's mean surface concentration, 2.3 nM, until the sea has life of its own.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:32`.

### Chin et al. 1996: sulphur to sulphate
Chin, M., Jacob, D. J., Gardner, G. M., et al. (1996). A global three-dimensional model of tropospheric sulfate. *Journal of Geophysical Research* 101(D13), 18667–18690. doi:10.1029/96JD01221
- **Model:** about 0.4 of DMS sulphur ends as sulphate aerosol.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:34`.

### Guenther et al. 1993, 1995 and 2006: plant emissions
Guenther, A. B., Zimmerman, P. R., Harley, P. C., et al. (1993). Isoprene and monoterpene emission rate variability: model evaluations and sensitivity analyses. *Journal of Geophysical Research* 98(D7), 12609–12617. doi:10.1029/93JD00527

Guenther, A., Hewitt, C. N., Erickson, D., et al. (1995). A global model of natural volatile organic compound emissions. *Journal of Geophysical Research* 100(D5), 8873–8892. doi:10.1029/94JD02950

Guenther, A., Karl, T., Harley, P., et al. (2006). Estimates of global terrestrial isoprene emissions using MEGAN (Model of Emissions of Gases and Aerosols from Nature). *Atmospheric Chemistry and Physics* 6(11), 3181–3210. doi:10.5194/acp-6-3181-2006
- **Model:** isoprene against leaf temperature and light (1993), monoterpenes always (1995), canopy emission factors of a mixed cover at 30 °C and full light (MEGAN, 2006).
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:40`, `:41`, `:118`.

### Kroll et al. 2006, Griffin et al. 1999 and Hallquist et al. 2009: secondary organic aerosol
Kroll, J. H., Ng, N. L., Murphy, S. M., et al. (2006). Secondary organic aerosol formation from isoprene photooxidation. *Environmental Science & Technology* 40(6), 1869–1877. doi:10.1021/es0524301

Griffin, R. J., Cocker, D. R., Flagan, R. C., Seinfeld, J. H. (1999). Organic aerosol formation from the oxidation of biogenic hydrocarbons. *Journal of Geophysical Research* 104(D3), 3555–3567. doi:10.1029/1998JD100049

Hallquist, M., Wenger, J. C., Baltensperger, U., et al. (2009). The formation, properties and impact of secondary organic aerosol: current and emerging issues. *Atmospheric Chemistry and Physics* 9(14), 5155–5236. doi:10.5194/acp-9-5155-2009
- **Model:** yields of about 3 % for isoprene and 10 % for monoterpenes.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:42`, `:43`.

## The atmosphere on the sphere (S4a)

The barotropic core on the cubed sphere, `docs/handoff/atmosphere-sphere.md`. Putman and Lin 2007 (the grid's non-orthogonality) and Williamson et al. 1992 (the test cases) are under [astronomy and geometry](astronomy-geometry.md); the leapfrog filter is above.

### Charney, Fjørtoft and von Neumann 1950: the barotropic vorticity equation
Charney, J. G., Fjørtoft, R., von Neumann, J. (1950). Numerical integration of the barotropic vorticity equation. *Tellus* 2(4), 237–254. doi:10.3402/tellusa.v2i4.8607 (the original Wiley record is doi:10.1111/j.2153-3490.1950.tb00336.x)
- **Model:** ∂ζ/∂t = −∇·(v (ζ + f)) with the wind from the stream function, ζ = Δψ, f = 2Ω sin φ.
- **Used in:** `@simulation/src/atmosphere/fn/SphereFlow.hpp:29`, `@simulation/src/atmosphere/type/SphereFlow.hpp:12`; `docs/handoff/atmosphere-sphere.md:83`.
- **Checked by:** `@simulation/test/AtmosphereSphereFlow.cpp:54` (the Rossby–Haurwitz wave), `:113` (steady solid rotation), `@simulation/test/AtmosphereSphereDrift.cpp:42` (unforced drift over 30 days).

### Hermeline 2000; Domelevo and Omnes 2005: the discrete duality Laplacian
Hermeline, F. (2000). A finite volume method for the approximation of diffusion operators on distorted meshes. *Journal of Computational Physics* 160(2), 481–499. doi:10.1006/jcph.2000.6466

Domelevo, K., Omnes, P. (2005). A finite volume method for the Laplace equation on almost arbitrary two-dimensional grids. *ESAIM: Mathematical Modelling and Numerical Analysis* 39(6), 1203–1249. doi:10.1051/m2an:2005047
- **Model:** the Laplacian as the energy ½ Σ diamond |∇ψ|² over diamonds of two cell centres and an edge's two vertices, with the gradient on each diamond from the differences across and along the edge; symmetric and positive on distorted meshes.
- **Used in:** `@simulation/src/atmosphere/fn/SphereOperators.hpp:46`, `@simulation/src/atmosphere/fn/SphereOperators.cpp:125`; `docs/handoff/atmosphere-sphere.md:24`, `:26`.
- **Checked by:** `@simulation/test/AtmosphereSphere.cpp:101` (symmetry, null space, positivity, the split product the solver uses), `:61` (the faces' interiors shared, still symmetric bit for bit), `:175` (local error by place), `@simulation/test/AtmosphereSpherePoisson.cpp:36` (second-order solutions).
- **Approximations:** the vertex unknowns are not solved for but replaced by least-squares fits of the cell values, so the operator is cell-centred; its local error at face edges stays at about a tenth of the value, while the solution is second order.

### Hestenes and Stiefel 1952: conjugate gradients
Hestenes, M. R., Stiefel, E. (1952). Methods of conjugate gradients for solving linear systems. *Journal of Research of the National Bureau of Standards* 49(6), 409–436. doi:10.6028/jres.049.044
- **Model:** preconditioned conjugate gradients on the symmetric stiffness, restricted to fields of zero mean.
- **Used in:** `@simulation/src/atmosphere/type/SpherePoisson.hpp:27`, `@simulation/src/atmosphere/fn/SpherePoisson.cpp:55`; `docs/handoff/atmosphere-sphere.md:46`.
- **Checked by:** `@simulation/test/AtmosphereSpherePoisson.cpp:36`, `:64` (null mode), `:94` (unconverged solves reported, the true residual), `:121` (the recursive residual reported from 1e-10 up).

### Brandt 1977: multigrid
Brandt, A. (1977). Multi-level adaptive solutions to boundary-value problems. *Mathematics of Computation* 31(138), 333–390. doi:10.1090/S0025-5718-1977-0431719-X
- **Model:** a V-cycle over the quadtree's coarser levels with Chebyshev–Jacobi smoothing (the two entries below), as the conjugate gradients' preconditioner.
- **Used in:** `@simulation/src/atmosphere/type/SpherePoisson.hpp:29`, `@simulation/src/atmosphere/fn/SphereMultigrid.hpp:9`; `docs/handoff/atmosphere-sphere.md:48`.
- **Checked by:** `@simulation/test/AtmosphereSpherePoisson.cpp:36`, iterations independent of the level; `@simulation/test/AtmosphereSphereMultigrid.cpp:24`, the V-cycle symmetric and positive.

### Adams, Brezina, Hu and Tuminaro 2003: polynomial smoothing
Adams, M., Brezina, M., Hu, J., Tuminaro, R. (2003). Parallel multigrid smoothing: polynomial versus Gauss–Seidel. *Journal of Computational Physics* 188(2), 593–610. doi:10.1016/S0021-9991(03)00194-3
- **Model:** a Chebyshev polynomial in D⁻¹K as the multigrid smoother in place of Gauss–Seidel, damping the upper part of the spectrum, with no order among the cells. The interval here, [0.2, 1.05] · λmax with λmax from 12 power steps, is Neon's proposal (`.out/review-mailbox/from-neon/DREV-NEON-002-02.txt`), not the paper's values.
- **Used in:** `@simulation/src/atmosphere/fn/SphereMultigrid.cpp:13`; `docs/handoff/atmosphere-sphere.md:50`.
- **Checked by:** `@simulation/test/AtmosphereSpherePoisson.cpp:36` (iterations independent of the level), `@simulation/test/AtmosphereSphereMultigrid.cpp:24` (the V-cycle symmetric and positive), `:82` (λmax's estimate).

### Saad 2003: the Chebyshev iteration
Saad, Y. (2003). *Iterative Methods for Sparse Linear Systems*, 2nd ed. SIAM, Philadelphia. doi:10.1137/1.9780898718003. Algorithm 12.1, "Chebyshev Acceleration", §12.3.2, p. 399.
- **Model:** the three-term recurrence of the Chebyshev iteration on an interval [α, β] (θ, δ, σ₁ = θ/δ, ρₖ₊₁ = 1/(2σ₁ − ρₖ), dₖ₊₁ = ρₖ₊₁ρₖ dₖ + 2ρₖ₊₁/δ · rₖ₊₁), applied to K with the Jacobi preconditioner D⁻¹, of degree 3.
- **Used in:** `@simulation/src/atmosphere/fn/SphereMultigrid.cpp:19`; `docs/handoff/atmosphere-sphere.md:50`.
- **Checked by:** `@simulation/test/AtmosphereSphereMultigrid.cpp:24`.

### Higham and Mary 2022: mixed-precision preconditioning
Higham, N. J., Mary, T. (2022). Mixed precision algorithms in numerical linear algebra. *Acta Numerica* 31, 347–414. doi:10.1017/S0962492922000022. §8.2, "Iterative methods with low or mixed precision preconditioner", p. 383.
- **Model:** a preconditioner applied in low precision inside a Krylov solver kept in double; the survey's examples include an fp32 multigrid preconditioning an fp64 CG. Here the multigrid V-cycle runs in float32 above its coarsest level, and CG, its residual, its product and the coarsest solve stay in double. The design (residual scaled into range, coarsest in double) is Neon's proposal (`.out/review-mailbox/from-neon/DREV-NEON-013-01.txt`), which cites no paper; the power-of-two scaling and the bound on the kernel's loads are ours.
- **Used in:** `@simulation/src/atmosphere/type/SpherePoisson.hpp:36`, `@simulation/src/atmosphere/fn/SphereMultigrid.cpp:156`, `@simulation/src/atmosphere/fn/PoissonLevel.hpp:25`; `docs/handoff/atmosphere-sphere.md:52`.
- **Checked by:** `@simulation/test/AtmosphereSphereMultigrid.cpp:24` (symmetric to float32 rounding, positive, within its rounding of the double cycle, exact under power-of-two scaling, finite for a subnormal residual, zero for zero), `:114` (the same CG iterations as the double cycle), `:135` (the float32 product bit for bit as the row sums), `@simulation/test/AtmosphereSpherePoisson.cpp:121` (the reported residual in both precisions).
- **Approximations:** the float32 cycle is symmetric only to its rounding (asymmetry under 4.3e-9 on levels 5 to 8), so plain CG rather than flexible CG relies on it taking the same iterations as the double cycle, which it does from 1e-6 to 1e-12.

### Fischer 1998: projection of successive solutions
Fischer, P. F. (1998). Projection techniques for iterative solution of Ax = b with successive right-hand sides. *Computer Methods in Applied Mechanics and Engineering* 163(1–4), 193–204. doi:10.1016/S0045-7825(98)00012-7 (record checked in Crossref, whose title drops "Ax = b"; the paper itself not read).
- **Model:** the first guess of an iterative solve as the projection, in the energy norm x K x, of the new solution on the span of earlier solutions, so the first guess is never worse in that norm than any of them. Here the span is the last three accepted ψ of the flow, in differences, solved through a 3 × 3 Gram matrix; Fischer builds a K-orthonormal basis instead. Neon's DREV-NEON-019-01 (`.out/review-mailbox/from-neon/DREV-NEON-019-01.txt`) proposed it as the fallback to extrapolation in time.
- **Used in:** `@simulation/src/atmosphere/fn/SphereFlowGuess.cpp:82`, `@simulation/src/atmosphere/fn/SphereFlow.hpp:39`; `docs/handoff/atmosphere-sphere.md:89`.
- **Checked by:** `@simulation/test/AtmosphereSphereStep.cpp:125` (fewer iterations on the Rossby–Haurwitz wave, the flow within the tolerance, the guess's K as the combination of the K dᵢ, restarts), `:28` (a refused step leaves the history).
- **Approximations:** only three levels; a difference whose Cholesky pivot falls below 1e-12 of its diagonal is dropped.

### Holmes and Featherstone 2002: normalised Legendre recursions
Holmes, S. A., Featherstone, W. E. (2002). A unified approach to the Clenshaw summation and the recursive computation of very high degree and order normalised associated Legendre functions. *Journal of Geodesy* 76(5), 279–299. doi:10.1007/s00190-002-0216-2
- **Model:** the 4π-normalised associated Legendre functions by the standard forward column recursions (their eqs. 11–13), for the spherical harmonics of the eddy band.
- **Used in:** `@simulation/src/atmosphere/fn/SphereHarmonics.hpp:7`, `@simulation/src/atmosphere/fn/SphereHarmonics.cpp:9`, the eddy band's shared recursion `@simulation/src/atmosphere/fn/EddyBand.cpp:28`; `docs/handoff/atmosphere-sphere.md:109`.
- **Checked by:** `@simulation/test/AtmosphereSphere.cpp:175` (Δ Y = −l(l + 1)/R² Y), `@simulation/test/AtmosphereSphereBand.cpp:66`, `:73` (the shared recursion against the harmonics mode by mode).

### van Leer 1974; Sweby 1984: the limited tracer flux
van Leer, B. (1974). Towards the ultimate conservative difference scheme. II. Monotonicity and conservation combined in a second-order scheme. *Journal of Computational Physics* 14(4), 361–370. doi:10.1016/0021-9991(74)90019-9

Sweby, P. K. (1984). High resolution schemes using flux limiters for hyperbolic conservation laws. *SIAM Journal on Numerical Analysis* 21(5), 995–1011. doi:10.1137/0721062
- **Model:** upwind flux plus a Lax–Wendroff correction limited by the van Leer limiter of the ratio of successive differences.
- **Used in:** `@simulation/src/atmosphere/fn/SphereFlow.hpp:50`, `@simulation/src/atmosphere/fn/SphereFlow.cpp:79`; `docs/handoff/atmosphere-sphere.md:98`.
- **Checked by:** `@simulation/test/AtmosphereSphereFlow.cpp:154` (mass, bounds, a uniform tracer).
- **Approximations:** the correction runs along one grid line per edge; across the lines the scheme is upwind and diffusive.

### Neumaier 1974: compensated summation
Neumaier, A. (1974). Rundungsfehleranalyse einiger Verfahren zur Summation endlicher Summen. *ZAMM – Zeitschrift für Angewandte Mathematik und Mechanik* 54(1), 39–51. doi:10.1002/zamm.19740540106
- **Model:** the area means over the sphere summed with a running compensation.
- **Used in:** `@simulation/src/atmosphere/fn/SphereOperators.cpp:214`.
- **Used in (S3b):** the planet's stock totals over crust pieces in key order, `@simulation/src/planet/fn/Pieces.hpp:39`.

### Haurwitz 1940: the Rossby–Haurwitz wave
Haurwitz, B. (1940). The motion of atmospheric disturbances on the spherical Earth. *Journal of Marine Research* 3(3), 254–267. https://elischolar.library.yale.edu/journal_of_marine_research/575 (unverified: pages, given as 254–267 by Williamson et al. 1992 and as 245–267 elsewhere)
- **Model:** the wave of wavenumber 4 as an exact solution of the non-divergent barotropic vorticity equation, turning at ν = (R(3 + R) ω − 2Ω) / ((1 + R)(2 + R)) (Williamson et al. 1992, eq. 136 of the ORNL/TM-11895 report).
- **Checked by:** `@simulation/test/AtmosphereSphereFixture.hpp:57`, `@simulation/test/AtmosphereSphereFlow.cpp:54`, `@simulation/test/AtmosphereSphereStep.cpp:78`; `docs/handoff/atmosphere-sphere.md:102`.

## Snow (approved, not yet written)

### Sturm et al. 2010: bulk density of snow
Sturm, M., Taras, B., Liston, G. E., et al. (2010). Estimating snow water equivalent using snow depth data and climate classes. *Journal of Hydrometeorology* 11(6), 1380–1394. doi:10.1175/2010JHM1202.1
- **Model:** snow density by depth, day of the season and snow class (their Table 4); depth per ground cell from SWE.
- **Used in:** planned, `docs/handoff/engine.md:54`, `:131`, `:150`, `:157`.
- **Validation planned:** against the paper's curves and a slope case (`docs/handoff/engine.md:157`).

### Sturm, Holmgren and Liston 1995: snow classes
Sturm, M., Holmgren, J., Liston, G. E. (1995). A seasonal snow cover classification system for local to global applications. *Journal of Climate* 8(5), 1261–1283. doi:10.1175/1520-0442(1995)008<1261:ASSCCS>2.0.CO;2
- **Model:** the snow class from the climate.
- **Used in:** planned, `docs/handoff/engine.md:131`.

Firn densification (Herron and Langway 1980) is under [ice](ice.md).
