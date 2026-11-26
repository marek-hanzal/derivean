# The simulated world

[`@simulation`](../@simulation) owns the natural world: its data, physical properties, history and spatial queries. It has no dependency on the voxel engine, the renderer or Electron; the `terra` command line in [`@simulation/tool`](../@simulation/tool) and the tests read the same world the game does.

A world is a function of the model version, a seed, a position and an age. Nothing in it depends on the order in which places are asked for, the number of threads, or whether a place was visited before.

## Space and time

Positions are metres: X east, Z south, Y up. `y = 0` is the reference ocean datum; the actual sea level follows from the water the planet holds and the shape of its basins. The world is flat and repeats after 16 777 km on both axes ([`Scale.hpp`](../@simulation/src/planet/constant/Scale.hpp)); a finite surface is what closes the water, salt and rock budgets. The playable solid ends at `WorldFloor` (−8 192 m); below it is the void.

The **era** counts years from the hot initial body ([`Timeline.hpp`](../@simulation/src/chronology/constant/Timeline.hpp)). The **calendar** is the time that passes during play. Climate bands follow a triangle wave along Z in place of latitude; the origin lies at 48° north.

## Levels

| Level | Cell | Owner | What it is |
| --- | --- | --- | --- |
| Planet | 65.5 km, 256² cells | [`planet`](../@simulation/src/planet) | Whole-planet history from the initial body to the chosen age, in steps of 2 Myr |
| Region | 1 024 m, 262 km tiles | [`landscape`](../@simulation/src/landscape) | Replay of the last 24–32 Myr from the state the planet recorded at the start of that window |
| Local | 64 m, 16 km tiles | `landscape` | Relief of its own size settled on the region's surface |
| Site | 4 m, 1 km tiles | `landscape` | Relief of its own size settled on the local surface |
| Ground | 0.25 m, 64 m tiles | `landscape`, [`ground`](../@simulation/src/ground) | The surface a person stands on, settled on the site surface: rills, steps over beds, stream beds, and on it the stones, sorted ground and spring deposits of its scale |
| Form | any | [`relief`](../@simulation/src/relief) | What a column of voxels is made of under the ground surface, and the rock shapes a height field cannot hold: ledges, overhangs, joint slots, towers, arches, seams, veins, caves |

A tile is solved together with a 32-cell halo. Tiles are solved in four passes by the parity of their index ([`Level.hpp`](../@simulation/src/landscape/type/Level.hpp)): a tile waits only for neighbours of an earlier pass, whose cells then move along a straight path from their starting state to that neighbour's result. This makes a tile a pure function of its key while rivers and slopes meet on every shared edge. Water may leave a domain only where the coarser level's rivers leave it, so neighbouring tiles agree on the direction of drainage.

A region replays dated time. The finer levels do not: relief of their size has no memory of where it started, so each is integrated until its valleys, benches and debris stand in balance with the parent's surface and the erosion of its own scale. The rock uplift that holds the averaged surface on the parent's is found during that integration ([`Replay.cpp`](../@simulation/src/landscape/fn/Replay.cpp)). High ground far above its outlet needs a fast rise and becomes steep and dissected; ground near its base level stays gentle. The layering is not moved by this: it stays where the region left it and the relief is cut into it.

## The ground level

The ground level runs on the same machinery as the levels above it (24 steps of 1 000 years), with what differs at a quarter of a metre made explicit:

