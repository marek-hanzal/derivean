# The planet's history on the cubed sphere (planet slice S3b)

Owns `@simulation/src/planet/{type/Globe.hpp,constant/Globe.hpp,fn/{Pieces,Rotate,Coupling}.*}`, `@simulation/src/tectonics/{type,fn}/Euler.*`, the 3D noise in `@simulation/src/core/fn/Noise` and `@simulation/test/Globe*.cpp`. Additive: the game and today's flat history (`planet/fn/{Evolve,Motion,Crossing,Outline,Contacts,Surface,Heat}`) are unchanged; the switch is S3e. Brief `.out/lead/takeover/S3b-brief.md`; design `.out/lead/handover/S3-design.md` (CLOSED by Codex, DREV-CLAUDE-003-06), which this document does not repeat.

## State

| Stage | Content | State |
| --- | --- | --- |
| 1 kinematics and pieces | Euler vectors and NNR after damping, `Globe`, pieces, rotation, coupling (exact and cloud-in-cell) | done, tests 1, 2, 3, 6 |
| 2 boundaries | band, union over Clipper2, transfer plan, births and consumptions | next |
| 3 surface and view | contacts, surface processes, heat, evolve, flat view, `terra globe` | |
| 4 Earth | Earth's plate speeds, retuning against hypsometry | |
| 5 parallel | persistent workers, dynamic band batches, determinism, timings | |

## Stage 1: kinematics and pieces

- **Epochs** (`tectonics/fn/Euler`): `eulerEpochAt` takes index, start, end and seed from the flat `epochAt` of the same world, so both histories reorganise together. n = lattice² sites (9 to 25) on a Fibonacci lattice (González 2010), turned at random per epoch and jittered by up to 0.4 of the site spacing; reach 0.62–1.55; pole uniform on the sphere. `plateAt` is the weighted Voronoi by angular distance / reach after a 3D-noise warp of the query point (amplitudes 0.42 and 0.06 of the spacing at one and six cycles per spacing, as the flat warp). `swellAt` is the flat dynamic topography with 3D noise.
- **Euler vectors** (`setEuler`): ω_p = pole · v_p / (R √⟨sin² γ⟩_p) over the plate's pieces at the epoch's entry. `PlateSpeeds::Today` draws v_p as the flat history does (4–30 mm/yr); `PlateSpeeds::Earth` gives mix(65.5, 22.8, c_p) mm/yr × U(0.7, 1.3), c_p the continental share (Argus et al. 2011; Forsyth & Uyeda 1975).
- **Applied vectors** (`planet/fn/Rotate.cpp` `applyKinematics`, once per planet step): ω̃_p = activity · ω_p / (1 + resistance/JamScale)², the jam decaying over 30 Myr; then NNR from the pieces as they stand, `withoutNetRotation` solving M ω_net = Σ I_p ω̃_p exactly (3 × 3, Cramer), ω̂_p = ω̃_p − ω_net. `JamScale` is the flat 2.5e6 m·cells in m³ (× `PlanetCellArea`), retuned in stage 4.
- **Pieces** (`planet/type/Globe.hpp`): `CrustPiece{stock (m³ felsic incl. sediment, m³ basalt, double), column (float crust = felsic / area, the intensive fields), record, rotation Q, key, birth, area, plate}`; `PieceKey{step (-1 primordial), birth cell, order}`. Pieces stay in key order (`sortPieces`); new pieces append with a later step, dead ones are compacted out stably, so storage order is key order and no free list is needed (deviation from §4's free list: the same determinism, less state).
- **Primordial body** (`makeGlobe`): one piece per cell, Q = I, `primordialColumnAt` (the flat cooling with 3D noise at the flat noise's 4 194 km wavelength), basalt 7 km × area (`OceanicCrust`, White et al. 1992), marks from the closed-form past.
- **Rotation** (`rotatePieces`): one `rotationOf(ω̂_p, dt)` per plate composed on the left of Q and renormalized; `substepCount` = max(1, ⌈|v_rel| dt / shortest edge⌉). `materialOf(piece, x)` = R Q⁻¹ x, the 3D material point of S3m/S3e.
- **Coupling** (`couple`): per piece either exact (S3a `candidateCells` + `overlapArea`, the sum before normalisation required to 1e-9 of the area, a fatal `require` in every build) or cloud-in-cell (the piece as one cell of its centre's face coordinates over up to four cells of that face, clamped at the face edge, normalised to the area). Per-cell lists are in key order. `viewOf` normalises over the covered part; `dominantPiece` picks the largest weight.

## Verify

```
cd @simulation && X=$(ls -d ~/.local/share/mise/installs/github-xmake-io-xmake/*/ | head -1)xmake && export XMAKE_CONFIGDIR=$PWD/../.out/s3b/xmake && $X config --yes --mode=release --builddir=../.out/s3b/build && $X build --yes simulation-tests terra
../.out/s3b/build/macosx/arm64/release/simulation-tests globe geometry atmospheresphere
```
