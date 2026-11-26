# Geometry of the sphere (planet slice S1)

Owns `@simulation/src/geometry/**` and `@simulation/test/Geometry{,Frame}.cpp`, `GeometryFixture.hpp`. It is the one owner of the sphere's topology and metric: S3 to S6 read neighbours, areas, lengths and frames from here and never step x/z or i/j by hand. It is additive: nothing consumes it yet. The plan and the S0 decisions are in `.out/lead/handover/PLANET.md` ("S0 review outcome" wins).

## State

Built and tested (`simulation-tests geometry` passes, 0.6 s), reviewed by an independent sub-agent and its findings settled. Not wired into any process, the bridge or the engine.

## Contracts

- **Shape.** A sphere of `PlanetRadius` = 6 371 000 m in real metres (`constant/Sphere.hpp`). Earth's flattening (`EarthFlattening`, WGS 84 1/298.257223563) is a recorded deviation: latitudes are geocentric, gravity has no latitude term.
- **Grid.** Six cube faces in S2's numbering and orientation (faces 0..5 face +X, +Y, +Z, −X, −Y, −Z; every face right-handed, u × v outward). Each face is a quadtree: level L has 2^L × 2^L cells, addressed as `Cell{face, level, i, j}` (i along u, j along v), up to `MaxLevel` 30.
- **Mapping: equiangular**, `fn/Projection` (ST in [0, 1] → u = tan(π/4 · (2s − 1))). Chosen over S2's quadratic mapping; see the comparison below. The mapping and the key format share one number, `GeometryVersion` = 1; anything stored by key records it.
- **Key.** `keyOf(Cell)` packs face (bits 61..63), a Z-order interleave of i and j (i's bit above j's, coarsest level highest), then a sentinel bit; a cell's descendants have keys within key ± its lowest set bit. It is not S2's Hilbert order, so keys are not S2CellIds.
- **Levels** (S0 Q2a): `RegionLevel` 5, `LocalLevel` 9, `SiteLevel` 13, `GroundLevel` 17; a tile holds 2^`TileLevels` = 256 cells a side, 8 levels finer (the count itself stays the landscape's `TileCells` until S5 merges the two ladders), so a Ground cell is level 25, nominally 0.298 m (face centre arc / 2^25). Real Ground cell edges run from 0.211 m (middle of a face edge) to 0.298 m (face centre).
- **Corners and edges** run counter-clockwise seen from outside: corner 0 (u low, v low), 1 (high, low), 2 (high, high), 3 (low, high); edge k from corner k to k + 1 (0 faces −v, 1 +u, 2 +v, 3 −u).
- **Neighbours** (`fn/Neighbour`): `neighbourAcross(cell, edge)` and `neighboursOf(cell)` give the cell at the same level and the shared edge in its numbering. Both cells walk the shared edge in opposite directions: our corner k is the neighbour's corner `edge + 1`. Across a face edge the along-edge index may reverse (`faceEdgeOf(face, edge)` gives the whole table for strips of edge cells, e.g. face 0 edge 3 ↔ face 4 edge 2 reversed, which is Codex's seam counterexample (0,0,1) ↔ (4,30,31) for passOf parity). `cellsAround(cell, corner)` lists the other cells at a corner clockwise: three, or two at a cube vertex (`atCubeVertex`).
- **Metric** (`fn/Metric`): cell edges are great circles, so a cell is a geodesic quadrilateral. `areaOf` is its spherical excess × R², `edgeLengthOf` the arc between its corners; both use the gnomonic coordinate steps in closed form (du = sin(Δα)/(cos α₀ cos α₁)), so they hold about 1e-15 relative from a whole face to level 30. `distanceBetween`, `centralAngle` (atan2 of cross and dot: good for tiny and near-antipodal separations), `sphericalExcess`.
- **Frames** (`fn/Frame`, types `SurfaceFrame`, `TangentVector`, `FrameRotation` in `type/Frame.hpp`): `geographicFrame` gives east, north, up (on a pole east is taken as at longitude 0: east (0, 1, 0), north (−1, 0, 0) at the north pole); `faceFrame`/`cellFrame` give a grid frame with its first axis along the face's +u line, which is where solvers keep vector components. At the face centres that axis points east on faces 0, 1 and 5, north on face 2, south on faces 3 and 4. `reframe` moves components between two frames at the same point. `transport` is parallel transport along the shorter great circle (the rotation about from × to). Up to a right angle it needs no axis normalisation, so it is exact for cells a fraction of a metre apart; beyond, Rodrigues' form about the unit axis keeps it finite and reversible up to π, to about 1e-14 (the suite bounds it at 1e-13 on random pairs from 1e-3 to 0 rad short of the antipode). The axis is taken as a × (a + b), equal to a × b, because near the antipode a + b is a small vector of small absolute error while a × b cancels to an error of 1e-16 / sin θ (1.6e-7 at 1e-9 rad short); it is scaled by its largest component before its length is taken, which would underflow below sin θ ≈ 1e-154. Near the antipode the great circle itself is ill-conditioned: rounding the target by 1e-16 tilts it by 1e-16 / sin θ. Only at the exact antipode (the unit directions sum to zero) is the great circle undefined; the axis is then chosen deterministically, perpendicular to from − to and to the coordinate axis that vector leans on least (Codex DREV-CLAUDE-002-03, item 1). `carry(tangent, from, to)` takes components from one cell's frame to another's along the geodesic between the centres, across an edge, a corner or a face edge; carrying back is the identity. The turn is fixed per pair of cells: `carryRotation(from, to)` returns it as a cosine and sine that a solver keeps per edge and applies with `turned`. Antipodal cells follow `transport`'s rule. Round a closed geodesic loop a vector turns by the loop's solid angle in the sense of travel (holonomy), so no loop "returns to itself" except a degenerate one.
- **Edges for fluxes** (`fn/Edge`, type `CellEdge`): `edgeOf(cell, edge)` gives the corners, the arc's midpoint, the length and the outward unit normal of the edge's great-circle plane, tangent everywhere along the edge. The normal is taken in the face's own basis from the edge's coordinate (for edge 1, (−u₁, 1, 0) in (N, U, V)), exact where a cross product of two corners a fraction of a metre apart loses about 1e-16/angle, and the neighbour's view of the edge has bit for bit the opposite normal, the same midpoint and length and swapped corners (on a reversed face seam the two closed-form lengths differ in the last bit, so the length is always taken from the view of the cell with the lower key), so fluxes summed over a closed mesh cancel exactly (across face edges and at cube vertices too). The centre-to-centre direction is not the normal in general: 23° apart at face 0, level 5, cell (29, 29), edge 1. Non-orthogonal metric terms of a gradient stay with S4.
- **Positions**: ECEF metres in doubles (Z to the north pole, X to 0° N 0° E). `ecefOf(direction, height)`, `heightOf(ecef)` (radial height above the datum sphere, authoritative), `cellOf(ecef or direction, level)`, `facePointOf`/`directionOf(FacePoint)`, `latLonOf`/`directionFromLatLon` (reading and the user only). A point within an ulp of a cell edge may land in either cell, so corners are not mapped back to cells.
- **Local frame** (`fn/LocalFrame`, type `LocalFrame`): `anchorAt(ecef)` gives a flat east-north-up frame; `localOf`/`ecefOfLocal` convert. Mapping east-north-up onto the engine's axes is S6's. The frame holds only within its error budget, `tangentErrorAt(d, r)`:
  - height: sqrt(r² + d²) − r ≈ d²/2r: 7.85 cm at 1 km, 785 m at 100 km;
  - tilt: atan(d/r): 0.009° at 1 km;
  - distance: d − r·atan(d/r) ≈ d³/3r²: 8.2 µm at 1 km, 8.2 m at 100 km;
  - `tangentReach(tolerance, r)` inverts the height: half a voxel (6.25 cm) is reached at 892 m.
