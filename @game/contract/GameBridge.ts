// The whole contract between the native engine (through the preload) and the renderer, about 410 lines.
// It stays one file because it is one boundary: every type here crosses it in the same structured clone, a
// change to it is agreed between the engine and renderer streams as one change, and either side reads it
// whole to know what the other promises. FarPatch and Frame.far (about 50 lines) go when the far patches do.
export type Point = [number, number, number];
export type BrushShape = "sphere" | "cube";

// The planet to play on. Omitted altogether it is seed 42 at 4.6 billion years.
export interface WorldOptions {
  seed: number;
  // Years since the planet formed.
  age: number;
  // X and Z in metres. Absent, the simulation picks the landing: a shore in a low country with a high range
  // rising over it.
  spawn?: [number, number];
  // The far terrain as levels of cubes (Frame.cubes) instead of height-field patches (Frame.far); a world makes
  // one of the two. With cubes the ground comes as packed quads, near and far alike.
  cubes?: boolean;
  // Cubes from the eye to the side of a level's box, 64 to 256, 128 when omitted. A cube at the side of its
  // level's box is 1 / cubeReach radians wide: pixels per cube there = focal length in pixels / cubeReach, and
  // twice that at the box of the next finer level.
  cubeReach?: number;
}

export interface WorldInfo {
  voxelSize: number;
  chunkMeters: number;
  defaultBrushRadius: number;
  maxBrushRadius: number;
  waterUnitsPerVoxel: number;
  materials: {
    name: string;
    color: number;
    roughness: number;
    density: number;
    support: number;
    friction: number;
    matter: "gas" | "solid" | "granular" | "liquid";
    angleOfRepose: number;
    dynamicViscosity: number;
    surfaceTension: number;
  }[];
  // How far patches are chosen. A patch with cells of c metres is on screen no nearer than splitCells · c and,
  // where its children would show, no farther than splitCells · 2c; it should have blended onto its `morph`
  // heights over the last (1 − morphStart) of that range. Ground that refining would not visibly change stays
  // coarser than that, and a patch already split stays split a tenth beyond its range. Distances run from the
  // eye to the vertex.
  far: { cells: number; splitCells: number; morphStart: number; viewDistance: number };
  seed: number;
  age: number;
  // Metres against the datum y = 0. Below the world floor is the void, the only place matter is lost to.
  seaLevel: number;
  worldFloor: number;
  // Metres after which the planet repeats on both axes.
  planetSpan: number;
  // Metres above the sea of the rungs CloudField.snowLadder keeps snow at.
  snowLadder: number[];
  // Shorts per quad in every Int16Array of faces: 9 in a world of cubes, 8 without the occlusion word in one of
  // patches.
  faceWords: number;
  // Null in a world of patches. Level k holds cubes of voxelSize · 2^k metres on the voxels' own lattice, from
  // finestLevel to coarsestLevel; the levels below are the voxels. A chunk is `side` cubes along every edge, as
  // a voxel chunk is. The box of a level reaches `reach` cubes from the eye and that of the coarsest level
  // `viewDistance` metres; a level's chunks cover at least `band` of its cubes beyond its box.
  cubes: { reach: number; side: number; band: number; finestLevel: number; coarsestLevel: number; viewDistance: number } | null;
}

export interface Command {
  elapsed: number;
  x: number;
  z: number;
  jump: boolean;
  sprint: boolean;
  active: boolean;
  direction: Point;
  radius: number;
  shape: BrushShape;
  dig: boolean;
  place: boolean;
  // True on the one frame that toggles flight.
  fly: boolean;
  // A sprint on foot; in flight the speed that crosses kilometres, growing with height above ground.
  boost: boolean;
  // -1…1, vertical thrust in flight.
  ascend: number;
  // Where to put the player's feet, in metres.
  teleport: Point | null;
  // The world clock. `rate` is calendar seconds per second of play: 0 holds it still, 1 is real time, up to a
  // year per second. `jump` skips that many calendar seconds at once; 0 normally. Play itself always runs in
  // real time. Omitted, the clock runs in real time.
  time?: { rate: number; jump: number };
}