- **Rock.** A cell does not meet a bed but one layer of it. [`Partings.cpp`](../@simulation/src/ground/fn/Partings.cpp) divides every bed along bedding planes, 0.1 m apart in shale and 2.5 m in granite, into layers of differing strength, some with a seam of weak rock at the base. The joints that cut it are those of its rock mass ([`RockMass.hpp`](../@simulation/src/rockmass/type/RockMass.hpp)), the same planes every level reads: a cell is weaker by the share of it they break. Veins and pipes that cut the beds count as rock of their own. Weathering and incision follow those traits, so strong layers and joint blocks are left standing as ledges, steps and knobs where the mantle is thin, and weak ones are stripped.
- **Water.** Running water cuts from 60 m³/yr up (a site cell is far wider than a rill and cuts none), and up to ten times harder the less of it a cell gathers: a slope's water comes in storms, at many times the yearly mean the discharge stands for. That gain is for loose cover only. Rock the water cuts at a twelfth of what the same law gives a wider cell: the parent has already struck the balance between the river and its bed, and a ground tile only steps the bed over a strong layer. A stream works its bed across its whole hydraulic width, not only along the line it is routed by ([`Settle.cpp`](../@simulation/src/ground/fn/Settle.cpp)).
- **Rise and fall.** The rise that holds the surface on the parent's is found as at the other finer levels, but a ground tile's rock never sinks: what is laid on it stands above the parent's surface until the water takes it on. A face the parent holds is held; rock breaks only where it stands steeper than both its own limit and the parent's surface there.
- **Creep.** Soil moves by a twentieth of the diffusivity a wider cell sums up; freezing ground makes more waste, by the share of the year it spends in the frost-cracking window, and more of it as blocks.
- **Ice and standing water** are the parent's: a tile of 64 m is too small to hold a glacier or a lake by itself.
- **Neighbours.** A ground tile takes the cells of its finished neighbours as they ended from its first step, not along a path to there: what crosses its edges are rills and channels that grow headward, and they need the whole settling time to grow on into it. A river that reaches a neighbour's cell beside the course the neighbour had for it is passed on, not replaced by what the neighbour recorded there.

When the surface has settled, [`Finish.cpp`](../@simulation/src/ground/fn/Finish.cpp) makes what lies on it, in this order:

1. **Frozen ground** ([`Frost.cpp`](../@simulation/src/ground/fn/Frost.cpp)). Where the annual mean at the ground is below 2 °C the thawed layer flows, faster where it is thick, and piles into lobes; below 0.5 °C stones and fines trade places between neighbouring cells until they stand apart as borders and centres about three metres across, drawn out into stripes on a slope.
2. **Stones.** The part of the coarse layer that is larger than a voxel becomes individual stones ([`Clast.hpp`](../@simulation/src/ground/type/Clast.hpp)): a size and shape, an attitude, the rock of the bed it broke from, the place that bed crops out and the distance it came. Every stone is paid for out of the coarse layer around it ([`Stones.cpp`](../@simulation/src/ground/fn/Stones.cpp)), so layer and stones together remain what the layer was.
   - *Fallen blocks.* Every site cell within 128 m whose rock is breaking off releases blocks: a quarter of the edge its rock mass cuts out, at most 1.2 m in the median, flatter the more bedded the rock. Each is followed down the site surface as one mass that gains speed with the drop, loses it to friction (more for a small block on rough ground), to the blow where the slope eases, and to being caught between pieces of its own size ([`Rockfall.cpp`](../@simulation/src/ground/fn/Rockfall.cpp)). Large blocks run out farther, which sorts a talus. On the ground tile a block still drops into the hollow next to where it stopped.
   - *The surface of a coarse deposit* ([`Fabric.cpp`](../@simulation/src/ground/fn/Fabric.cpp)): lag over the rock it weathered from, showing where the fine cover is thinner than the stone; scree under a failing face, coarser the farther it rolled; the armour of a stream bed, as large as the flow can no longer move (depth × fall against a Shields stress of 0.045), rounder the farther it travelled, of the rock found that far up the channel; the boulders of a till where the parent says ice ended, with rock from up the ice's path among them; the blocks of frozen ground.
