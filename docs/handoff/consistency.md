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

Nothing changed in the shared tree yet. Experiments run in a private mirror (section 7).

Findings (seed 42, 4.6 Gyr, planet of 26 Nov 10:30, sea 53.7 m):

- **Lakes vanish already in the child's starting surface**, before any settling. lake3 area (2 × 2 Local tiles from 319, −365): of the parent lake cells under the block only 5.3 % are lake in the child's start, and the child water stands 39.5 m below the parent's. Two causes in `Seed.cpp`/`Build.cpp`: (1) the smoothing spline of the parent's rock cuts through a lake's rim (a shore cell ≥ the water level is averaged with the lake floor) and raises the floor, so the basin drains far below the parent's level; (2) `levelHollows` then fills every remaining hollow by up to 45 m (Local) / 6 m (Site), parent lakes included, and makes that filled surface the trend the controller holds.
- **Rivers and seam lakes are one defect**: a river the parent has is a one-cell valley the spline erases, so the child grows its own rivers elsewhere (parent rivers found within one parent cell: 41 %). A later tile's river then meets the cells of an earlier neighbour where that neighbour has no valley: the finished cells do not erode and dam it, and a lake backs up along the tile edge (lake3 block: a 15 km lake against a seam where the parent has none; lakes 1.8× denser within 3 cells of an edge). Local crossings lost: 36 % (into earlier 50 %, into later 29 %).
- **Region** (3 × 3 tiles from 18, −24): lakes 0.63 % of land there but 32× denser within 3 cells of a tile edge than inside (thin strips along edges); 37.5 % of river crossings lose > 30 % (into earlier 48 %, into later 34 %), mostly to near zero.

## 3. In progress

Mirror variants (`labVariant` in the mirror only): 1 = `levelHollows` leaves parent lakes; 2 = 1 + bilinear interpolation of the parent's rock within two parent cells of a parent lake; 3 = 2 + the parent's rivers carried into the child seed (`carveParentRivers`: course through parent centres smoothed two cells up and down, bed at the parent's ground, banks 0.25). Being compared at lake3; Region seam cases being dumped.

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
