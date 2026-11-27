# Windows port hand-over

Owner of this file: the Windows port (`windows-port` stream). It describes the present; the rules it found are in [Windows](../windows.md), which owns them. Paths are relative to the repository root. Measurements and the scripts that made them are under `.out/windows-port/` in the worktree `.claude/worktrees/inspiring-snyder-8bbf5c`; on the Windows machine everything lives in `C:\Users\Marek Hanzal\derivean-windows-port` (sources shipped as an archive of the worktree, nothing pushed, nothing installed or configured outside that directory).

## 1. Goal

DeRivean builds and runs on Windows x86-64 next to macOS, and the authoritative simulation gives the same bits on both from the same commit.

## 2. State now

Written 2026-11-27 (branch `claude/inspiring-snyder-8bbf5c`, on `026816c`).

Done:
- Build: `macosx` and `windows` are the allowed platforms, the Linux branch is gone. Windows builds with clang-cl (LLVM 23.1.5) against Visual Studio 2022 Build Tools (MSVC 14.44, SDK 10.0.22621); `@simulation`, `@engine`, Jolt 5.7.0 and the addon build there.
- Floating point: clang on both platforms with `-ffp-contract=on`, and `/arch:AVX2` on Windows, public on the simulation target; MSVC `cl` is refused at configure time. macOS output is unchanged by it (planet digests identical before and after).
- Addon: Node-API import library from `node-api-headers`' `.def`, delay-loaded, bound to the host by `@napi/src/windows/HostImports.cpp`.
- Platform code: `Threads.cpp` (P-cores, game-thread priority), `terra`'s CPU time and peak memory.
- Docs: [Windows](../windows.md) (setup, build, determinism), README, the architecture's sentence on thread priority, credits (LLVM, VS Build Tools, node-gyp, platform documentation).

Verified on the Windows machine (i9-13900K, 8 P + 16 E cores, Windows 10.0.26300):
- `simulation-tests`, `terra` and `engine-tests` compile and link; the addon links and loads in `node.exe` 24.22.1 and in Electron 45.5.0's `electron.exe` (`ELECTRON_RUN_AS_NODE`), on the main thread and in a worker (`.out/windows-port/windows-scripts/electron-load.cjs`).
- `@game`: `npm ci` and `vite build` pass.
- Planet history, seed 42, 1 Myr to 4.6 Gyr: bit-identical with macOS (table in section 3).