3. **Stream beds** ([`Bed.cpp`](../@simulation/src/ground/fn/Bed.cpp)). In a channel steeper than 3.5 % the few stones too large for the water to move hold where they lie; smaller ones from just upstream come to rest against them across the channel; below each such step, and below every drop over bare rock, the water scours a pool and lays what it dug behind the next step.
4. **Spring deposits** ([`Hydrothermal.cpp`](../@simulation/src/ground/fn/Hydrothermal.cpp)), for every spring the groundwater model reports within 24 m: the water is routed over the surface sixty times, pools filled to their rims, and mineral laid where the water is a thin film and where it spills, never more than the water brought. Rims dam pools that step down the slope; water leaving as steam builds a cone around the vent; ground soaked by water above 70 °C turns to clay and carries no soil.
5. **Water** ([`Water.cpp`](../@simulation/src/ground/fn/Water.cpp)): the last routing runs with stones and deposits in the way. Water stands in every hollow that receives 3 000 m³/yr or more, and a stream's level reaches from bank to bank; over a step it drops.

A stone belongs to the tile its centre lies in and may reach up to 4.2 m beyond it, so a reader takes the stones of every tile within that distance.

## Processes

Each entry says how it is obtained: **evolved** means a numerical state stepped through time, **closed form** an analytic expression, **seeded** a deterministic initial condition or rock-quality pattern derived from the seed, **absent** not modelled.