// The quads of one chunk of 32 cubes a side, two words each, counted in cubes from the chunk's minimum corner.
//   word 0: plane (bits 0–5), lowU (6–11), lowV (12–17), sizeU − 1 (18–22), sizeV − 1 (23–27), axis (28–29: 0 x,
//           1 y, 2 z), sign of the normal (30: 1 for positive).
//   word 1: material (0–7), an index into WorldInfo.materials; corner occlusion (8–15).
// U and V are the two axes after `axis` in cyclic order (x: y, z; y: z, x; z: x, y). Occlusion holds two bits per
// corner, from the lowest (lowU, lowV), (highU, lowV), (lowU, highV), (highU, highV): how many of the three cells
// that touch the corner in the layer the face looks into are solid, 3 where both edge neighbours are. A quad is
// one material and is only stretched along an axis its occlusion does not change on, so the four corner values
// interpolate exactly. The remaining bits are zero.
export type Quads = Uint32Array;

export interface ChunkMesh {
  key: Point;
  origin: Point;
  // WorldInfo.faceWords shorts per quad: axis, sign of the normal, plane, lowU, lowV, sizeU, sizeV, material and,
  // in a world of cubes, occlusion, with the meanings they have in Quads. Water meshes come this way, in their
  // own units and without occlusion, and so does the ground in a world of patches; in a world of cubes the
  // ground's `faces` are empty and its quads are in `quads`. Debris faces share the layout, counted in voxels.
  faces: Int16Array;
  // The ground of a voxel chunk in a world of cubes; null in every other mesh.
  quads: Quads | null;
}

// One chunk of a level of far cubes: the voxel world resampled at a coarser cube size, 32 cubes along every
// edge. A cube is solid if the world is solid at its centre and is made of what is there; the top cube of a
// column is made of what covers the surface. Chunks of one level never overlap. What is held of a level changes
// a column of chunks at a time: all the chunks over one square of ground are added, dropped, or swapped for a
// better build of themselves, `removed` and `added` in the same frame.
export interface CubeChunk {
  id: number;
  // Cube edge = voxelSize · 2^level = `edge` metres.
  level: number;
  edge: number;
  // World position of the minimum corner, a multiple of 32 · edge on every axis.
  origin: Point;
  // Top faces carry the material that covers the surface. A riser is mostly the step a slope turns into when it
  // is cut into cubes, so it carries that same cover wherever the ground there is a slope its loose cover lies
  // on, up to 45°; where the ground is steeper than that, or bare, it carries the bed at its depth, and a cliff
  // shows its strata as bands. Only faces between a solid cube and air are sent: no bottoms, nothing between
  // two cubes of a level, nothing under the surface, no face twice at a border between chunks.
  quads: Quads;
  // Standing water over this chunk's square of ground: one sheet per water level, a lake or the sea. `quads` are
  // top faces over the wet columns with plane 0, to be drawn at `level` metres (world) whatever height the
  // chunk is at; the highest chunk over a square carries them. Under a sheet the cubes end below the level, and
  // the dry columns beside it at or above it.
  water: { level: number; quads: Quads }[];
  // As in FarPatch: the coarsest data level any column was read from, and the level the chunk is cut from once
  // that is finished there. Below its wanted level a chunk shows coarse data; it is swapped when finer tiles
  // arrive.
  dataLevel: number;
  wantedLevel: number;
}

export interface CubeLevel {
  level: number;
  edge: number;
  // Squares of ground of this level (columns of chunks) the view wants: those reaching into the level's box at full
  // reach and its band, not the margin built beyond for an eye on the move. How many of them the renderer holds, and
  // how many of those are final: cut from the data the level wants (dataLevel = wantedLevel) and not about to be
  // swapped. The level is drawable where its box says, and final when settled equals wanted. A level rests,
  // wanting none, while the eye is too high above the ground or too fast for it.
  wanted: number;
  shown: number;
  settled: number;
  // Where the level may be drawn this frame, world metres, a cube centred on the eye; null while it cannot be.
  // A level is drawn inside its box and outside the box of the next finer level that has one, the finest far
  // level outside voxelBox. Boxes are nested. Every level is whole inside its box, also under the finer levels:
  // over each square of ground in the box's footprint the renderer holds every chunk of the level that has a
  // face, at any height, so the top and bottom of the box cut nothing off. A chunk that lies wholly inside the
  // next finer box need not be drawn. The chunks reach at least WorldInfo.cubes.band cubes beyond the box: the renderer may dither across a
  // boundary, or sink a coarser level under the finer one instead of clipping it. While chunks are missing the
  // box shrinks and the coarser level shows there, never a hole. A level is at full reach when its box reaches
  // WorldInfo.cubes.reach · edge from the eye, the coarsest WorldInfo.cubes.viewDistance.
  box: { min: Point; max: Point } | null;
}

