# Life: flora, fauna and their origin

The owner's decisions of 26 Nov, written by the lead. Not started; to begin after the relief work lands (vegetation changes every erosion law the relief is being tuned with) and after the reactive world in the hand-over README (live Local and Region zones), which life needs for fire, succession and pests.

## What the owner decided

- **"The world has no life" no longer holds** once this work starts. Until then it stays binding.
- **Procedural plants** that are seen in the game: trees, shrubs, grasses, flowers down to 1/16 of a voxel, with their influence on the terrain.
- **Start from archetypes of real plants** (spruce, oak, birch, pine, juniper, grass, moss, fern, cactus, reed and so on) to enrich the world quickly. The target is a flora that **evolves from one founder**: a primitive lineage from which, under the simulated environment, whatever fits comes about. Archetypes then become the check that evolution reaches believable forms, not their source.
- **Plants must reproduce**, so life has to be modelled from its origin, and plants and animals must form symbioses.
- **Fauna: small animals only.** Insects, birds and the other small creatures that matter to plants and ground. No large animals for now.
- **Simplified, but good enough.** Rendered from the same laws (no props). The simulation must be good enough that cycles, outbreaks and pollinator dependence arise, and simple enough to compute.
- **Deployed in steps**, each a finished state that works on its own, with the next one hung on it.

## Design

### Genome, not species

Every organism is a vector of continuous traits with a physical meaning.

- **Plants:** height, branching, wood density, leaf area and form, root depth and spread, growth rate, lifespan, seed size and dispersal mode, phenology, and tolerance of drought, frost, shade and salt.
- **Fauna guilds:** body size, diet, mobility, mouth or proboscis form, and season.

From the genome come:
- **function**, which the simulation reads: cohesion of a slope, transpiration, protection from erosion, organic matter, fuel;
- **form**, which the renderer draws: a growth grammar (L-system or space colonisation) driven by the same traits.

The space of forms is bounded by physics (stem statics, hydraulics, light capture), so evolution converges on plausible shapes, as it does on Earth.

### Services: the seam for stepwise deployment

Everything a plant needs from other life is a named service with a fixed interface:

- pollination
- seed dispersal
- herbivory
- seed predation
- decomposition
- mycorrhiza
- nitrogen fixation

In early steps a service is supplied by a closed-form "ghost": for example, pollinator density from climate and flower cover. Later the simulated fauna supplies it through the same interface, and plant code does not change.

Energy (biomass production and consumption) is the common currency between plants and fauna.

### Fauna as guilds

Fauna is modelled as functional groups with biomass per cell, not as species. The precedent is the Madingley general ecosystem model.

| Guild | What it does |
| --- | --- |
| Pollinators | Pollinate flowers |
| Herbivores | Eat plants (caterpillars, aphids, grasshoppers) |
| Seed dispersers | Birds, ants |
| Predators | Spiders, predatory insects, insectivorous birds |
| Decomposers | Fungi, bacteria, earthworms, termites |
| Symbionts | Mycorrhizal fungi, nitrogen-fixing bacteria |

Soil fauna moves ground and is a terrain process of its own: bioturbation, termite mounds.

### Origin of life

The origin of life belongs to the planet, on its deep-time steps:

- microbial mats in warm shallow water;
- photosynthesis and oxygen, then ozone, then colonisation of land;
- biogenic carbonate (reefs, lime muds) beside the solute precipitation that exists now;
- soil formation and faster weathering;
- eras on the land: bare, first plants, forests.

### Evolution

Evolution also runs on the planet, in steps of about a million years:

- **Lineages:** each cell holds a few, each a distribution of traits with an abundance.
- **Mutation:** deterministic, from a hash of (lineage, time).
- **Selection:** by the simulated climate, water, soil, altitude, volcanism and ice ages.
- **Competition:** for light and water.
- **Speciation:** by isolation behind ranges, seas and deserts. Moving plates then give endemics, different floras per continent, and mixing when continents collide.
- **Coevolution:** by trait matching, for example flower depth against proboscis length, or fruit size against beak gape.

### Levels, as for the terrain

| Level | What life is there |
| --- | --- |
| Planet | Origin, lineages, ranges, eras, biogeochemistry |
| Region, Local | Communities: cover and composition by functional type, guild biomass, succession after disturbance; feedback on erosion, slope stability, runoff, dunes and soil during the replay |
| Site, Ground | Individual plants (position, age, size, lineage), placed deterministically from stand dynamics; soil fauna works the ground |
| Game | Live growth and succession in calendar time; seasons; visible fauna near the player |

Rules carried over from the terrain:
- An untouched tile is a pure function of (seed, age, key).
- Ranges and composition come from the parent level; the fine level only places individuals, the way rivers are handed down.
- Ledgers close.

### Rendering

- **Near:** the form from the genome, as fine voxels (1/16 for flowers and stalks, coarser for trunks); trunks can be real voxels that can be cut.
- **Middle distance:** simplified instanced forms.
- **Far:** vegetation as a property of the cube (colour, canopy height, surface texture), keeping "cubes at every distance".
- **Individuals:** only around the player, statistics everywhere else, as with stones today.

## Steps

| Step | Adds | Needs | Stands alone |
| --- | --- | --- | --- |
| 0 | Microbial life and biogeochemistry on the planet | planet, atmosphere | yes |
| 1 | Flora from archetypes as functional cover with feedback on the terrain; every fauna service as a ghost; vegetation seen as cube colour and canopy | 0 | yes: a vegetated world whose terrain responds |
| 2 | Individual plants, procedural form, fine voxels; growth and succession in live time | 1 | yes |
| 3 | Fauna as guilds replacing the ghosts; soil fauna moving ground | 1 | yes: a closed ecosystem |
| 4 | Visible fauna near the player (flocks, swarms, ant hills) drawn from guild densities | 2, 3 | yes |
| 5 | Evolution and coevolution from the founder; archetypes become the check | 1–3 | replaces the hand-set archetypes |

From the first day:
- the service interfaces;
- genomes rather than species names;
- energy as the currency;
- the terrain's determinism and ledger rules.

Each step is validated against Earth figures before the next is hung on it, for example:
- net primary production by climate;
- erosion under cover against bare ground;
- pollinator dependence;
- herbivore biomass against plant biomass.

## Costs and risks

- **Size.** Steps 0–2 are about the size of the terrain work so far, and steps 3–5 another project of similar size. Expect separate streams for life simulation, form and rendering, and engine and live time.
- **Recalibration.** Today's erosion laws are fitted to a vegetated Earth; with life present they must be refitted for bare ground plus the effect of cover.
- **Believable evolution.** A free genome yields nonsense. The morphospace must be bounded by physics, and results compared with the archetypes.
- **Determinism across tiles.** Seeds and animals cross tile edges and levels; the parent level owns ranges and abundances, and fine levels only place.

## Open questions for the owner

1. When life arrives in planet history, and whether the world's age (4.6 Gyr) should give Earth-like eras.
2. Whether step 1's archetypes may be hand-set for a while, or whether evolution must come early.
3. Can the player interact with plants and fauna: cut, plant, harvest, disturb? This decides what the engine must support from step 2.
4. Fire as a process: lightning, dry fuel, regrowth. It belongs with step 1 or 2.
5. Larger animals: excluded for now; confirm that grazing pressure stays a ghost service until then.