| Process | Where | How |
| --- | --- | --- |
| Cooling of the initial body, quench crust, first felsic nuclei | Planet, before 200 Myr | Closed form ([`Primordial.cpp`](../@simulation/src/planet/fn/Primordial.cpp)); plume pattern seeded |
| Condensation of the oceans, sea level | Planet | Closed-form share of a fixed inventory; level solved from basin shape every step |
| Plates: velocities, epochs of reorganisation | Planet | Seeded per epoch ([`Plates.cpp`](../@simulation/src/tectonics/fn/Plates.cpp)); at each reorganisation the crust is dealt out anew among the plates, between them a plate is the set of cells it owns and its outline travels with the crust |
| Transport of crust, subduction, collision stacking, rifting | Planet | Evolved, conservative whole-cell shifts ([`Motion.cpp`](../@simulation/src/planet/fn/Motion.cpp)); continental crust is the finite stock mountains are built from |
| Arc and hotspot magmatism, gravitational spreading of thick crust | Planet | Evolved from contact kinematics; yields are tuned constants |
| Isostasy | Planet, Region | Closed form (Airy); the region spreads the response to erosion over a plate-stiffness radius |
| Planet-scale erosion, sediment routing, shelf carbonate | Planet | Evolved, exact ledger ([`Surface.cpp`](../@simulation/src/planet/fn/Surface.cpp)) |
| Dated sedimentary cover and facies | Planet | Evolved record in 25 Myr slots that travels with the crust ([`Strata.hpp`](../@simulation/src/strata/type/Strata.hpp)) |
| Burial: deep strata turn metamorphic, then basement | Planet | Closed-form limits applied to the evolved cover record ([`Strata.cpp`](../@simulation/src/strata/fn/Strata.cpp)) |
| Unconformity between folded and later strata; shields | all | Closed form from the recorded time of the last stacking or melt ([`Column.cpp`](../@simulation/src/strata/fn/Column.cpp)) |
| Plutons cutting older strata | all | Seeded bodies conditioned on the simulated province and its dated deformation |
| Heat from below | Planet | Closed form from crust kind, lid age and the evolved record of melt per column; magmatic centres seeded in the frame of the crust ([`Heat.cpp`](../@simulation/src/planet/fn/Heat.cpp)) |
| Basement rock bodies, folds, joints, fault zones, range and graben structure | all | Seeded patterns attached to the crust, conditioned on the simulated province ([`Column.cpp`](../@simulation/src/strata/fn/Column.cpp), [`Structure.cpp`](../@simulation/src/tectonics/fn/Structure.cpp)) |
| Rock uplift | Region | Evolved: the planet's recorded thickening distributed by structure, plus isostatic response |
| Flow routing, lakes, lake evaporation | Region to Ground | Evolved each step: priority flood, multiple flow directions that focus with discharge ([`Flood.cpp`](../@simulation/src/landscape/fn/Flood.cpp), [`Flow.cpp`](../@simulation/src/landscape/fn/Flow.cpp)) |
| River incision and deposition in three grain classes, bedload wear | Region to Ground | Evolved, implicit stream power with transport-length deposition ([`Fluvial.cpp`](../@simulation/src/landscape/fn/Fluvial.cpp)) |
| Deltas and basin fill in standing water | Region, Local, Site | Evolved: suspended plume spread and settled without a preferred direction |
| Weathering, solution of soluble rock, soil creep, slope failure | Region to Ground | Evolved; creep is solved implicitly ([`Hillslope.cpp`](../@simulation/src/landscape/fn/Hillslope.cpp)) |
| Selective erosion of beds | Local, Site | Evolved: each cell erodes with the traits of the bed it has cut down to |
| Joint sets, corridors and zones of a rock mass; attitude of the layering | all | Seeded planes attached to the crust, one field for every level and for the voxel column ([`RockMass.cpp`](../@simulation/src/rockmass/fn/RockMass.cpp)) |
| Layers within a bed, seams | Ground, Form | Seeded geometry attached to the crust, from each rock's bedding ([`Partings.cpp`](../@simulation/src/ground/fn/Partings.cpp)) |
| Rills and gullies, ledges and steps over layers, knobs between joints, patchy mantle | Ground | Evolved: the level's settling with those traits |
| Channel width | Ground | Closed form (hydraulic geometry) deciding which cells the evolved incision works |
| Frost shattering | Ground | Evolved: regolith production scaled by the closed-form share of the year in the cracking window |
| Sorted circles, nets and stripes | Ground | Evolved: 64 rounds of conservative exchange of stones and fines between neighbouring cells; starts from the mantle the settling left |
| Solifluction lobes | Ground | Evolved: 40 rounds of thickness-dependent flow of the thawed mantle |
| Blocks falling from steep rock, their run-out and rest | Ground | Evolved one by one as lumped masses on the site surface; size, first direction and the chances of being deflected or caught are seeded draws |
| Lag, scree, stream armour, till boulders, frost blocks as individual stones | Ground | Seeded packing of a simulated layer: where a stone can lie, how many, how large, of which rock and how round follow from the settled mantle, the flow, the ice and the rock; which of the possible places hold one is a seeded draw |
| Steps and pools of steep stream beds, plunge pools | Ground | Constructed once from simulated state: steps stand where stones too large for the flow came to lie, pools are dug to a depth set by the step and the flow, and their waste is laid behind the next step |
| Riffles, bars, wandering thalweg of gentle reaches | Ground | Whatever the level's settling leaves across the stream's width; not organised further |
| Spring terraces, rims and pools, cones, altered ground | Ground | Evolved from the springs the groundwater model reports: sixty epochs of routing, pooling and deposition on the site surface, started from seeded roughness of a centimetre |
| Water in pools and from bank to bank | Ground | Closed form on the last routing: hollows fed through the year hold water, a stream stands at the depth its hydraulic geometry gives |
| Glaciers: snow balance, ice flow, abrasion, moraines | Local, Site | Evolved during cold phases of the settling ([`Glacial.cpp`](../@simulation/src/landscape/fn/Glacial.cpp)); snow balance is the closed form of a sinusoidal year |
| Ledges and overhangs of beds on a steep face | Form | Closed form from layering, the lie of the slope (the ground surface averaged over 2 m to every side) and cover ([`Form.cpp`](../@simulation/src/relief/fn/Form.cpp)) |
| Joint slots, towers, arches | Form | Closed form along the planes of the rock mass; how wide and deep a slot has opened along its length is seeded ([`City.cpp`](../@simulation/src/relief/fn/City.cpp)) |
| Veins around intrusions, pipes in shields | Form | Seeded fractures, conditioned on the simulated province, its melt and its plutons ([`Ore.cpp`](../@simulation/src/ore/fn/Ore.cpp)) |
| Caves | Form | Voids the groundwater model reports, cut out of the rock; where one comes within 0.3 m of the rock surface the cover over it lies on its floor |
| Waves: growth by fetch, breaking, bed friction, shelter | Region, Local, Site | Evolved: eight wave trains carried downwind per step ([`Waves.cpp`](../@simulation/src/coast/fn/Waves.cpp)) |
| Cliff retreat, wave-cut bench, sea caves on joints | Region, Local, Site; caves in Form | Evolved retreat against rock strength; caves in closed form from the face's undercut and joint spacing ([`Cliff.cpp`](../@simulation/src/coast/fn/Cliff.cpp), [`SeaVoid.cpp`](../@simulation/src/coast/fn/SeaVoid.cpp)) |
| Beach drift and grain sorting, spits, barriers, lagoons | Region, Local, Site | Evolved: per-train sand transport with alongshore turning, swash and undertow ([`Drift.cpp`](../@simulation/src/coast/fn/Drift.cpp)) |
| Wind regime (sand rose) | Region, Local, Site | Closed form from the climate's belt winds, their seasonal shift, gustiness and storm-track storminess ([`Regime.cpp`](../@simulation/src/aeolian/fn/Regime.cpp)) |
| Sand budget: supply-limited entrainment, stone lag armour, moisture and frost, capture by water, rivers and shelter of relief | Region | Evolved each step as a steady flux along the resultant drift; wind-laid cover kept off the ground between steps except on water and channels ([`Budget.cpp`](../@simulation/src/aeolian/fn/Budget.cpp)) |
| Dunes, slip faces, sand ramps | Local, Site | Evolved once at the end of settling: slab transport model (Werner 1995) in flux form, up to 500 rounds on the sand the parent hands down ([`Dunes.cpp`](../@simulation/src/aeolian/fn/Dunes.cpp), [`DuneField.cpp`](../@simulation/src/aeolian/fn/DuneField.cpp)) |
| Dust and loess | Region | Evolved: raised by winnowing, sand impact and dry river bars; settles along the drift in two sizes ([`Dust.cpp`](../@simulation/src/aeolian/fn/Dust.cpp)) |
| Salt pans | Region | Closed form each step from the water balance of closed basins ([`Basin.cpp`](../@simulation/src/aeolian/fn/Basin.cpp)) |
| Regional ice sheets, dust from beyond a region, yardangs, old shorelines, placers (other than vein float carried as stones), stratiform ores, volcanic edifices, impact craters | — | Absent |
| Stones that break, wear or move after they are laid; stones shaping the flow that carves the ground (they only dam the last routing); bank collapse and meander migration; debris flows; vegetation and everything it does to ground | — | Absent |