// One height-field patch of the far terrain. Vertex arrays hold (cells + 1)² entries, row-major with Z as the
// row. Cells split along the diagonal from their minimum to their maximum corner; `morph` assumes it. Every
// per-vertex height is measured from `origin`, which alone carries the world position.
export interface FarPatch {
  id: number;
  // 0 is the finest; cell size doubles per level.
  level: number;
  // World position of the minimum corner; its y is the elevation the patch's heights are measured from.
  origin: Point;
  // Edge length in metres.
  size: number;
  // Quads per side.
  cells: number;
  // Metres above origin.y.
  heights: Float32Array;
  // Metres above origin.y: the parent level's surface under the same vertex; equal to `heights` on the coarsest level.
  morph: Float32Array;
  // xyz per vertex, × 127.
  normals: Int8Array;
  // rgb per vertex, sRGB: the palette colour of the material the voxels put on top of the ground there, also under water.
  colors: Uint8Array;
  // Metres above origin.y: the flood level of a lake or of the sea, one value per water body, read from the
  // finest data there is whatever the patch's level. It is carried across ground below it and one vertex into
  // the dry shore, so the sheet meets the ground at its depth-zero contour. NaN everywhere else; running water
  // is not a level and has no sheet.
  water: Float32Array;
  // Metres the border may be dropped by to cover what morphing cannot.
  skirt: number;
  // Which data the patch was built from, for diagnostics and a debug tint: the coarsest level any of its
  // vertices was read from, and the level it reads once that is finished there. 0 is the planet's 65 km grid,
  // then 1 region (1 km cells), 2 local (64 m), 3 site (4 m), 4 ground (0.25 m). A patch below its wanted
  // level shows coarse data, not a coarse world; it is rebuilt when the finer tiles arrive.
  dataLevel: number;
  wantedLevel: number;
  // A raster for shading only, finer than the geometry: a trick that lets coarse geometry read as ground. It
  // lies over the same square as the height grid with (cells · shadeDetail + 1)² texels, row-major with Z as the
  // row, texel (0, 0) on the patch origin, so texel (shadeDetail · i, shadeDetail · j) lies on vertex (i, j).
  // shadeDetail is 1, 2 or 4 and may differ between levels.
  shadeDetail: number;
  // Four bytes per texel:
  //   [0], [1] x and z of the unit surface normal as round(n · 127) + 128, so 128 is zero; y is rebuilt and
  //            always up. From the finest data the simulation has there, with one-sided slopes: not averaged
  //            across a cliff edge or a bench lip.
  //   [2]      material id of the surface, an index into WorldInfo.materials: glacier ice where it lies more
  //            than a metre thick, loose cover where it is thick enough for a voxel to show it, else the rock.
  //            Where cover thins out the id is dithered by the share a voxel column would show, not blended.
  //            Seasonal snow is not here; it changes with the calendar. Beds exposed inside one cell of the
  //            simulation's data are not resolved yet.
  //   [3]      how much sky the place sees, 255 on an open plain, lower in gullies and under walls; from the
  //            heights including a ring beyond the patch, so it has no seam at the border.
  shade: Uint8Array;
}

