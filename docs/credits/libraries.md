# Credits: engine, libraries and tools

Part of [Credits](../credits.md). A citation credits the people whose work we build on; a licence is what allows us to ship their code. This file records both for the code and tools DeRivean uses. Licences were read from each project's LICENSE file on 26 Nov 2026; versions are the ones pinned in the repository.

| Project | Authors / maintainers | Licence | Version used | Pinned in | What it does here |
| --- | --- | --- | --- | --- | --- |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | Jorrit Rouwe and contributors | MIT | 5.7.0 | `@engine/xmake.lua:1` | Rigid-body collision and physics of the engine |
| [three.js](https://threejs.org) | Ricardo Cabello (mrdoob) and the three.js authors | MIT | 0.187.0 | `@game/package.json:14` | WebGPU scene, meshes and shaders of the game |
| [Electron](https://www.electronjs.org) | Electron contributors, GitHub Inc.; an OpenJS Foundation project | MIT | 45.5.0 | `@game/package.json:18` | Desktop shell and platform integration |
| [Node.js](https://nodejs.org) | Node.js contributors (OpenJS Foundation) | MIT (bundled components carry their own) | 24.22.1 | `mise.toml:2` | Runtime of the tools and the Electron main process |
| [node-addon-api](https://github.com/nodejs/node-addon-api) | Node-API collaborators (Node.js project) | MIT | 8.9.2 | `@napi/package.json:6` | C++ wrapper of Node-API for the native addon |
| [node-api-headers](https://github.com/nodejs/node-api-headers) | Node.js project | MIT | 1.7.0 | `@napi/package.json:7` | Node-API headers the addon compiles against |
| [TypeScript](https://www.typescriptlang.org) | Microsoft | Apache-2.0 | 5.9.3 | `@game/package.json:19` | Type checking of the game |
| [Vite](https://vite.dev) | Evan You, VoidZero Inc. and Vite contributors | MIT | 8.3.8 | `@game/package.json:20` | Build of the renderer |
| [xmake](https://xmake.io) | ruki and xmake contributors (xmake-io) | Apache-2.0 | 3.1.2 | `@napi/mise.toml`, `@simulation/mise.toml` | Native build of `@simulation`, `@engine` and `@napi` |
| [mise](https://mise.jdx.dev) | Jeff Dickey (jdx) | MIT | (developer's install) | `mise.toml` | Tool versions and module tasks |

## Studied, not linked

| Project | Authors | Licence | What we took |
| --- | --- | --- | --- |
| [S2 Geometry](https://github.com/google/s2geometry) | Google (Eric Veach et al.) | Apache-2.0 | Face numbering, orientation and quadtree conventions, reimplemented in `@simulation/src/geometry`; no code copied, the library is not linked (`docs/handoff/geometry.md:55`) |
| [PROJ](https://proj.org) | PROJ contributors (OSGeo; originally Gerald Evenden, Frank Warmerdam) | MIT-style | The QSC projection was compared and not taken (`docs/handoff/geometry.md:97`) |

No code from either project is in the repository, so neither licence binds the build; they are listed as credit.

## Platform documentation

| Document | Publisher | What it informs |
| --- | --- | --- |
| [Arm Neon Intrinsics Reference](https://arm-software.github.io/acle/neon_intrinsics/advsimd.html), part of the [Arm C Language Extensions](https://github.com/ARM-software/acle) | Arm Limited, 2014–2026, read 27 Nov 2026 | `vld1q_f64`, `vextq_f64`, `vfmaq_f64` (FMLA) and `vst1q_f64` in the arm64 kernel of the shared stiffness product, `@simulation/src/atmosphere/fn/StencilMatrix.cpp:47` |
| [Clang Compiler User's Manual](https://clang.llvm.org/docs/UsersManual.html), `-ffp-contract` | LLVM Project, web edition read 27 Nov 2026 (Xcode's clang 21 here) | `on`, the default for C and C++, fuses a multiply and an add within one statement; the arm64 kernel's fused sums equal the scalar ones because of it (`@simulation/src/atmosphere/fn/StencilMatrix.hpp:86`) |

## Data

No third-party dataset is bundled or read at run time. The datasets and measurements the planet is compared with are cited under [reference data](reference-data.md); images on Wikimedia Commons linked from `docs/reference-landscapes.md` are linked, not copied, and keep their own licences.