- **Names in `derivean::sim`.** `type/Vec3.hpp` (`Vec3` with free functions; `normalized` leaves a zero vector zero) is the 3-vector the astronomy stream builds on. `diagnostics/type/Scene.hpp` now includes it instead of defining its own (below). The domain defines no `Pi` (it uses `std::numbers::pi`), because about 11 files keep their own `Pi` in an anonymous namespace inside `derivean::sim` and would become ambiguous; type names are chosen not to meet the files' local `struct Frame`. Brace-list calls that fit both `Vec2` and `Vec3` (`length({a, b})`) are ambiguous in a file that includes both headers; spell the type.
- **Inputs.** `parentOf` to a finer level, `childOf` at `MaxLevel` and `cellOfKey(0)` are programming errors (asserted in debug builds).

## Edit outside the domain

`@simulation/src/diagnostics/type/Scene.hpp`, former lines 10–18 (approved by the lead, 26 Nov): its own `struct Vec3` with `operator+`, `operator-`, `operator*(Vec3, double)`, `dot` and `normalized` was a second `derivean::sim::Vec3` linked into the same library (an ODR clash with two different `normalized` bodies, and a compile error in any file including both headers). It now has `#include "geometry/type/Vec3.hpp"` on line 4 instead; the geometric `normalized` carries the same `max(1e-30, dot)` guard token for token. Checked: the library, `terra` and `simulation-tests geometry` build and pass, and the machine code (`otool -tV`) of all eight translation units that use the header (`diagnostics/fn/{Gather,Scene,View}.cpp`, `tool/{Block,View}Command.cpp`, `tool/{CoastMap,CoastShow,VolcanicScene}.cpp`) is identical before and after, so `terra view` pictures cannot change; a `terra view` run was not made (it builds the whole planet first).