export interface Frame {
  eye: Point;
  target: { voxel: Point; material: number } | null;
  chunks: ChunkMesh[];
  waterMeshes: ChunkMesh[];
  water: { initial: number; bulk: number; airborne: number; escaped: number };
  waterDepth: number | null;
  // x, y, z in metres from particleOrigin, cube size in metres, particle kind (water, sand, foam).
  particles: Float32Array;
  particleOrigin: Point;
  unloaded: Point[];
  spawned: { id: number; faces: Int16Array }[];
  debris: { id: number; position: Point; rotation: [number, number, number, number] }[];
  retired: number[];
  editedTiles: number;
  visibleChunks: number;
  bodies: number;
  nativeMs: number;
  buildStock: number;
  // False while the player waits for the voxels around them; the world is not playable yet.
  streamingReady: boolean;
  sandAirborne: number;
  sandDeposited: number;
  sandEscaped: number;
  debrisEscapedKg: number;
  excavatedKg: number;
  flying: boolean;
  // A removed id is gone for good; a rebuilt patch arrives under a new id in the frame that removes the old one.
  // `data` counts the patches the renderer now holds by their dataLevel, 0 to 4.
  far: { added: FarPatch[]; removed: number[]; data: number[] };
  // The far terrain as cubes; null in a world of patches, whose `far` is then the far terrain. `levels` runs from
  // the finest and is sent every frame; `data` counts the held chunks by their dataLevel, 0 to 4.
  cubes: { added: CubeChunk[]; removed: number[]; data: number[]; levels: CubeLevel[] } | null;
  // Region in which voxel chunks, not far patches, are the ground to draw; null while there are none.
  voxelBox: { min: Point; max: Point } | null;
  // Metres, simulated ground under the eye; null if unknown.
  ground: number | null;
  // Metres, natural water surface standing over that ground; null when dry.
  waterLevel: number | null;
  // Every simulation tile the view wants at that level of detail is finished; `far` says the far terrain is
  // final: every level holds all it wants, cut from the data it wants, and nothing is waiting to be swapped.
  // A picture taken while `far` is false shows stand-in data somewhere.
  ready: { site: boolean; local: boolean; region: boolean; far: boolean };
  // The same as counts, for a progress display and for waiting until a view is whole: the tiles the view wants
  // at each level (region over 330 km, local over 64 km, site over 3 km, ground around the feet; in a world of
  // cubes the tiles its levels read instead; a level that is pointless from the player's height wants none) and
  // how many of them are built. The wanted set is finite and
  // moves with the player; everything is built when finished equals wanted at every level.
  tiles: Record<"region" | "local" | "site" | "ground", { wanted: number; finished: number }>;
  pending: { chunks: number; patches: number };
  // Background time spent on the voxel chunks that entered the world this frame.
  generationMs: number;
  // Game-thread time this frame spent bringing the planet in: tiles, voxels, far patches. Part of nativeMs.
  streamingMs: number;
  // The world clock: `era` is the planet's age in years, `seconds` the calendar time played since. dayOfYear runs
  // 0…365 and hour 0…24, both with fractions.
  calendar: { era: number; seconds: number; dayOfYear: number; hour: number };
  // Unit vector towards the sun at the player; y is negative at night.
  sun: Point;
  // Weather at the player from the simulated atmosphere: °C at the ground, rain and snow in mm/h of water, wind in
  // m/s along x and z, cloud cover and relative humidity as shares, pressure in Pa, snowpack in kg/m² (mm of
  // water) lying on ground of the player's height there. `lag` is the calendar seconds the atmosphere is behind the clock, above
  // zero only while a high rate outruns it.
  weather: {
    temperature: number; rain: number; snow: number; wind: [number, number];
    cloudCover: number; humidity: number; pressure: number; snowpack: number; lag: number;
  };
  // A new cloud snapshot, at most about one a second; null on every other frame. The renderer draws from the
  // latest one and never makes weather up.
  clouds: CloudField | null;
}