## Materials

Every material is one typed definition under [`material/definition`](../@simulation/src/material/definition), listed in [`Materials.hpp`](../@simulation/src/material/constant/Materials.hpp). Each process reads only the group of properties it needs; porosity (storage) and conductivity (speed) are separate inputs. The engine's voxel materials are a view of the same definitions.

## Queries

[`Query.hpp`](../@simulation/src/simulation/fn/Query.hpp) is the interface for the engine and for tools. `surfaceAt` answers from whatever is already built and never blocks. `surveyAt`, `materialAt` and `sampleBlock` are authoritative: they build the tiles under the point and return the same answer for a point however large the block it is asked through, because the surface, the layering, the exposure, the stones and the voids are all read in world-anchored frames.

Voxels are cut from the ground level: `sampleBlock` builds the ground tiles within 7.5 m of its block (and the site tiles within 40.5 m, for the exposure of cliffs). A survey builds every tile its interpolation reads, across a tile edge too, so its answer does not depend on what happens to be built. The surface voxels show is `surveyAt(…, Level::Ground)` or, without blocking, `surfaceAt(…, Level::Ground)`; both default to the site level, which is all a caller needs beyond the reach of voxels. `stoneAt` names the stone a point lies in: its rock, the process that left it, the place that rock crops out and how far it came. A stone stands up to nearly four metres over that surface and is not part of it. `SurfaceSample::water` of a ground tile is the level in pools and from bank to bank of streams.

