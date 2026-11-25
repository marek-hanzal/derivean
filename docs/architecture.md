# DeRivean architecture

| Module | Ownership |
| --- | --- |
| [`@engine`](../@engine) | Authoritative world, material behavior, structural failure, Jolt collision, and frame projections |
| [`@napi`](../@napi) | Node-API binding, xmake addon build, frame serialization, and bridge integration tests |
| [`@game`](../@game) | Electron launcher, worker and IPC, input, Three.js renderer, and HUD |

The engine runs in a Node worker owned by Electron's main process. The sandboxed renderer submits one in-flight command over the [bridge](../@game/contract/GameBridge.ts) and updates its look direction locally. The engine advances in fixed 120 Hz steps while play is active. It returns player and debris transforms, changed or evicted terrain and water meshes, the authoritative target, and voxel particles. Three.js expands the packed faces into render geometry and instantiates water, sand, and foam cubes.

## World and support

The [voxel world](../@engine/src/world/type/VoxelWorld.hpp) uses sparse 4 m cubic chunks in all three axes. Each occupied 1 m tile spans 8 × 8 × 8 voxels. A uniform tile stores its material once and expands to a 512-entry array when it contains mixed materials. Terrain masks, meshes, and colliders use local dense samples. [The Stacked Channels](../@engine/src/demo/fn/ProvingGrounds.cpp) is an explicitly built fixture inside that world. Deck, sides, dams, and bed are ordinary material that can be excavated at any altitude.

[Materials](../@engine/src/voxel/constant/Materials.hpp) carry density, friction, and load capacity per voxel face. The [support check](../@engine/src/structure/fn/Collapse.cpp) discovers connected pieces around cuts, tests their load against local contacts, and accounts for weaker bearings such as leaves. Contacts with real material at the search boundary anchor the local calculation. Detached solids enter Jolt as weighted rigid bodies. The authoring stock and excavated mass are reported separately.

## Water and grains

[Water state](../@engine/src/water/type/Water.hpp) stores 256 integer volume units per full voxel. A 30 Hz transfer step visits changed water cells and their neighbors, moves volume down openings, and uses surface head for lateral flow. A [connected backwater pass](../@engine/src/water/fn/Backwater.cpp) can move pressure head through a full, roofed passage. A gate or a newly placed solid blocks transfer across its occupied cells. Placing a solid in bulk water displaces the volume into available connected space before committing the edit.

A drop detaches an actual amount from a cell. [Airborne parcels](../@engine/src/water/fn/WaterParcels.cpp) use gravity, preserve that volume through flight and contact, and merge into the receiving water or terrain pool. Fast impacts emit short-lived foam cubes. [Sand](../@engine/src/granular/fn/Sand.cpp) detaches when support gives way or the neighboring slope exceeds its angle of repose. Grains settle back into voxels or remain in motion. Both types query static terrain and sweep against moving Jolt bodies, including the player's capsule.

Water that has fallen below all surviving terrain and possible moving contacts goes into the escaped ledger. This test follows actual material coordinates. A deeper shelf can catch material without changes to the simulation. The invariant is `seeded water = in cells + airborne + escaped`. Sand grains and detached rock track their own losses. If the player falls past the terrain, the engine looks for surviving support for respawn.

The fixture uses finite elevated reservoirs as its sources. A future spring can release from a finite catchment or groundwater store using the head difference at a physical outlet, with the same volume debit. An impermeable dam can stop downstream flow until water finds an outlet or overtops it. Current fluid behavior approximates pressure and flow on the voxel grid. The PoC has a finite hand-built scene, session-local edits, and a single gravity direction.
