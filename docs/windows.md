# Windows

DeRivean supports macOS on Apple Silicon and Windows on x86-64; Linux is not supported. This file owns what the Windows build needs and the rules that keep the simulation bit-identical on both platforms. The measurements behind it are in the [hand-over](handoff/windows.md).

## Setup

You need:

- Windows 10 or 11, x86-64, on a processor with AVX2 and FMA3: Intel Haswell (2013) or later, AMD Excavator (2015) or Zen or later. The build requires these instructions because the simulation's rounding depends on them ([determinism](#determinism)).
- Visual Studio 2022 Build Tools with the *Desktop development with C++* workload (MSVC v143 and a Windows 10/11 SDK). clang-cl compiles against its standard library, the Universal CRT and the SDK, and links with its `link.exe`. Neither the IDE nor `cl.exe` on the `PATH` is needed; xmake finds the installation through `vswhere`.
- [mise](https://mise.jdx.dev) and Git. mise installs Node, xmake (`xmake-bundle-v3.1.2.win64.exe`) and LLVM 23.1.5 (`clang+llvm-23.1.5-x86_64-pc-windows-msvc.tar.zst`, for clang-cl) from the versions and checksums pinned in [`@simulation/mise.toml`](../@simulation/mise.toml) and [`@napi/mise.toml`](../@napi/mise.toml).

Then, from the repository root, the same commands as on macOS:

```sh
mise trust --all
mise run setup
mise run play
```

These are the macOS steps, unchanged; on Windows the native build, its tests, the addon under Node and Electron and the renderer build are verified one by one ([hand-over](handoff/windows.md)), the mise tasks end to end and the game window not yet.

If `@game/node_modules/electron/dist/electron.exe` is missing after `mise run setup`, Electron's install script did not run; `node @game/node_modules/electron/install.js` downloads the binary.

## What the build does on Windows

Each build file explains its own Windows branch; this is the map.

| Part | Windows | Where |
| --- | --- | --- |
| Compiler | clang-cl, x64 only; the simulation target refuses any other compiler | [`@simulation/xmake.lua`](../@simulation/xmake.lua), [`@napi/xmake.lua`](../@napi/xmake.lua) |
| Floating point | the rule of [determinism](#determinism), public on the simulation target | [`@simulation/xmake.lua`](../@simulation/xmake.lua) |
| C runtime | the defines `M_PI` and `<windows.h>` need | [`@simulation/xmake.lua`](../@simulation/xmake.lua) |
| Jolt | built for the instruction set the engine compiles its headers with | [`@engine/xmake.lua`](../@engine/xmake.lua) |
| Addon | `derivean.node` is a DLL whose Node-API imports are bound at load time to the executable that loads it, `electron.exe` or `node.exe` | [`@napi/xmake.lua`](../@napi/xmake.lua), [`@napi/src/windows/HostImports.cpp`](../@napi/src/windows/HostImports.cpp) |
| Threads | P-cores of a hybrid processor for the pools, a raised priority for the game thread | [`@engine/src/threads/fn/Threads.cpp`](../@engine/src/threads/fn/Threads.cpp) |
| Diagnostics | `terra`'s thread CPU time and peak memory | [`@simulation/tool/GroundCommand.cpp`](../@simulation/tool/GroundCommand.cpp), [`@simulation/tool/MeasurePerformance.cpp`](../@simulation/tool/MeasurePerformance.cpp) |
| Electron | nothing platform-specific: paths go through `fileURLToPath`, and no packager is configured on either platform | [`@game/main`](../@game/main) |

The C++ sources use no `std::filesystem`, POSIX threads or other POSIX calls outside the two diagnostics above; standard threads, atomics and `std::chrono` are the same on both platforms. `long` is 32 bits on Windows and 64 on macOS; the simulation uses it only for counters far below 2³¹.

## Determinism

The authoritative simulation must give the same bits on every supported machine and at every thread count. Within one machine that is checked by `terra measure determinism` and the suite. Across machines three things decide it.

**Fused multiply-add.** clang ([manual](https://clang.llvm.org/docs/UsersManual.html), `-ffp-contract`; [credits](credits/libraries.md#platform-documentation)) turns `a * b + c` written within one expression into a fused multiply-add where the target has one, and nowhere else (`-ffp-contract=on`). An Apple Silicon processor always has one; an x86-64 processor has FMA3 only from AVX2 on, and code built for the plain x86-64 baseline rounds the product separately. The two differ in the last bit, and the planet's history amplifies that bit: seed 42 at 4.6 Gyr ends with the sea at 53.73 m with fusion and at 45.34 m without. The rule is therefore clang on both platforms with contraction on (stated explicitly, though it is clang's default) and `/arch:AVX2` on Windows. With it the whole planet history is bit-identical between the two machines from 1 Myr to 4.6 Gyr, and macOS results did not change. The other way, `-ffp-contract=off` everywhere, is equally exact but changes every world on macOS and breaks calibrated tests (`testAtmosphere` and `testReliefLedger`, which pass on macOS with fusion), so it was not taken. MSVC's `cl` is not a substitute: its contraction rules (`/fp:contract`) are its own.

**The C maths library.** `std::sin`, `std::exp`, `std::pow` and the other transcendental functions are not correctly rounded, and Apple's libm and Microsoft's Universal CRT are different implementations. On the same inputs they disagree in up to half the results by 1 to 9 units in the last place, depending on the function; only `sqrt`, `fmod` and, as measured, `hypotf` agree, because the first two are exact by IEEE 754. The planet history does not depend on them, but the tiles do: with the rule above, the Region, Local and Site tiles at seed 42's shore differ between the two machines, and `testAtmosphere` fails on Windows as it does on macOS without fusion. Linked with one implementation of these functions on both machines, the same tiles agree bit for bit, so the platform libm is what remains. Until the simulation computes these functions itself, Windows plays a different world from macOS for the same seed. The cure is in the hand-over: one implementation of every transcendental function the simulation calls, compiled from source on both platforms, preferably correctly rounded so that its results do not depend on how it is compiled.

**What the compiler evaluates.** clang folds a maths call on constant arguments at compile time with the host's own libm, and on macOS it merges `sin(x)` and `cos(x)` of one `x` into `__sincos_stret` and `pow(10, x)` into `__exp10`. A portable maths library must therefore not be reachable under the standard names, or those names must be built with `-fno-builtin-<name>`.

Other sources checked and found harmless: the sorts that order results use total orders or break ties by index, and the one `nth_element` takes a median value; the unordered containers (`Atlas`, `Aquifers`, `BuildKeys`) are looked up, never iterated for results; no `<random>` distribution is used.

Upgrading a compiler on either platform can change any of this. After an upgrade, compare planet and tile digests on both machines from the same commit; the hand-over says how.