// The clouds of the whole planet on the weather grid. Entry z * cells + x of every array belongs to the column
// centred at origin + ((x + ½) · cell, (z + ½) · cell) in world metres, and the field repeats after cells · cell.
// `time` is the calendar second it describes. Bands run from the lowest up.
export interface CloudField {
  origin: [number, number];
  cell: number;
  cells: number;
  time: number;
  bands: {
    // Share of the sky.
    cover: Float32Array;
    // Metres above the sea (WorldInfo.seaLevel).
    base: Float32Array;
    top: Float32Array;
    // Optical depth, straight down.
    depth: Float32Array;
    // Share of the condensed water that is ice.
    ice: Float32Array;
    // m/s, direction of travel, to carry detail along between snapshots.
    windX: Float32Array;
    windZ: Float32Array;
    // WMO genus: cumulus, cumulonimbus, stratus, stratocumulus, nimbostratus, altocumulus, altostratus, cirrus,
    // cirrocumulus, cirrostratus.
    genus: Uint8Array;
  }[];
  // 0 flat sheets to 1 towers.
  heaping: Float32Array;
  // kg/m²/s reaching the ground.
  rain: Float32Array;
  snow: Float32Array;
  // kg/m² of snow lying on the ground, which is mm of water: the mean over the cell's ground, plain and summit
  // alike. A summary only; what lies on a slope is read from the ladder.
  snowpack: Float32Array;
  // Snow by elevation: kg/m² lying on ground WorldInfo.snowLadder[rung] metres above `seaLevel`, entry
  // [(z · cells + x) · rungs + rung]. To read it at a point of the terrain: within a cell nothing below that
  // cell's snowLine, linear from the snow line to the first rung above it and between rungs further up, the end
  // rungs held beyond the ladder; then bilinear between the four cells around the point. This is the simulation's
  // snow, and the weather at the player reads it the same way; the engine and the renderer invent none. Every
  // rung of a cell gets the cell's precipitation: there is no wind drift, aspect or avalanching.
  snowLadder: Float32Array;
  // Metres above `seaLevel` of the lowest snow in each cell.
  snowLine: Float32Array;
  // Metres against the datum.
  seaLevel: number;
  // Visible optical depth of the veil of fine ash and sulphate over each cell: it dims and reddens the sun.
  aerosolDepth: Float32Array;
  // kg/m² of fine ash in the air over each cell, and the metres above `seaLevel` around which most of it floats:
  // where there is an ash cloud to draw. All three are zero until something throws ash into the air.
  ashAloft: Float32Array;
  ashLevel: Float32Array;
}

// A square of the world to chart, north up: centre and edge in metres, `size` pixels along each edge (at most
// 1024). Any position is valid; the planet repeats under it.
export interface ChartRequest {
  x: number;
  z: number;
  span: number;
  size: number;
  // `tiles` of a chart already held: the answer is null when the simulation has finished nothing since.
  unless?: number;
}

// What the simulation holds over that square. Rasters have size² entries, row-major with Z as the row: pixel
// (i, j) is centred at origin + (i + ½, j + ½) · pitch. They are views of one buffer.
export interface Chart {
  // X and Z of the north-west corner, metres.
  origin: [number, number];
  // Metres per pixel.
  pitch: number;
  size: number;
  // Finished tiles the simulation held when the chart was read; see ChartRequest.unless.
  tiles: number;
  // Finest level of detail a chart of this pitch reads, as in `level`; 0 when a pixel is too wide for any tile.
  finest: number;
  // Metres, top of the ground.
  ground: Float32Array;
  // Metres, surface of the sea, a lake or a stream; NaN over dry ground.
  water: Float32Array;
  // m³/yr, the largest mean annual flow crossing the pixel; none where only the planet is known.
  discharge: Float32Array;
  // Metres of glacier ice.
  ice: Float32Array;
  // °C, annual mean at the ground.
  warmth: Float32Array;
  // Metres of loose material on the rock.
  cover: Float32Array;
  // Index into WorldInfo.materials of that rock; plain bedrock where only the planet is known.
  bedrock: Uint8Array;
  // 0 dry, 1 sea, 2 lake, 3 stream.
  waters: Uint8Array;
  // Level of detail the pixel was read from: 0 planet (65 km cells), 1 region (1 km), 2 local (64 m), 3 site (4 m).
  level: Uint8Array;
}

export interface GameBridge {
  // Evolving the planet and finding a place to stand takes seconds to minutes.
  startWorld(options?: WorldOptions): Promise<WorldInfo>;
  advance(command: Command): Promise<Frame>;
  // What the simulation knows of the surface at a point: its fields and the level of detail they come from.
  inspect(x: number, z: number): Promise<Record<string, number | string>>;
  // Read off the engine's thread from whatever is built: it never waits for tiles and never holds up a frame.
  // Asked again later, the same square comes back sharper.
  chart(request: ChartRequest): Promise<Chart | null>;
  quit(): Promise<void>;
}

declare global {
  interface Window {
    derivean: GameBridge;
  }
}
