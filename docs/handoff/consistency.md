# Hand-over: level consistency (lakes, rivers, tile seams)

Describes the present state of the level-consistency stream. Rewrite it when the state changes; history belongs in Git.

## 1. Goal

Lakes, rivers and tile seams are the same features at every level and across every tile edge:

1. A lake the parent level has stays a lake, with its level and shore, at the finer levels, unless a simulated process of the finer level (infill, outlet incision) removes it, and then the process is named. The owner wants a lake with a sandy beach that can be found (`terra coast --find 1`).
2. A river the parent has enters a fine tile where the parent's river crosses the tile's edge, with the parent's discharge, and leaves where the parent's leaves. Region rivers keep their water across Region tile edges.
3. No steps or straight lines along tile edges (Local: coast, ice, relief all report them).
4. Lake cover at Region (15 % of land against Earth's 2–4 %): measured after 1–3, what remains said.

Owned: `@simulation/src/landscape/**` files the relief stream is not changing (not `fn/{Uplift,Fluvial,Stability,Talus,Hillslope,Glacial}.cpp`, `Process.hpp`), `@simulation/test/LevelConsistency.cpp`. Fixes in relief-owned files go to the lead as diffs.

## 2. State now

Started 26 Nov. Reading the model; nothing changed in code yet.

## 3. In progress

Reading `Build.cpp` (`levelHollows`), `Seed.cpp`, `Flood.cpp`, `Extract.cpp`, `Replay.cpp`.

## 4. Next steps

1. Mechanism of lakes vanishing at Local/Site (seed 42, `lake3`, `lake4` of `.out/coast/find2/run.txt`).
2. Rivers across Region tile edges and from Region into Local.
3. Seams (`terra measure seams`).
4. Lake share at Region.

## 5. Decisions and why

None yet.

## 6. Contracts with other streams

None yet.

## 7. How to verify

To be written with the first fix.

## 8. Open questions

None yet.
