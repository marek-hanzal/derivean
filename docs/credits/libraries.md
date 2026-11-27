# Credits: engine, libraries and tools

Part of [Credits](../credits.md). A citation credits the people whose work we build on; a licence is what allows us to ship their code. This file records both for the code and tools DeRivean uses. Licences were read from each project's LICENSE file on 26 Nov 2026; versions are the ones pinned in the repository.

| Project | Authors / maintainers | Licence | Version used | Pinned in | What it does here |
| --- | --- | --- | --- | --- | --- |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | Jorrit Rouwe and contributors | MIT | 5.7.0 | `@engine/xmake.lua:1` | Rigid-body collision and physics of the engine |
| [three.js](https://threejs.org) | Ricardo Cabello (mrdoob) and the three.js authors | MIT | 0.187.0 | `@game/package.json:14` | WebGPU scene, meshes and shaders of the game |
| [Electron](https://www.electronjs.org) | Electron contributors, GitHub Inc.; an OpenJS Foundation project | MIT | 45.5.0 | `@game/package.json:19` | Desktop shell and platform integration |
| [Node.js](https://nodejs.org) | Node.js contributors (OpenJS Foundation) | MIT (bundled components carry their own) | 24.22.1 | `mise.toml:2` | Runtime of the tools and the Electron main process |
| [node-addon-api](https://github.com/nodejs/node-addon-api) | Node-API collaborators (Node.js project) | MIT | 8.9.2 | `@napi/package.json:6` | C++ wrapper of Node-API for the native addon |
| [node-api-headers](https://github.com/nodejs/node-api-headers) | Node.js project | MIT | 1.7.0 | `@napi/package.json:7` | Node-API headers the addon compiles against |
| [TypeScript](https://www.typescriptlang.org) | Microsoft | Apache-2.0 | 5.9.3 | `@game/package.json:20` | Type checking of the game |
| [@webgpu/types](https://github.com/gpuweb/types) | W3C GPU for the Web Community Group and contributors (gpuweb) | BSD-3-Clause | 0.1.71 | `@game/package.json:18` | Types of the raw WebGPU API the map draws with, `@game/src/map/service/ChartSurface.ts:1` |
| [Vite](https://vite.dev) | Evan You, VoidZero Inc. and Vite contributors | MIT | 8.3.8 | `@game/package.json:21` | Build of the renderer |
| [Clipper2](https://github.com/AngusJohnson/Clipper2) | Angus Johnson | BSL-1.0 | 1.5.4 | `@simulation/xmake.lua:12` | Integer polygon union with holes (PolyTree64) of crust footprints in the planet's boundary band, `@simulation/src/geometry/fn/Union.cpp:126` |
| [xmake](https://xmake.io) | ruki and xmake contributors (xmake-io) | Apache-2.0 | 3.1.2 | `@napi/mise.toml`, `@simulation/mise.toml` | Native build of `@simulation`, `@engine` and `@napi` |
| [mise](https://mise.jdx.dev) | Jeff Dickey (jdx) | MIT | (developer's install) | `mise.toml` | Tool versions and module tasks |
| [LLVM](https://llvm.org) (clang-cl, llvm-lib) | LLVM Developer Group and contributors | Apache-2.0 with LLVM exception | 23.1.5 | `@simulation/mise.toml`, `@napi/mise.toml` | C++ compiler on Windows ([Windows](../windows.md)); on macOS Xcode's clang |
| [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/) (MSVC STL, Universal CRT, Windows SDK, `link.exe`) | Microsoft | Microsoft Software License Terms; the STL is Apache-2.0 with LLVM exception | 17.14, MSVC 14.44, SDK 10.0.22621 (developer's install) | — | Standard library, C runtime, platform headers and linker of the Windows build |

## Studied, not linked

| Project | Authors | Licence | What we took |
| --- | --- | --- | --- |
| [S2 Geometry](https://github.com/google/s2geometry) | Google (Eric Veach et al.) | Apache-2.0 | Face numbering, orientation and quadtree conventions, reimplemented in `@simulation/src/geometry`; no code copied, the library is not linked (`docs/handoff/geometry.md:55`) |
| [PROJ](https://proj.org) | PROJ contributors (OSGeo; originally Gerald Evenden, Frank Warmerdam) | MIT-style | The QSC projection was compared and not taken (`docs/handoff/geometry.md:97`) |
| [node-gyp](https://github.com/nodejs/node-gyp) | Node.js contributors | MIT | Its `win_delay_load_hook.cc` binds a delay-loaded `node.exe` to the host executable; `@napi/src/windows/HostImports.cpp` does the same in its own code |

No code from these projects is in the repository, so none of their licences binds the build; they are listed as credit.

## Platform documentation

| Document | Publisher | What it informs |
| --- | --- | --- |
| [Arm Neon Intrinsics Reference](https://arm-software.github.io/acle/neon_intrinsics/advsimd.html), part of the [Arm C Language Extensions](https://github.com/ARM-software/acle) | Arm Limited, 2014–2026, read 27 Nov 2026 | `vld1q`, `vextq`, `vfmaq` (FMLA) and `vst1q` in `f64` and `f32` in the arm64 kernels of the shared stiffness product, double and float32, `@simulation/src/atmosphere/fn/StencilNeon.hpp:21`, `:53` |
| [Clang Compiler User's Manual](https://clang.llvm.org/docs/UsersManual.html), `-ffp-contract` | LLVM Project, web edition read 27 Nov 2026 (Xcode's clang 21 here) | When clang fuses a multiply and an add: `on`, the default for C and C++, fuses them within one statement; the floating-point rule of [Windows](../windows.md#determinism), and why the arm64 kernel's fused sums equal the scalar ones (`@simulation/src/atmosphere/fn/StencilMatrix.hpp:93`, and why the seam and tail sums stay scalar (`:27`)) |
| [/arch (x64)](https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64) | Microsoft | `/arch:AVX2` enables AVX2 and FMA3 code generation (`@simulation/xmake.lua`) |
| [Linker support for delay-loaded DLLs](https://learn.microsoft.com/en-us/cpp/build/reference/linker-support-for-delay-loaded-dlls) and [delay load hooks](https://learn.microsoft.com/en-us/cpp/build/reference/understanding-the-helper-function) | Microsoft | `/DELAYLOAD`, `__pfnDliNotifyHook2` and `dliNotePreLoadLibrary` in `@napi/src/windows/HostImports.cpp` |
| [GetLogicalProcessorInformationEx](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformationex) and [PROCESSOR_RELATIONSHIP](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-processor_relationship) | Microsoft | Efficiency classes of cores, `@engine/src/threads/fn/Threads.cpp` |
| [WebGPU](https://www.w3.org/TR/webgpu/) and [WGSL](https://www.w3.org/TR/WGSL/), Candidate Recommendation drafts | W3C GPU for the Web Working Group (eds. Kai Ninomiya, Brandon Jones, Jim Blandy; Alan Baker, Mehmet Oguz Derin, David Neto), read 27 Nov 2026 | The map's device, textures, passes and shaders, `@game/src/map/service/ChartSurface.ts`, `@game/src/map/fn/chartShader.ts`; that `fwidth` works on 2×2 quads, needs uniform control flow unless its diagnostic is off, and `textureLoad` on unfilterable `rgba32float` ([map](../handoff/map.md)) |
| [Node-API](https://nodejs.org/api/n-api.html) | Node.js project | The addon's binary interface; on Windows its exports are imported from the host executable |

## Data

No third-party dataset is bundled or read at run time. The datasets and measurements the planet is compared with are cited under [reference data](reference-data.md); images on Wikimedia Commons linked from `docs/reference-landscapes.md` are linked, not copied, and keep their own licences.
