import assert from "node:assert/strict";
import test from "node:test";
import type { StorageInstancedBufferAttribute } from "three/webgpu";
import type { WorldInfo } from "../contract/GameBridge";
import { HeightBlocks, LandReach, NoHeight, SlopeWords } from "../src/cubes/constant/land";
import { createLevelHeights } from "../src/cubes/service/LevelHeights";

// The slopes LevelHeights measures as squares come and go must be those the surface measured itself on every
// fragment: the height of each column of a place's square and its neighbours as the GPU holds them, through the
// owner of each place, the near and far reach of landNormal as it was. Each held square's heights must be the
// highest top over each column among its chunks, and every word that changed must lie in a range sent up.
const Square = 32;
const Ring = HeightBlocks * HeightBlocks;

// The tops of a chunk's quads in cubes above its floor, column by column, as LevelHeights reads them.
function topsOf(quads: Uint32Array, floor: number) {
  const tops = new Float64Array(Square * Square).fill(NoHeight);
  for (let quad = 0; quad < quads.length; quad += 2) {
    const shape = quads[quad];
    const [lowZ, lowX, sizeZ, sizeX] = [shape >>> 6 & 63, shape >>> 12 & 63, (shape >>> 18 & 31) + 1, (shape >>> 23 & 31) + 1];
    for (let z = lowZ; z < lowZ + sizeZ; z++) for (let x = lowX; x < lowX + sizeX; x++) tops[z * Square + x] = Math.max(tops[z * Square + x], floor + (shape & 63));
  }
  return tops;
}

// The tops of a chunk as packQuads writes them: up-facing faces of axis 1, planes above the chunk's floor.
function quads(random: () => number) {
  const out: number[] = [];
  for (let quad = 0; quad < 40; quad++) {
    const [lowX, lowZ] = [Math.floor(random() * 32), Math.floor(random() * 32)];
    const sizeX = 1 + Math.floor(random() * Math.min(32 - lowX, 12)), sizeZ = 1 + Math.floor(random() * Math.min(32 - lowZ, 12));
    const plane = Math.floor(random() * 20);
    out.push((1 << 30 | 1 << 28 | (sizeX - 1) << 23 | (sizeZ - 1) << 18 | lowX << 12 | lowZ << 6 | plane) >>> 0, 0);
  }
  return Uint32Array.from(out);
}

// landNormal's former reads, from the GPU's own buffers.
function expected(owners: Float32Array, heights: Float32Array, tile: number, x: number, z: number, column: number) {
  const height = (dx: number, dz: number) => {
    const [cx, cz] = [x * Square + column % Square + dx, z * Square + Math.floor(column / Square) + dz];
    const [sx, sz] = [Math.floor(cx / Square), Math.floor(cz / Square)];
    const place = tile * Ring + (sz & HeightBlocks - 1) * HeightBlocks + (sx & HeightBlocks - 1);
    if (owners[place * 2] !== sx || owners[place * 2 + 1] !== sz) return NoHeight;
    return heights[(place * Square * Square + (cz & 31) * Square + (cx & 31)) * SlopeWords];
  };
  const own = height(0, 0);
  if (!(own > NoHeight / 2)) return undefined;
  const held = (dx: number, dz: number) => { const h = height(dx, dz); return h > NoHeight / 2 ? h : own; };
  const across = (alongX: boolean, reach: number) => {
    let rise = 0;
    for (const side of [-1, 0, 1]) rise += alongX ? held(reach, side) - held(-reach, side) : held(side, reach) - held(side, -reach);
    return rise / (6 * reach);
  };
  const [nearX, nearZ] = [across(true, LandReach[0]), across(false, LandReach[0])];
  return [own, Math.hypot(nearX, nearZ), (nearX + across(true, LandReach[1])) / 2, (nearZ + across(false, LandReach[1])) / 2];
}

