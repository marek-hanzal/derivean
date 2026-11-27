import { instancedArray, uniform, uniformArray } from "three/tsl";
import { type StorageInstancedBufferAttribute, type UniformArrayNode, Vector4 } from "three/webgpu";
import type { Point, Quads, WorldInfo } from "../../../contract/GameBridge";
import { HeightBlocks, NoHeight, SlopeMargin, SlopeWords } from "../constant/land";
import { LevelSlots } from "../constant/levels";
import { landSlopes } from "../fn/landSlopes";

// Cubes along a chunk's edge (CubeChunk), and so along a square of ground.
const Square = 32;
const Ring = HeightBlocks * HeightBlocks;
const wrap = (value: number) => ((value % HeightBlocks) + HeightBlocks) % HeightBlocks;

// The height of the ground under every column of the far levels, for the surface to read the slope of the land a
// coarse cube stands for (landNormal). Each level keeps a ring of HeightBlocks² squares of ground on the world's
// lattice as the chunks bring them: a column's height is the highest top over it among the chunks held over its
// square, in cubes of the level. A level holds more squares than its ring has places for (its margin beyond the
// view), so a place goes to the square nearer the eye, and the ring says which square each place holds: a
// column of any other square reads as not held.
//
// The slope round a column changes only when a square of ground does, so it is measured here (landSlopes) as the
// squares come and go, not by every fragment: the GPU holds each column's height and slope, and the owner of each
// place. A place changed changes the slopes within the far reach of its old square and of its new one; those are
// measured again when the frame is placed, and a square's columns go up together.
export function createLevelHeights(world: WorldInfo) {
  const finest = world.cubes?.finestLevel ?? 0;
  const tiles = (world.cubes?.coarsestLevel ?? finest) - finest + 1;
  const ring = new Float32Array(tiles * Ring * Square * Square).fill(NoHeight);
  const slopes = new Float32Array(tiles * Ring * Square * Square * SlopeWords);
  for (let column = 0; column < slopes.length; column += SlopeWords) slopes[column] = NoHeight;
  const columns = instancedArray(slopes, "vec4");
  const owners = instancedArray(new Float32Array(tiles * Ring * 2).fill(NoHeight), "float");
  const measured = columns.value as StorageInstancedBufferAttribute;
  const held = owners.value as StorageInstancedBufferAttribute;
  // Per level: where the render origin lies, as the square it is in (z, w) and the columns past that square's
  // corner (x, y).
  const places: UniformArrayNode<"vec4"> = uniformArray(Array.from({ length: LevelSlots }, () => new Vector4()), "vec4");
  // Per level: the first place of its ring, or -1 where it has none.
  const firsts: UniformArrayNode<"float"> = uniformArray(new Array<number>(LevelSlots).fill(-1), "float");
  // The render origin's height, metres.
  const lift = uniform(0);
  const chunks = new Map<number, { square: string; tops: Float32Array }>();
  const squares = new Map<string, { level: number; x: number; z: number; place: number; held: Set<number> }>();
  const owner = new Map<number, string>();
  // The squares that want each place.
  const wanting = new Map<number, Set<string>>();
  const eye = { x: 0, z: 0 };
  const edges: number[] = [];
  // The square each place holds as the GPU sees it, and the places whose slopes are to be measured again, with
  // the columns [x0, x1) × [z0, z1) that changed.
  const shown: ({ level: number; x: number; z: number } | undefined)[] = new Array(tiles * Ring);
  const stale = new Map<number, [number, number, number, number]>();
  const around = new Float32Array((Square + 2 * SlopeMargin) ** 2);
  const placeOf = (level: number, x: number, z: number) => (level - finest) * Ring + wrap(z) * HeightBlocks + wrap(x);
  const holds = (place: number, level: number, x: number, z: number) => {
    const square = shown[place];
    return square !== undefined && square.level === level && square.x === x && square.z === z;
  };

  // Marks the columns within the far reach of a square whose place changed, in the squares round it that are shown.
  function stir(square: { level: number; x: number; z: number } | undefined) {
    if (!square) return;
    for (let dz = -1; dz <= 1; dz++) {
      for (let dx = -1; dx <= 1; dx++) {
        const place = placeOf(square.level, square.x + dx, square.z + dz);
        if (!holds(place, square.level, square.x + dx, square.z + dz)) continue;
        const [x0, x1] = dx < 0 ? [Square - SlopeMargin, Square] : dx > 0 ? [0, SlopeMargin] : [0, Square];
        const [z0, z1] = dz < 0 ? [Square - SlopeMargin, Square] : dz > 0 ? [0, SlopeMargin] : [0, Square];
        const was = stale.get(place);
        stale.set(place, was ? [Math.min(was[0], x0), Math.max(was[1], x1), Math.min(was[2], z0), Math.max(was[3], z1)] : [x0, x1, z0, z1]);
      }
    }
  }

  // Shows `square` at `place`, or nothing.
  function show(place: number, square: { level: number; x: number; z: number } | undefined) {
    const was = shown[place];
    shown[place] = square && { level: square.level, x: square.x, z: square.z };
    stir(was);
    stir(square);
  }

  // Measures the slopes of the places that changed.
  function measure() {
    const width = Square + 2 * SlopeMargin;
    for (const [place, [x0, x1, z0, z1]] of stale) {
      const square = shown[place];
      if (!square) continue;
      around.fill(NoHeight);
      for (let dz = -1; dz <= 1; dz++) {
        for (let dx = -1; dx <= 1; dx++) {
          const from = placeOf(square.level, square.x + dx, square.z + dz);
          if (!holds(from, square.level, square.x + dx, square.z + dz)) continue;
          // The part of that square within the window, in its own columns.
          const [lowX, highX] = [Math.max(0, -SlopeMargin - dx * Square), Math.min(Square, Square + SlopeMargin - dx * Square)];
          const [lowZ, highZ] = [Math.max(0, -SlopeMargin - dz * Square), Math.min(Square, Square + SlopeMargin - dz * Square)];
          for (let z = lowZ; z < highZ; z++) {
            const start = from * Square * Square + z * Square;
            around.set(ring.subarray(start + lowX, start + highX), (z + dz * Square + SlopeMargin) * width + dx * Square + SlopeMargin + lowX);
          }
        }
      }
      landSlopes(around, Square, slopes.subarray(place * Square * Square * SlopeWords, (place + 1) * Square * Square * SlopeWords), x0, x1, z0, z1);
      measured.addUpdateRange(place * Square * Square * SlopeWords, Square * Square * SlopeWords);
      measured.needsUpdate = true;
    }
    stale.clear();
  }

  // Squares of the level between a square and the one the eye is in.
  const away = (square: { level: number; x: number; z: number }) => {
    const span = (edges[square.level] ?? 1) * Square;
    return Math.max(Math.abs(square.x + 0.5 - eye.x / span), Math.abs(square.z + 0.5 - eye.z / span));
  };

  function write(key: string) {
    const square = squares.get(key)!;
    const block = ring.subarray(square.place * Square * Square, (square.place + 1) * Square * Square).fill(NoHeight);
    for (const id of square.held) {
      const { tops } = chunks.get(id)!;
      for (let index = 0; index < block.length; index++) block[index] = Math.max(block[index], tops[index]);
    }
    (held.array as Float32Array).set([square.x, square.z], square.place * 2);
    held.addUpdateRange(square.place * 2, 2);
    held.needsUpdate = true;
    show(square.place, square);
  }

  // Gives a place to the nearest held square that wants it, or empties it.
  function award(place: number) {
    let best: string | undefined;
    for (const key of wanting.get(place) ?? []) {
      if (!best || away(squares.get(key)!) < away(squares.get(best)!)) best = key;
    }
    if (best === owner.get(place)) return;
    if (best) {
      owner.set(place, best);
      write(best);
      return;
    }
    owner.delete(place);
    (held.array as Float32Array).fill(NoHeight, place * 2, place * 2 + 2);
    held.addUpdateRange(place * 2, 2);
    held.needsUpdate = true;
    show(place, undefined);
  }

  function remove(id: number) {
    const chunk = chunks.get(id);
    if (!chunk) return;
    chunks.delete(id);
    const square = squares.get(chunk.square)!;
    square.held.delete(id);
    if (!square.held.size) {
      squares.delete(chunk.square);
      wanting.get(square.place)?.delete(chunk.square);
    }
    if (owner.get(square.place) === chunk.square) owner.delete(square.place);
    award(square.place);
  }

  return {
    // `finest`: the finest level that keeps heights; finer ground is measured on it.
    nodes: { columns, owners, places, firsts, lift, finest },
    remove,
    // A far chunk's tops: a top is a face of axis 1 that looks up (packQuads), its plane the height above the
    // chunk's floor in cubes. A chunk with no top adds nothing.
    add(id: number, level: number, edge: number, origin: Point, quads: Quads) {
      remove(id);
      if (level < finest || level - finest >= tiles) return;
      const floor = Math.round(origin[1] / edge);
      const tops = new Float32Array(Square * Square).fill(NoHeight);
      let any = false;
      for (let quad = 0; quad < quads.length / 2; quad++) {
        const shape = quads[quad * 2];
        if ((shape >>> 28 & 3) !== 1 || (shape >>> 30 & 1) === 0) continue;
        const height = floor + (shape & 63);
        const [lowZ, lowX] = [shape >>> 6 & 63, shape >>> 12 & 63];
        const [sizeZ, sizeX] = [(shape >>> 18 & 31) + 1, (shape >>> 23 & 31) + 1];
        for (let z = lowZ; z < lowZ + sizeZ; z++) {
          for (let x = lowX; x < lowX + sizeX; x++) tops[z * Square + x] = Math.max(tops[z * Square + x], height);
        }
        any = true;
      }
      if (!any) return;
      edges[level] = edge;
      const [x, z] = [Math.round(origin[0] / edge / Square), Math.round(origin[2] / edge / Square)];
      const key = `${level}:${x}:${z}`;
      const place = placeOf(level, x, z);
      const square = squares.get(key) ?? { level, x, z, place, held: new Set<number>() };
      squares.set(key, square);
      if (!wanting.has(place)) wanting.set(place, new Set());
      wanting.get(place)!.add(key);
      square.held.add(id);
      chunks.set(id, { square: key, tops });
      if (owner.get(place) === key) write(key);
      else award(place);
    },
    // Where each level's ring lies against the render origin, for the surface to find a column from a point.
    // `drawn` holds the edge of every level the frame names, by level.
    place(origin: { x: number; y: number; z: number }, drawn: readonly (number | undefined)[]) {
      measure();
      eye.x = origin.x;
      eye.z = origin.z;
      lift.value = origin.y;
      for (let level = 0; level < LevelSlots; level++) {
        const edge = drawn[level];
        const ours = edge !== undefined && level >= finest && level - finest < tiles;
        firsts.array[level] = ours ? (level - finest) * Ring : -1;
        if (!ours) continue;
        const [x, z] = [origin.x / edge / Square, origin.z / edge / Square];
        (places.array[level] as Vector4).set((x - Math.floor(x)) * Square, (z - Math.floor(z)) * Square, Math.floor(x), Math.floor(z));
      }
    },
    dispose() {
      chunks.clear();
      squares.clear();
      owner.clear();
      wanting.clear();
      stale.clear();
      shown.fill(undefined);
      // Frees the GPU buffers: the renderer drops an attribute's buffer when the attribute is disposed.
      measured.dispose();
      held.dispose();
    },
  };
}

export type LevelHeights = ReturnType<typeof createLevelHeights>;