## Projection comparison (before locking)

Measured by `.out/geometry/compare/Compare.cpp` (results in `result.txt` there), on the sphere of R = 6 371 km. "Mean" is 4πR² / (6 · 4^L).

| Level 10 (≈ 9.8 km cells) | quadratic (S2 default) | equiangular (chosen) | linear |
| --- | --- | --- | --- |
| smallest cell / mean | 0.602 | 0.834 | 0.368 |
| largest cell / mean | 1.259 | 1.178 | 1.910 |
| largest / smallest area | 2.09 | 1.41 | 5.19 |
| shortest edge | 5 872 m | 6 911 m | 5 870 m |
| longest / shortest edge | 1.81 | 1.41 | 2.12 |
| shortest diagonal | 7 831 m | 9 219 m | 5 874 m |
| ST ↔ UV round trip | 2.4 ns | 7.9 ns | 1.6 ns |

The corner angle at a cube vertex is 120° (90° + 30°) in every mapping; three cells meet there.

Why equiangular:
- The shortest edge sets explicit time steps (CFL) and the smallest cell the finest resolution a solver pays for. Equiangular gives an 18 % longer shortest edge and a 39 % larger smallest cell for the same cell count, and a near-uniform grid (1.41 spread against 2.09).
- It is the grid Earth's own cubed-sphere models use (GFDL FV3), which is the reference the atmosphere slice S4 follows.
- Its only cost is a tan/atan per conversion, 5 ns more, which is negligible beside any process that reads a cell.
- S0 preferred quadratic because the S2 library would supply ids and geometry; the library is not taken (below), so that reason fell away. If the library is ever wanted, S2 compiles with its tangent projection (`S2_TAN_PROJECTION`).

## The S2 Geometry library: not integrated

- xmake-repo (local copy of 18 Nov 2026) has no `s2geometry` package, only `abseil`. Integration would need our own `package()` script around S2's CMake build, with abseil and OpenSSL (S2's exact arithmetic) as further dependencies of `derivean-simulation` and therefore of `@napi` and `@engine`: three more native libraries in every build, for about 100 k lines of which we would use ids, neighbours and the face mapping.
- What the slice needs fits in about 700 lines of our own code with comments (projection, quadtree, neighbours, metric, frames). The physical metric, the shared-edge orientation and the vector transforms were ours in any case (S0 Q1). So no `add_requires` was added.

## Verify

The `xmake` on the path is a mise shim, and inside `@simulation` mise sets `XMAKE_CONFIGDIR` to the shared `.out/simulation/xmake`, overriding one given on the command line; a private build therefore calls the installed binary directly:

```
cd @simulation && X=$(ls -d ~/.local/share/mise/installs/github-xmake-io-xmake/*/ | head -1)xmake && export XMAKE_CONFIGDIR=$PWD/../.out/geometry/xmake && $X config --yes --mode=release --builddir=../.out/geometry/build && $X build --yes simulation-tests
../.out/geometry/build/macosx/arm64/release/simulation-tests geometry
```

