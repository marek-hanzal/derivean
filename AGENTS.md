# DeRivean agent contract

DeRivean is a desktop voxel physics sandbox. The public name is DeRivean and the code namespace is `derivean`. Use the smallest explicit design and keep ownership visible.

- Root directories prefixed with `@` are independently owned modules. Unprefixed directories hold repository documentation and resources.
- [Architecture](docs/architecture.md) maps runtime, world, and display ownership. Read only the contracts the task touches.
- C++ owns gameplay decisions and authoritative state. Electron owns platform integration. Three.js builds and draws projections. An editor can eventually consume the engine through its own desktop composition.
- Model C++ data as structs with free functions. Use classes for actual lifecycle and resource invariants, including third-party handles and native bindings. Prefer libraries for general-purpose collision, physics, mesh rendering, and platform behavior.
- Put domain code in `src/<domain>/<kind>/<file>` where the kind describes what the file owns, such as `type`, `fn`, `constant`, `service`, or `ui`. Framework entry points and foreign-function bindings keep their framework layout. Import the owning implementation directly and avoid forwarding layers.
- Put cross-owner invariants and setup in documentation. Keep each fact with one owner. Comments explain an invariant or a design constraint. History belongs in Git.
- Examine handwritten source files above 250 lines. Split owners that mix responsibilities. If a longer owner is cohesive, document the reason and line count at its top.
- Use module mise tasks and xmake for the native build. Keep generated files under `.out/` except standard package and framework build directories. Put UI verification artifacts under `.out/playwright/`.
- Preserve unrelated workspace changes. Validate focused behavior before wider checks and report unverified platforms or workloads.
- For substantial changes, run an independent reviewer sub-agent on code and architecture before delivery. The implementer does not approve its own work.
