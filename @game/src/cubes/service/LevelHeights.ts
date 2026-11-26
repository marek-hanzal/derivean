import { instancedArray, uniformArray } from "three/tsl";
import { type StorageInstancedBufferAttribute, type UniformArrayNode, Vector4 } from "three/webgpu";
import type { Point, Quads, WorldInfo } from "../../../contract/GameBridge";
import { HeightSide, NoHeight } from "../constant/land";
import { LevelSlots } from "../constant/levels";

// Cubes along a chunk's edge (CubeChunk), and so along a square of ground.
const Square = 32;
const Blocks = HeightSide / Square;

// The height of the ground under every column of the far levels, for the surface to read the slope of the land a
// coarse cube stands for. Each level keeps a ring of HeightSide² columns on the world's lattice, held in square
// blocks as the chunks bring them: a column's height is the highest top over it among the chunks held over its
// square, in cubes of the level. The ring is a GPU buffer the surface reads (landNormal); a block goes up alone.
// The coarsest level holds a little more than a ring's width: there two squares at opposite rims of the view
// share a block, the last one brought keeps it, and the other rim reads its heights.
export function createLevelHeights(world: WorldInfo) {
  const finest = world.cubes?.finestLevel ?? 0;
  const tiles = (world.cubes?.coarsestLevel ?? finest) - finest + 1;
  const ring = instancedArray(new Float32Array(tiles * HeightSide * HeightSide).fill(NoHeight), "float");
  const buffer = ring.value as StorageInstancedBufferAttribute;
  const data = buffer.array as Float32Array;
  // Per level: where the render origin lies in the ring (x, z columns), the level's first entry, and 1 if held.
  const places: UniformArrayNode<"vec4"> = uniformArray(Array.from({ length: LevelSlots }, () => new Vector4(0, 0, 0, 0)), "vec4");
  const chunks = new Map<number, { square: string; tops: Float32Array }>();
  const squares = new Map<string, { at: number; held: Set<number> }>();
  // The square whose heights a block holds.
  const owners = new Map<number, string>();

  function settle(key: string) {
    const square = squares.get(key);
    if (!square) return;
    if (!square.held.size) squares.delete(key);
    if (owners.get(square.at) !== key) return;
    if (!square.held.size) {
      // The block goes back to the square at the other rim, if one is held there.
      owners.delete(square.at);
      const heir = [...squares].find(([, other]) => other.at === square.at);
      if (heir) {
        owners.set(square.at, heir[0]);
        settle(heir[0]);
        return;
      }
    }
    const block = data.subarray(square.at, square.at + Square * Square).fill(NoHeight);
    for (const id of square.held) {
      const { tops } = chunks.get(id)!;
      for (let index = 0; index < block.length; index++) block[index] = Math.max(block[index], tops[index]);
    }
    buffer.addUpdateRange(square.at, Square * Square);
    buffer.needsUpdate = true;
  }

  function remove(id: number) {
    const chunk = chunks.get(id);
    if (!chunk) return;
    chunks.delete(id);
    squares.get(chunk.square)?.held.delete(id);
    settle(chunk.square);
  }

  return {
    nodes: { ring, places },
    remove,
    // A far chunk's tops: a top is a face of axis 1 that looks up (packQuads), its plane the height above the
    // chunk's floor in cubes.
    add(id: number, level: number, edge: number, origin: Point, quads: Quads) {
      remove(id);
      if (level < finest || level - finest >= tiles) return;
      const [x, z] = [Math.round(origin[0] / edge / Square), Math.round(origin[2] / edge / Square)];
      const floor = Math.round(origin[1] / edge);
      const tops = new Float32Array(Square * Square).fill(NoHeight);
      for (let quad = 0; quad < quads.length / 2; quad++) {
        const shape = quads[quad * 2];
        if ((shape >>> 28 & 3) !== 1 || (shape >>> 30 & 1) === 0) continue;
        const height = floor + (shape & 63);
        const [lowZ, lowX] = [shape >>> 6 & 63, shape >>> 12 & 63];
        const [sizeZ, sizeX] = [(shape >>> 18 & 31) + 1, (shape >>> 23 & 31) + 1];
        for (let column = lowZ; column < lowZ + sizeZ; column++) {
          for (let row = lowX; row < lowX + sizeX; row++) tops[column * Square + row] = Math.max(tops[column * Square + row], height);
        }
      }
      const wrap = (value: number) => ((value % Blocks) + Blocks) % Blocks;
      const key = `${level}:${x}:${z}`;
      const at = (level - finest) * HeightSide * HeightSide + (wrap(z) * Blocks + wrap(x)) * Square * Square;
      const square = squares.get(key) ?? { at, held: new Set<number>() };
      squares.set(key, square);
      owners.set(at, key);
      square.held.add(id);
      chunks.set(id, { square: key, tops });
      settle(key);
    },
    // Where each level's ring lies against the render origin, for the surface to find a column from a point.
    place(origin: { x: number; z: number }, edges: readonly (number | undefined)[]) {
      const wrap = (value: number) => value - Math.floor(value / HeightSide) * HeightSide;
      edges.forEach((edge, level) => {
        const place = places.array[level] as Vector4;
        if (!edge || level < finest || level - finest >= tiles) place.set(0, 0, 0, 0);
        else place.set(wrap(origin.x / edge), wrap(origin.z / edge), (level - finest) * HeightSide * HeightSide, 1);
      });
    },
    dispose() {
      chunks.clear();
      squares.clear();
      owners.clear();
      // Frees the GPU buffer: the renderer drops an attribute's buffer when the attribute is disposed.
      buffer.dispose();
    },
  };
}

export type LevelHeights = ReturnType<typeof createLevelHeights>;