`test/Geometry.cpp` (grid) and `test/GeometryFrame.cpp` (frames) check:
- the face edge (7 842.4 km), the arc across a face centre (10 007.5 km) and the nominal Ground cell (0.298 m); edge lengths against their corners;
- area sums against 4πR² to 1e-9 relative on every level 0..10 (6.3 M cells at level 10), and 4⁴ descendants against their ancestor to 1e-12 at levels 13, 17, 25 and 30;
- neighbour symmetry and the opposite orientation of every shared edge (by its corners) on all cells of level 4 and on sampled cells (face corners, edges, centre, scatter) of every face at levels 0, 1, 3, 5, 9, 13, 17, 25 and 30; Codex's seam pair and the exposed face-edge table; the cells round every sampled corner share it, two at cube vertices and three elsewhere;
- round trips ST ↔ UV, cell centre → ECEF at −16 km to 8 848 m → height and cell, the centre in the middle of its ST square, face point ↔ direction, key ↔ cell, parent and child, key ranges; fixed keys of version 1; S2's face tie rule; a point on a cell edge going to the cell above;
- where the frames point: east and north at 0° 0°, at 0° 90° E and on the pole, and the grid frame's first axis on every face centre;
- the two views of every shared edge (inner, face edge, cube vertex): swapped corners, equal midpoint and length and the normal negated bit for bit; unit normal and midpoint, the length against the arc between the corners, the normal tangent along the edge and pointing out; Codex's 23° case;
- the same vector at one point in the frames of two faces on their shared edge, and the geographic frame's orthonormality;
- carrying a vector across every edge and corner and back is the identity, and equals the transported vector;
- transport near and at the antipode: Codex's two reproductions (1e-9 short of the antipode; cells (0, 30, 2²⁹, 2²⁹) and (3, 30, 2²⁹ + 1, 2²⁹)), the exact antipode, 2 000 random pairs off the axes at 1e-3 to 1e-15 rad short of the antipode and at it, and every sampled cell carried to the cells round its antipode and back stay finite, tangent, reversible to 1e-13 and reach the target; angles 1e-160 to 1e-300 rad short of the antipode stay a rotation; beyond a right angle transport equals two steps through the arc's midpoint;
- parallel transport round loops turns by the loop's solid angle: the octant (π/2), the three cells round a cube vertex, four cells round inner and face-edge corners at Site level; transport is a rotation onto the target;
- great-circle distances against haversine (London–New York, Sydney–Santiago, 0.64 m in Prague), the quarter circle and a near-antipode;
- the tangent budget: 7.85 cm and 0.009° at 1 km, 785 m at 100 km, `tangentReach` inverting it, and the same height, tilt and distance errors measured by placing planar points through an anchored frame and reading them on the sphere.

Mutations of the neighbour table, the transport rotation, the area, the tangent height, the frame orientations (west/south, north/−east, grid frame on v or −u, pole east), the key layout, the cell centre, the face tie rule, the sign of `turned`, the old transport near the antipode, the axis a × b in place of a × (a + b), its length taken without scaling, a half-turn about a + b beyond a right angle and wrong edge normals each fail the suite. The suite also passes at -O0 and with `-ffp-contract=off` (fused multiply-adds are why the edge normal is not a cross product: with them `to × from` is not bitwise `−(from × to)`).

## Open

