# Geometry of the sphere (planet slices S1 and S3a)

Owns `@simulation/src/geometry/**` and `@simulation/test/Geometry{,Frame,Sphere,SphereOverlap}.cpp`, `GeometryFixture.hpp`, `GeometrySphereFixture.hpp`. It is the one owner of the sphere's topology and metric: S3 to S6 read neighbours, areas, lengths and frames from here and never step x/z or i/j by hand. It is additive: only the sphere atmosphere of S4a (itself not called by the game) reads its grid; S3a's rotations, overlaps and candidates are read by nothing yet (S3b). The plan and the S0 decisions are in `.out/lead/handover/PLANET.md` ("S0 review outcome" wins).

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
- **Frames** (`fn/Frame`, types `SurfaceFrame`, `TangentVector`, `FrameRotation` in `type/Frame.hpp`): `geographicFrame` gives east, north, up (on a pole east is taken as at longitude 0: east (0, 1, 0), north (−1, 0, 0) at the north pole); `faceFrame`/`cellFrame` give a grid frame with its first axis along the face's +u line, which is where solvers keep vector components. At the face centres that axis points east on faces 0, 1 and 5, north on face 2, south on faces 3 and 4. `reframe` moves components between two frames at the same point. `transport` is parallel transport along the shorter great circle (the rotation about from × to). Up to a right angle it needs no axis normalisation, so it is exact for cells a fraction of a metre apart; beyond, Rodrigues' form about the unit axis keeps it finite and reversible up to π, to about 1e-14 (the suite bounds it at 1e-13 on random pairs from 1e-3 to 0 rad short of the antipode). The axis is taken as a × (a + b), equal to a × b, because near the antipode a + b is a small vector of small absolute error while a × b cancels to an error of 1e-16 / sin θ (1.6e-7 at 1e-9 rad short); a + b is first scaled by an exact power of two to a largest component in [0.5, 1), since its length would underflow below sin θ ≈ 1e-154 and subnormal components would round the cross product to the absolute 1e-324 grid and tilt the axis (Codex DREV-CLAUDE-002-04). Directions are ECEF vectors of length 1e-15 to 1e150, the range `normalized` makes unit. Near the antipode the great circle itself is ill-conditioned: rounding the target by 1e-16 tilts it by 1e-16 / sin θ. Only at the exact antipode (the unit directions sum to zero) is the great circle undefined; the axis is then chosen deterministically, perpendicular to from − to and to the coordinate axis that vector leans on least (Codex DREV-CLAUDE-002-03, item 1). `carry(tangent, from, to)` takes components from one cell's frame to another's along the geodesic between the centres, across an edge, a corner or a face edge; carrying back is the identity. The turn is fixed per pair of cells: `carryRotation(from, to)` returns it as a cosine and sine that a solver keeps per edge and applies with `turned`. Antipodal cells follow `transport`'s rule. Round a closed geodesic loop a vector turns by the loop's solid angle in the sense of travel (holonomy), so no loop "returns to itself" except a degenerate one.
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
- transport near and at the antipode: Codex's two reproductions (1e-9 short of the antipode; cells (0, 30, 2²⁹, 2²⁹) and (3, 30, 2²⁹ + 1, 2²⁹)), the exact antipode, 2 000 random pairs off the axes at 1e-3 to 1e-15 rad short of the antipode and at it, and every sampled cell carried to the cells round its antipode and back stay finite, tangent, reversible to 1e-13 and reach the target; angles 1e-160 rad short of the antipode down to the smallest subnormal stay a finite rotation onto the target, also off the axes; beyond a right angle transport equals two steps through the arc's midpoint;
- parallel transport round loops turns by the loop's solid angle: the octant (π/2), the three cells round a cube vertex, four cells round inner and face-edge corners at Site level; transport is a rotation onto the target;
- great-circle distances against haversine (London–New York, Sydney–Santiago, 0.64 m in Prague), the quarter circle and a near-antipode;
- the tangent budget: 7.85 cm and 0.009° at 1 km, 785 m at 100 km, `tangentReach` inverting it, and the same height, tilt and distance errors measured by placing planar points through an anchored frame and reading them on the sphere.