What the session changed is laid over these answers ([`Live.cpp`](../@simulation/src/simulation/fn/Live.cpp)). Edits and live water are kept by site cell ([`intervention`](../@simulation/src/intervention)); the ground level under them is not replayed. A surface sample, every node of the ground a voxel column reads, and every stone are moved by what changed in the site cell they lie in, so the ground keeps its own shape inside a dug or built cell and steps at the cell's edge. Water is the live water where a cell holds any; sea over untouched ground stands at the level the ocean has now. The queries only read that state, and none of them may run while the thread that drives time and edits is inside one of its calls.

A ground tile spreads the parts of its own replay whose rows are independent over the pool its atlas builds with (the first reading of partings and joints, the implicit creep lines, the stream power of every cell, the run-outs of fallen blocks, the running means of frost sorting). The rest of a step is ordered work: flooding, routing, cutting and failure follow the water from cell to cell. What a tile contains does not depend on the pool.

The atlas keeps every tile it has built. A ground tile holds about 7 MB, so a long walk needs a bound that does not exist yet.

[`chartOf`](../@simulation/src/chart/fn/Chart.hpp) reads a whole square at once for maps: per pixel what the finished tiles hold at its centre (`readSurface`) from the finest level the pixel pitch can show, the planet where no tile is finished, and water drawn to be read at that pitch: the largest stream a wide pixel covers, a stream as its path and a lake up to its shore where pixels are narrower than cells. It never blocks or builds, and it only reads what cannot change, so any thread may call it while tiles are being built and the session runs. For that reason it is the untouched world: what the session has changed (`overlayLive`) is not in a chart.

## Known weaknesses

- Plates move by whole 65 km cells along the grid axes and do not rotate. Rifted margins and collision fronts can therefore run straight along an axis for a few hundred kilometres. Finer levels read the planet through a smooth offset ([`Forcing.cpp`](../@simulation/src/landscape/fn/Forcing.cpp)), which hides the alignment without removing its cause.
- The finer levels are a balance with present conditions, not a dated continuation of the region. A landform that owes its shape to a specific past event below region scale is not reproduced.
- Overhangs, slots, towers and arches are constructed, not grown by a weathering simulation, and are limited to what one column can decide from its own surroundings.
- A falling block is followed for at most 124 m from its source and only over the four-metre site surface; blocks that would run farther are left out, so the largest outliers of a long slope are missing. Stones of a deposit's surface know the face they came from only within their own ground tile; beyond it they are given the rock under them.
- Stones do not cross a tile edge while they settle or jam, and a tile does not see the stones of the tile next to it: two stones can overlap across an edge.
- A spring deposit is grown on the site surface, not on the ground tile's own, so that every tile it spans agrees on it; it ignores rills and stones under it and ends 24 m from its vent whatever the spring yields. Its water joins the routing only of tiles whose domain holds the vent, so the pools of a deposit that reaches into the next tile stand dry there.
- Sorting, lobes, steps and pools are worked on a tile's own cells: a pattern or a pool does not continue across a tile edge. Frost weathering uses one share of the year for a whole tile.
- Ground tiles ask the groundwater for springs through the world their atlas was told of (`Atlas::bind`, called by every query that can build one). A ground tile built through the atlas before any query has none; the world should name itself to the atlas when it is made.
- At the site level the rise that holds the surface can be negative: a hollow that keeps filling makes the rock under it sink, which has produced fills of hundreds of metres below steep slopes. The ground level inherits such a fill as its mantle.
- Cover carries no record of how it came to lie where it is: what creep and failure moved is not told from what weathered in place, and a till is known only by the ice its parent recorded over it.
- A region's rivers know the water of catchments outside their domain only through the planet's 65 km drainage.