- Nothing consumes the domain yet; S3 to S6 adopt it. S5's `TileKey` gains a face and maps onto `Cell`; the passOf parity counterexample above needs explicit seam ownership there.
- The geoid (flattening, latitude-dependent gravity) is a later step.
- **For S4's grad/div** (Codex DREV-CLAUDE-002-03): evaluate each shared edge once at `edgeOf(...).midpoint` along its `normal` and apply the flux to both cells with ± signs. Tests the operator needs: the degree-1 harmonic, Δu = −2u/R² for u = z/R (and higher degrees, Δ Y_l = −l(l+1)/R² Y_l); face interior against face edge and cube corner; the area-weighted divergence summed over the closed mesh is zero; the discrete operator is symmetric and its only null mode is the constant, removed by area before CG. The standard shallow-water cases of Williamson et al. (1992) follow once the operator stands.
- Vector components are kept in the face grid frame (orthonormal, first axis along +u). A solver that wants covariant or contravariant components on the non-orthogonal u/v basis (FV3 style) derives them from `faceAxisU`/`faceAxisV` at the point; not provided yet.
- Neighbours are given at one level; coverage across levels (`cover`/`parent` in DREV-008's terms) is the quadtree (`parentOf`, `childOf`), and neighbours of a different level are left to S5's tile owner.

## Sources

- **Google, S2 Geometry** (Eric Veach et al., 2005 onwards), C++ library, Apache-2.0, https://github.com/google/s2geometry, `src/s2/s2coords.h`; cell hierarchy: https://s2geometry.io/devguide/s2cell_hierarchy.html. Gives the face numbering and orientation (`FaceUVtoXYZ`, `GetFace` with ties to the higher axis), the ST/UV split and the face quadtree; its three projections (linear, tangent with a 2⁻⁵³ end correction, quadratic by default) are the ones compared. Our tangent mapping is exact at the face edges instead of corrected, so it is not bit-identical to `S2_TAN_PROJECTION`.
- **R. Sadourny** (1972), "Conservative finite-difference approximations of the primitive equations on quasi-uniform spherical grids", *Monthly Weather Review* 100(2), 136–144, doi:10.1175/1520-0493(1972)100<0136:CFAOTP>2.3.CO;2. The gnomonic cubed sphere the grid is built on.
- **C. Ronchi, R. Iacono, P. S. Paolucci** (1996), "The 'cubed sphere': a new method for the solution of partial differential equations in spherical geometry", *Journal of Computational Physics* 124(1), 93–114, doi:10.1006/jcph.1996.0047. The equiangular gnomonic grid (great-circle coordinate lines at equal central angles) that `stToUv` implements.
- **M. Rančić, R. J. Purser, F. Mesinger** (1996), "A global shallow-water model using an expanded spherical cube: gnomonic versus conformal coordinates", *Quarterly Journal of the Royal Meteorological Society* 122(532), 959–982, doi:10.1002/qj.49712253209. Gnomonic against conformal cube grids; the reason the grid stays gnomonic (great-circle cell edges) and keeps vector components in an orthonormal frame per cell.
- **W. M. Putman, S.-J. Lin** (2007), "Finite-volume transport on various cubed-sphere grids", *Journal of Computational Physics* 227(1), 55–78, doi:10.1016/j.jcp.2007.07.022. FV3's comparison of cubed-sphere grids; the reference for preferring the equiangular grid for the atmosphere slice, and for the gnomonic grid's non-orthogonality, which is why a flux uses the edge's own normal (`fn/Edge`) rather than the centre-to-centre direction.
- **E. M. O'Neill, R. E. Laubscher** (1976), "Extended studies of a quadrilateralized spherical cube Earth data base", Naval Environmental Prediction Research Facility Tech. Report NEPRF 3-76 (CSC); PROJ's implementation: https://proj.org/en/stable/operations/projections/qsc.html. The equal-area QSC alternative, considered and not taken (worse angles at face edges, no great-circle cell edges).
- **A. van Oosterom, J. Strackee** (1983), "The solid angle of a plane triangle", *IEEE Transactions on Biomedical Engineering* BME-30(2), 125–126, doi:10.1109/TBME.1983.325207. The tan(E/2) formula for a triangle's spherical excess used by `areaOf` and `sphericalExcess`.
- **O. Rodrigues** (1840), "Des lois géométriques qui régissent les déplacements d'un système solide dans l'espace…", *Journal de Mathématiques Pures et Appliquées* 5, 380–440, http://www.numdam.org/item/JMPA_1840_1_5__380_0/. The rotation about from × to behind `transport`.
- **M. P. do Carmo** (1976), *Differential Geometry of Curves and Surfaces*, Prentice-Hall, §4-5 (Gauss–Bonnet). Parallel transport round a closed loop turns by the enclosed integral of curvature, area/R² on the sphere: the holonomy test.
- **R. W. Sinnott** (1984), "Virtues of the haversine", *Sky and Telescope* 68(2), 159. The independent distance formula the tests compare `distanceBetween` with.
- **C. F. F. Karney** (2013), "Algorithms for geodesics", *Journal of Geodesy* 87, 43–55, doi:10.1007/s00190-012-0578-z. The ellipsoidal geodesic reference (GeographicLib) to adopt when flattening is modelled; not used on the sphere.
- **H. Moritz** (2000), "Geodetic Reference System 1980", *Journal of Geodesy* 74, 128–133, doi:10.1007/s001900050278. Earth's mean radius R₁ = 6 371 008.8 m; the planet uses the rounded 6 371 000 m.
- **National Imagery and Mapping Agency** (1997, amended 2000), *Department of Defense World Geodetic System 1984*, TR 8350.2, 3rd ed.; parameters at https://earth-info.nga.mil/GandG/wgs84/. The flattening 1/298.257223563 recorded as the known deviation.
- **D. L. Williamson, J. B. Drake, J. J. Hack, R. Jakob, P. N. Swarztrauber** (1992), "A standard test set for numerical approximations to the shallow water equations in spherical geometry", *Journal of Computational Physics* 102(1), 211–224, doi:10.1016/S0021-9991(05)80016-6. The standard test cases S4's operator on this grid is to pass (Open).
- **G. M. Morton** (1966), *A computer oriented geodetic data base and a new technique in file sequencing*, International Business Machines Co., Ottawa. The Z-order interleave of the cell key.