Mutations of the neighbour table, the transport rotation, the area, the tangent height, the frame orientations (west/south, north/−east, grid frame on v or −u, pole east), the key layout, the cell centre, the face tie rule, the sign of `turned`, the old transport near the antipode, the axis a × b in place of a × (a + b), a + b left unscaled, its scale applied as 1 / largest, a half-turn about a + b beyond a right angle and wrong edge normals each fail the suite. The suite also passes at -O0 and with `-ffp-contract=off` (fused multiply-adds are why the edge normal is not a cross product: with them `to × from` is not bitwise `−(from × to)`).

## Open

- Nothing consumes the domain yet; S3 to S6 adopt it. S5's `TileKey` gains a face and maps onto `Cell`; the passOf parity counterexample above needs explicit seam ownership there.
- The geoid (flattening, latitude-dependent gravity) is a later step.
- **For S4's grad/div** (Codex DREV-CLAUDE-002-03): evaluate each shared edge once at `edgeOf(...).midpoint` along its `normal` and apply the flux to both cells with ± signs. Tests the operator needs: the degree-1 harmonic, Δu = −2u/R² for u = z/R (and higher degrees, Δ Y_l = −l(l+1)/R² Y_l); face interior against face edge and cube corner; the area-weighted divergence summed over the closed mesh is zero; the discrete operator is symmetric and its only null mode is the constant, removed by area before CG. The standard shallow-water cases of Williamson et al. (1992) follow once the operator stands.
- Vector components are kept in the face grid frame (orthonormal, first axis along +u). A solver that wants covariant or contravariant components on the non-orthogonal u/v basis (FV3 style) derives them from `faceAxisU`/`faceAxisV` at the point; not provided yet.
- Neighbours are given at one level; coverage across levels (`cover`/`parent` in DREV-008's terms) is the quadtree (`parentOf`, `childOf`), and neighbours of a different level are left to S5's tile owner.

## S3a: the sphere grid, rotations, overlaps and candidates

Brief: `.out/lead/takeover/S3a-brief.md`; design `.out/lead/handover/S3-design.md` §4, §10, §11; Codex DREV-CLAUDE-003-01 (V3, V10) and -02 (the counterexample). Built and tested (`simulation-tests geometrysphere geometry atmospheresphere` pass, also at -O0 with asserts), additive. Measurements and their programs are in `.out/s3a/` (`cost.txt`, `bench/`, `probe/`, `mutations.txt`).

### The grid moved here (Codex V10, lead's L6)

`type/SphereGrid.hpp`, `fn/SphereGrid.{hpp,cpp}` replace `atmosphere/{type,fn}/SphereGrid.*`. The atmosphere imports them directly; there is no forwarding header.

- **Geometry holds:** `level`, `side`, `cells`; `area` (`areaOf`), `centre` (`centreOf`); per cell its four edges and four vertices in its own order; `SphereEdge` = left (lower index), right, from, to (vertices, left's counter-clockwise order), `length` and `normal` (`edgeOf` from left); `SphereVertex` = its three or four cells (clockwise from the cell that numbered it) and its `point` (that cell's `cornerOf`). Functions: `makeSphereGrid`, `indexOf`/`cellAt` (index `(face · side + j) · side + i`), `sphereCellCount`, `neighbourOf(grid, cell, k)`, `placeOf`/`SpherePlace` (inside, face edge, cube corner).
- **Added for S3:** `reach` per cell, the largest chord from the centre to a corner (the candidates' envelope), and `vertex.point`, shared bit for bit by every cell at the vertex, so `polygonOf(grid, cell)` tiles the sphere without slivers at face edges.
- **Stayed in the atmosphere** (`atmosphere/{type,fn}/SphereStencil`, `type/SphereMatrix`), because each is a choice of the weather's discretisation, not of the sphere: per edge `diamond` (the dual cell of its Laplacian), `across`/`along` (the dual basis of its edge gradient), `chord` (its circulation), `offset` (its edge value), `behind` (its limiter's upstream cells, left and right); per vertex the least-squares weights (its vertex values); the `SphereMatrix`. Its `topology` (per edge left, right, from, to) and `vertexCells` are copies of the grid's facts for the step's hot loops; the grid stays their owner. The atmosphere's `using SphereField = std::vector<double>` became `CellField`, because the name is now this domain's type below.
- **Left out:** `latitudeSine` (it was `centre.z`; the weather's Coriolis reads `centre[c].z`, the same bits).
- **Kept bit for bit:** `.out/s3a/probe/Probe.cpp` hashes every table, the Laplacian, vertex values, gradients, fluxes, divergence, curl, the Poisson solutions at three tolerances (iterations, residuals), twelve Rossby–Haurwitz steps, ten tracer steps and the eddy band at levels 1 to 8, built against the library before and after: identical. The stencil takes each edge's corners and midpoint by the same expressions `edgeOf` uses. S4a's bench (`.out/s3a/bench/pair2.txt`, before and after back to back, load 23 to 34): the same iterations and residuals, step and solve times within noise (L7 Poisson 1e-9 101.6 / 101.7 ms, barotropic step 85.1 / 81.4 ms). Building the grid costs more once per world, because two owners now walk it: 39 → 48 ms at L7 (geometry 26 ms, stencil 20 ms) and 147 → 181 ms at L8; memory 47 → 50 MB at L7 (`reach`, `point`).

### Sphere fields

`type/SphereField.hpp`: `SphereField<T>{level, value}`, one value per cell in the grid's order, so a field carries its level and cannot be sampled as a flat raster. Nothing more; S3b adds what it needs.

### Rotations (`type/Rotation.hpp`, `fn/Rotation`)

- `Rotation` is a unit quaternion (Hamilton 1844), w = cos(θ/2), (x, y, z) = sin(θ/2) · axis, the Euler–Rodrigues parameters (Rodrigues 1840); default identity; q and −q are the same rotation. `RotationMatrix` by rows.
- `rotationAbout(axis, angle)`; `rotationOf(ω, t)` from an Euler vector (rad/s) and a time: the vector part is ω · t/2 · sinc(|ω| t/2), never ω/|ω|, so ω = 0 is the identity bit for bit and tiny angles keep full relative precision (1e-9 to 1e-20 rad to 1e-15 relative in the angle and the vector part).
- `compose(later, first)` = the Hamilton product later · first: rotating by it is `first`, then `later`. A plate's step on a piece is `Q = renormalized(compose(rotationOf(ω, dt), Q))`. `inverse` (conjugate), `renormalized`, `angleOf` (2 atan2(|v|, |w|), in [0, π]), `rotate(q, v)`, `matrixOf`, `rotate(matrix, v)`.
- Pure functions, no state: the same calls in the same order give the same bits on any thread.
- Checked: composition order, inverse and matrix to a few ulp (worst of 10⁵ random pairs 3e-15); 2π in 10³ and 10⁵ steps back to the identity and every point within 1e-12; 10⁶ steps of 1e-9 rad equal to one rotation of 1e-3 within 5e-14 (rounding in the vector part, about N · ulp/2; over 100 axes median 1e-16, worst 1.1e-14); the move of an Euler vector is (ω × x) t. Costs: compose + renormalize 16 ns, `rotationOf` 9 ns, a rotated four-corner footprint 12 ns.

### Overlaps (`type/SpherePolygon.hpp`, `fn/SpherePolygon`, `fn/Overlap`)

- `SpherePolygon`: up to eight unit corners, counter-clockwise from outside, convex, great-circle edges. `polygonOf(cell)` (by `cornerOf`), `polygonOf(grid, cell)` (by the grid's shared vertex points), `rotated(polygon, q)`, `capOf` (normalized corner sum and the largest chord to a corner).
- `overlapArea(a, b)` (m²) is the weight of Jones's (1999) first-order conservative coupling, computed as CSLAM does (Lauritzen, Nair & Ullrich 2010): both polygons projected gnomonically onto the plane tangent at a's cap centre (great circles are straight lines there), a clipped by each edge of b (Sutherland & Hodgman 1974), the area the sum of the fan's spherical excesses by van Oosterom & Strackee's tan(E/2) with the corners' plane coordinates (P = (x, y, 1); the triple product is the plane cross product of corner differences, so it keeps its relative precision on small polygons). `areaOf(polygon)` is the same fan. Polygons whose caps are farther apart than their reaches give 0 without clipping.
- Limits: each polygon within a chord of 0.35 (about 20°) of its centre, so every corner of either lies well in front of the plane (asserted): cells of level 2 and finer (largest reach 0.285 at L2, 0.524 at L1) and their rotations. At most eight corners each; a clip adds at most one corner, so the sixteen-slot buffer is asserted.
- Checked at levels 2, 5, 7 and 10 on corner, edge, centre and scattered cells of every face: a cell with itself and the polygon's area equal `areaOf(cell)` to 1e-12; neighbours across edges (face seams and cube vertices included) and across corners overlap by less than 1e-12 of a cell; cells two rows apart give exactly 0; a cell inside its parent gives its own area; an offset 2 × 2 block overlaps each parent it straddles in exactly its one child (1e-12); overlap(a, b) = overlap(b, a) to 1e-12.
- Cost: 116 to 134 ns per overlap of a footprint with one candidate (L5 and L7, load 9 to 34).

### Candidates by the footprint's envelope (Codex V3; `fn/Candidates`)

- `candidateCells(grid, footprint, cells, work)` returns, in a deterministic flood order, every cell whose overlap with the footprint can be positive.
- **Guarantee.** The footprint lies in its cap (centre m, chord ρ, from `capOf`), each cell in its own (centre c, chord `reach`), so a cell that meets the footprint has |m − c| ≤ ρ + reach (triangle inequality of chords; margin 1e-12 on the unit sphere). The flood starts at the cell holding m and crosses shared edges only from cells that pass the test; across face edges and round cube vertices an edge is an edge. It misses no cell that meets the footprint: between two such cells runs a geodesic inside the convex footprint, every cell it crosses passes the test, and where it crosses a vertex the vertex is in the footprint, so every cell round it passes too.
- **Codex's fixture** (DREV-CLAUDE-003-02, L7): birth cell face 0, i = j = 127, corner (s, t) = (1, 127/128) at 63 577.254 m from the centre; the centre turned to (0.995, 0.5 − 1e-7), the corner pointing along the tangent towards +Z. The centre lands in (127, 63), the corner in (127, 65), two rows away; (127, 65) is a candidate with a positive overlap, and the candidates cover the footprint to 1e-12.
- **Coverage before normalisation**: 6 000 footprints at each of L5 and L7, a third each inside faces, within 1.5 cells of a face edge and of a cube vertex, random birth cell and spin: Σ of exact overlaps over the candidates against `areaOf(birth)` worst 1.2e-14 (L5), 4.2e-14 (L7). On 90 L5 footprints every cell outside the candidates was clipped too: none overlaps by more than 1e-14 of the footprint.
- Mutations that each fail the suite: candidates only the centre's ring, no cell reach, 0.7 of the envelope, the flood kept on one face; overlap dropping points on the clip line, the excess scaled by 1e-7, an eager apart test; w = 1, a wrong product sign, a wrong rotate sign, sinc = 1, no renormalization; reach from one corner (`.out/s3a/mutate.sh`, `mutations.txt`).

| per footprint (20 000 each, one thread) | candidates mean / max | with positive overlap | candidates | one overlap | whole footprint |
| --- | --- | --- | --- | --- | --- |
| L5 inside a face | 7.0 / 10 | 4.5 | 285 ns | 119 ns | 1.12 µs |
| L5 near a face edge | 8.2 / 12 | 5.0 | 346 ns | 122 ns | 1.36 µs |
| L5 near a cube vertex | 9.2 / 12 | 4.8 | 351 ns | 118 ns | 1.45 µs |
| L7 inside a face | 7.0 / 11 | 4.5 | 442 ns | 134 ns | 1.60 µs |
| L7 near a face edge | 8.3 / 12 | 5.0 | 377 ns | 126 ns | 1.51 µs |
| L7 near a cube vertex | 9.5 / 13 | 4.9 | 377 ns | 116 ns | 1.52 µs |

"Whole footprint" is the candidates plus every exact overlap. Apple M3 Max under a load average of 9 to 34 (other streams building); `.out/s3a/cost.txt`, `bench/Cost.cpp`.

### Contract for S3b

- One `SphereGrid` per level per world, shared with the weather (whose `SphereStencil` must be made from that grid: asserted by size in the operators and the flow); fields as `SphereField<T>` in its order.
- A piece's footprint is `rotated(polygonOf(grid, birth), Q)`; its area is `grid.area[birth]` (a rotation keeps it; the fan of the rotated corners agrees to about 4e-14 at L7).
- Exact weights: `candidateCells` then `overlapArea(footprint, polygonOf(grid, c))` for each candidate, with the footprint first (one tangent plane for all its cells). Before any normalisation Σ_c w_ck = a_k to 1e-12 (measured 4e-14); a test of S3b that normalises must check this sum first (V3).
- Budget: about 1.5 µs per footprint at L7, so 98 304 pieces cost 0.15 s per substep if all were exact; the boundary band is a fraction of that. `candidateCells` reuses its two vectors (no allocation in the loop) and keeps its visited list linear, which suits footprints of about one cell; a footprint many cells wide needs a set.
- Rotations: `Q = renormalized(compose(rotationOf(ω, dt), Q))` per substep, deterministic per piece. The rounding of Q grows about linearly with the steps (2.4e-13 worst after 10⁵ steps), so a piece's lifetime Q stays within 1e-12 up to about 4 × 10⁵ substeps; a 4.6 Gyr history at L7 has about 1.3 × 10⁴.
- **Vectors and normals across the map are not here** (Codex V7, S3e): a Jacobian J of a face map takes a velocity as J⁻¹v and a normal as Jᵀn, then normalised; S3a builds no Jacobian.

### Verify

Other streams edit the shared checkout, so the build is a private snapshot: HEAD's `@simulation` with only S3a's paths overlaid (`.out/s3a/snapshot.sh`; `build.sh` builds it in `.out/s3a/tree-build`; overlaid files get a fresh mtime, because xmake decides by mtime at one-second resolution and a copy with the old mtime leaves a stale object).

```
.out/s3a/build.sh
.out/s3a/tree-build/macosx/arm64/release/simulation-tests geometrysphere geometry atmospheresphere
```

In a quiet checkout the usual private build works too (as above under S1, with `.out/s3a` paths). The probe: `clang++ -std=c++20 -O3 [-DAFTER] -I<src> .out/s3a/probe/Probe.cpp <lib>` before and after, then `diff`.

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
- **W. R. Hamilton** (1844), "On quaternions; or on a new system of imaginaries in algebra", *The London, Edinburgh, and Dublin Philosophical Magazine and Journal of Science* 25(163), 10–13, doi:10.1080/14786444408644923. The quaternion product of `fn/Rotation` (S3a); Rodrigues (1840) above gives the parameters.
- **P. W. Jones** (1999), "First- and second-order conservative remapping schemes for grids in spherical coordinates", *Monthly Weather Review* 127(9), 2204–2210, doi:10.1175/1520-0493(1999)127<2204:FASOCR>2.0.CO;2. The overlap weights of `fn/Overlap` (S3a).
- **P. H. Lauritzen, R. D. Nair, P. A. Ullrich** (2010), "A conservative semi-Lagrangian multi-tracer transport scheme (CSLAM) on the cubed-sphere grid", *Journal of Computational Physics* 229(5), 1401–1424, doi:10.1016/j.jcp.2009.10.036. Overlaps in gnomonic coordinates (S3a).
- **I. E. Sutherland, G. W. Hodgman** (1974), "Reentrant polygon clipping", *Communications of the ACM* 17(1), 32–42, doi:10.1145/360767.360802. The clip of `fn/Overlap` (S3a).
- **G. M. Morton** (1966), *A computer oriented geodetic data base and a new technique in file sequencing*, International Business Machines Co., Ottawa. The Z-order interleave of the cell key.