Broken or unverified:
- **Tiles differ between the machines** (section 3): the platform libm. Until the simulation has its own transcendental functions, Windows plays a different world for the same seed.
- `simulation-tests` on Windows: see section 4; the failures so far are the tests that also fail on macOS when its rounding is perturbed.
- `engine-tests` fail on both machines, at different checks. Windows, final build: "Far cubes and voxels disagree about what the ground is made of" (17 040 of 24 523 finest cube tops of the top voxel's material, 9 327 of them stones; 1 261 s). macOS, same commit: that check passes (29 623 of 31 075, 1 424 stones) and the suite stops later at "A waiting body must come to rest on the returned ground". The Windows world is a different world (the libm, section 3), so its far cubes are checked against other ground; the macOS failure does not depend on this branch, whose only macOS change is a flag that leaves the bits as they were (`.out/windows-port/mac-engine-tests.log`, `.out/windows-port/windows/engine-tests-final.log`).
- Bridge tests (`node --test @napi/test/*.test.cjs`) on Windows: see section 4.
- Module mise tasks on Windows: `mise -C @simulation run build` and `run test` (through `@simulation/test/groups.mjs`, five groups side by side, pass and fail as on macOS), `mise -C @napi run build`; trusted per process (`MISE_TRUSTED_CONFIG_PATHS`, `.out/windows-port/windows-scripts/mise-test.cmd`), not through `mise trust`, which would change the machine's mise configuration.
- Not run: the root tasks (`mise run setup`, `check`, `play`) on Windows, since the root `mise.toml` installs Node through mise and the machine's Node is its own (the same 24.22.1); the game window (an ssh session has no desktop; Codex starts visible runs through a scheduled task), Electron packaging (none exists on either platform).

## 3. Measurements

Digests come from a probe outside the tree (`.out/windows-port/probe/probe.cpp`, built against `@simulation` by `.out/windows-port/probe/xmake.lua`): `digestOf(Planet)` after `evolvePlanet(42, age)` for a ladder of ages, then a `World` at 4.6 Gyr and `digestOf(Tile)` for the Region, Local, Site and Ground tiles at `findShore(planet, {0, 0})`.

Planet digest, seed 42:

| Age | macOS (FMA) | Windows `/arch:AVX2` | macOS `-ffp-contract=off` | Windows x86-64 baseline |
| --- | --- | --- | --- | --- |
| 1 Myr | 0328703f04215297 | 0328703f04215297 | 0328703f04215297 | 0328703f04215297 |
| 10 Myr | 80c65eb89a6f9c72 | 80c65eb89a6f9c72 | b7b01e1e1f482f3d | b7b01e1e1f482f3d |
| 1 Gyr | 1e1c9540d3050206 | 1e1c9540d3050206 | efce26a67c9ce8e1 | efce26a67c9ce8e1 |
| 4.6 Gyr | 26f313fade943f31 | 26f313fade943f31 | b0c66810c7e0e6d7 | b0c66810c7e0e6d7 |
| sea level at 4.6 Gyr | 53.7311526 m | 53.7311526 m | 45.3354435 m | 45.3354435 m |

Fusion decides the planet; the two machines agree whenever both fuse or neither does (all ages in `.out/windows-port/mac/planet-*.txt` and the Windows logs).

Tile digests (ground / water / flow), seed 42, 4.6 Gyr, shore at (3 209 212.65, 930 934.83), final build on both:

| Tile | macOS | Windows |
| --- | --- | --- |
| L1 12 3 | c15ad4a9f3c0a436 / d0a07048b0442f1a / 8d1baf682fef2df1 | 02dc2a4a29e5a474 / 0e7961041bf3d86e / 7508b83ec6fadb08 |
| L2 195 56 | 34f066c0362f386d / 5b9727fcd45c6844 / 884957ccdb45b845 | a29557b83f16b5f8 / d24a4d7c1dd96adb / 90cc9d91dddf1f9c |
| L3 3133 909 | 4b3e51cfa8adb0aa / 4e9ce93d713e7d1c / f9a79cdfc25ca50b | 751f604809c531b3 / aa52b4bb511b0b39 / 26efc21fff585a62 |
| L4 50143 14545 | b7ec2ddb05adc7a4 / 68688e96ae95ca81 / ba46958586b40ec0 | stopped before it finished, to free the machine for the suite |

Without fusion on both (planet b0c66810c7e0e6d7, shore at (2 854 305.09, 1 349 759.54)) the tiles differ too: L1 ground 3fac85e436ae852a against 6813f1940bb30cf0; L2 water agrees (c00173bb7ebe16be), L2 ground and flow do not.

libm against libm (`.out/windows-port/libm/`): 200 000 inputs per function and range, built with contraction off, results compared bit for bit between Apple's libm and the Universal CRT.

| Function | Differ | Max ulp | Function | Differ | Max ulp |
| --- | --- | --- | --- | --- | --- |
| sin / sinf [−10, 10] | 5.0 % / 1.4 % | 1 / 1 | exp / expf | 0.5 % / 0.6 % | 1 / 1 |
| cos / cosf | 4.8 % / 1.2 % | 1 / 1 | exp2 / exp2f | 38 % / 38 % | 2 / 2 |
| tan / tanf | 41 % / 7.0 % | 3 / 1 | log / logf | 0.004 % / 0.009 % | 1 / 1 |
| asin / asinf | 9.1 % / 6.7 % | 1 / 1 | log2 / log2f | 0.011 % / 0.62 % | 1 / 1 |
| acos / acosf | 17 % / 2.4 % | 1 / 1 | log10 / log10f | 0.004 % / 0.082 % | 1 / 1 |
| atan / atanf | 3.7 % / 1.6 % | 1 / 1 | pow / powf | 0.25 % / 0.6 % | 1 / 1 |
| atan2 / atan2f | 1.0 % / 5.8 % | 1 / 1 | cbrt / cbrtf | 31 % / 32 % | 2 / 2 |
| sinh / sinhf | 14 % / 0.7 % | 1 / 1 | erf / erff | 35 % / 20 % | 4 / 4 |
| tanh / tanhf | 0.9 % / 0.9 % | 1 / 1 | erfc / erfcf | 47 % / 37 % | 9 / 5 |
| hypot / hypotf | 5.4 % / 0 | 1 / 0 | sqrt, fmod (both) | 0 | 0 |

One maths library on both machines (`.out/windows-port/probe/samemath.cpp`): exp, log, pow, sin, cos, tan, the inverse and hyperbolic functions, cbrt, hypot, erf and erfc written from +, −, ×, ÷, sqrt and bit operations, double and float, linked into the probe in place of the platform's (and, on Windows, its `__imp_` pointers, since the UCRT headers declare these functions dllimport), every simulation source built with `-fno-builtin-<name>` for them. `nm -u` on macOS and `llvm-readobj --coff-imports` on Windows show no libm function left. Accuracy against Python's `math`: about 2e-16 relative, erfc 5e-12. Result, seed 42, 4.6 Gyr, same shore as above:

| Tile | macOS | Windows |
| --- | --- | --- |
| planet | 26f313fade943f31 | 26f313fade943f31 |
| L1 12 3 | 994a18ccf3dad8fb / 3f528756339bd2a7 / d05275df7b648814 | 994a18ccf3dad8fb / 3f528756339bd2a7 / d05275df7b648814 |
| L2 195 56 | 0e0e5d1aedfe3461 / 4e15706fcb288981 / 4bc569b558f8026d | 0e0e5d1aedfe3461 / 4e15706fcb288981 / 4bc569b558f8026d |
| L3 3133 909 | 4ade9ab992f3edc5 / e8af4d1bec997e8c / d6395e1b6fe08e2d | 4ade9ab992f3edc5 / e8af4d1bec997e8c / d6395e1b6fe08e2d |
| L4 50143 14545 | 08eead102a71d375 / 3852457e6882003f / 6cd906a002ccb3cf | 08eead102a71d375 / 3852457e6882003f / 6cd906a002ccb3cf |

So with the floating-point rule and one maths library the chain is bit-identical; the platform libm is the whole remaining difference. The planet digest is the same with either library: the planet history does not depend on these functions.

## 4. Tests

`simulation-tests` on Windows, final build, one test per process (`.out/windows-port/windows-scripts/tests.ps1`, results in `.out/windows-port/windows/final/`): every test from BuildGraph to ReliefBeds passes except two.

| Test | Windows | macOS, same commit | macOS without fusion |
| --- | --- | --- | --- |
| Atmosphere | fails: the temperate plain's sea-level rung is not snowy in winter | passes | fails |
| Relief | fails (226.035) | fails (211.137, before this branch too) | fails |
| ReliefGlaciatedSite | still running | fails (before this branch too) | fails |
| ReliefLedger | still running | passes | fails |

Both Windows failures are calibrated checks that a perturbed rounding also breaks on macOS: the Windows world is a different world until the libm is shared (section 3).

Bridge tests (`node --test @napi/test/*.test.cjs`) on Windows, under the suite's load: the first and third pass; "a chart around the player reads the tiles under them…" hit the 900 s timeout and needs a run on a quiet machine.

## 5. Next

1. **The simulation's own transcendental functions** (the one thing between the port and identical worlds). Every `std::sin`, `exp`, `pow`, `atan2`, `cbrt`, `erf`, `tanh`, … in `@simulation/src` (about 600 calls) goes through one header of the simulation's own, compiled from source on both platforms. The robust choice is correctly rounded functions, whose result is defined by the input alone however they are compiled: [CORE-MATH](https://core-math.gitlabpages.inria.fr) (Sibidanov, Zimmermann, Glondu; MIT) covers every binary32 function and the binary64 ones the simulation calls. A plain port of an existing libm (FreeBSD msun, musl) would also agree under the floating-point rule, but only as long as both compilers contract alike. The change touches files of every stream and changes every world on macOS as well (Apple's libm leaves the build), so calibrated tests move like `testAtmosphere` and `testRelief` did without fusion; it needs a quiet tree, the lead's slot, and a credit entry. Measure its cost on the planet and on a tile first.
2. **A cross-machine check in the tree.** The probe belongs in `terra` (for example `terra digest`), so that the lead compares one line per level from both machines after a merge or a compiler upgrade.
3. The engine is not bit-identical across machines and was not asked to be: Jolt is built without `cross_platform_deterministic`, and the engine's own floating point is outside the simulation's rule only in the sense that it is not checked. It matters once two machines must agree on a session (shared worlds, replays).
4. Windows' game window: start `mise run play` through an interactive session on the machine (as Codex's scheduled-task runs do) once the lead allows it.
5. `@simulation/tool/GroundCommand.cpp:153` reads `tile->now.sediment[Sand]` where `Sand` is the tool's own cover kind 7, not the sediment class: an out-of-bounds read in a diagnostic (clang warns `-Warray-bounds` on both platforms). Not a platform matter; left for the tool's owner.
6. The lead's [README](README.md) still says the build allows `macosx` and `linux` only.