test("slopes measured as squares come and go match the slopes the surface read itself", () => {
  let seed = 20261127;
  const random = () => ((seed = (seed * 1103515245 + 12345) % 2147483648) / 2147483648);
  const world = { cubes: { finestLevel: 1, coarsestLevel: 2 } } as unknown as WorldInfo;
  const heights = createLevelHeights(world);
  const owners = (heights.nodes.owners.value as StorageInstancedBufferAttribute).array as Float32Array;
  const columns = (heights.nodes.columns.value as StorageInstancedBufferAttribute).array as Float32Array;
  // What the GPU holds: the buffers as created, then only the ranges each frame names as changed.
  const uploads = [heights.nodes.owners.value, heights.nodes.columns.value] as StorageInstancedBufferAttribute[];
  const mirrors = uploads.map((attribute) => Float32Array.from(attribute.array as Float32Array));
  const upload = () => uploads.forEach((attribute, at) => {
    const array = attribute.array as Float32Array;
    for (const { start, count } of attribute.updateRanges) mirrors[at].set(array.subarray(start, start + count), start);
    attribute.clearUpdateRanges();
  });
  const held = new Map<number, { level: number; x: number; z: number; tops: Float64Array }>();
  let id = 0;
  for (let round = 0; round < 30; round++) {
    // Chunks of two levels over a patch wider than the ring, so places change hands, and some go.
    for (let step = 0; step < 12; step++) {
      if (held.size && random() < 0.35) {
        const gone = [...held.keys()][Math.floor(random() * held.size)];
        heights.remove(gone);
        held.delete(gone);
        continue;
      }
      const level = 1 + Math.floor(random() * 2), edge = 2 ** level;
      const [x, z] = [Math.floor(random() * 20) - 10, Math.floor(random() * 20) - 10];
      const [floor, made] = [Math.floor(random() * 6), quads(random)];
      heights.add(++id, level, edge, [x * Square * edge, floor * edge, z * Square * edge], made);
      held.set(id, { level, x, z, tops: topsOf(made, floor) });
    }
    heights.place({ x: (random() - 0.5) * 2000, y: 0, z: (random() - 0.5) * 2000 }, [undefined, 2, 4]);
    upload();
    for (const [at, mirror] of mirrors.entries()) {
      const array = uploads[at].array as Float32Array;
      const differs = mirror.findIndex((value, index) => !Object.is(value, array[index]));
      assert.equal(differs, -1, `round ${round}: ${at ? "columns" : "owners"} word ${differs} changed without an upload`);
    }
    for (let tile = 0; tile < 2; tile++) {
      for (let spot = 0; spot < Ring; spot++) {
        const place = tile * Ring + spot;
        if (owners[place * 2] < NoHeight / 2) continue;
        const [x, z] = [owners[place * 2], owners[place * 2 + 1]];
        // A square whose last chunk has gone may still be shown as it was; one with chunks shows their tops.
        const chunks = [...held.values()].filter((chunk) => chunk.level === tile + 1 && chunk.x === x && chunk.z === z);
        for (let column = 0; column < Square * Square && chunks.length; column++) {
          const top = Math.max(...chunks.map((chunk) => chunk.tops[column]));
          const at = (place * Square * Square + column) * SlopeWords;
          assert.ok(top < NoHeight / 2 ? columns[at] < NoHeight / 2 : columns[at] === top, `round ${round}, place ${place}, column ${column}: height ${columns[at]} against ${top}`);
        }
        for (let column = 0; column < Square * Square; column++) {
          const want = expected(owners, columns, tile, x, z, column);
          const at = (place * Square * Square + column) * SlopeWords;
          if (!want) {
            assert.ok(columns[at] < NoHeight / 2);
            continue;
          }
          for (let word = 0; word < SlopeWords; word++) {
            assert.ok(Math.abs(columns[at + word] - want[word]) <= 1e-6 * Math.max(1, Math.abs(want[word])),
              `round ${round}, place ${place}, column ${column}, word ${word}: ${columns[at + word]} against ${want[word]}`);
          }
        }
      }
    }
  }
});
