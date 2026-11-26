import { Vector3, Vector4 } from "three/webgpu";
import { uniformArray } from "three/tsl";
import type { Point } from "../../../contract/GameBridge";
import { LevelRows, LevelSlots } from "../constant/levels";
import type { LevelTable } from "../type/LevelTable";

export interface Box { min: Point; max: Point }

const Far = 1e9;
export const Everywhere: Box = { min: [-Far, -Far, -Far], max: [Far, Far, Far] };
export const Nowhere: Box = { min: [1, 1, 1], max: [-1, -1, -1] };

// The footprint of a box at any height.
export const column = (box: Box): Box => ({ min: [box.min[0], -Far, box.min[2]], max: [box.max[0], Far, box.max[2]] });

// The largest footprint within a column whose sides lie on a lattice of `edge` metres; nowhere if there is none.
export function snapped(box: Box, edge: number): Box {
  const min: Point = [Math.ceil(box.min[0] / edge) * edge, box.min[1], Math.ceil(box.min[2] / edge) * edge];
  const max: Point = [Math.floor(box.max[0] / edge) * edge, box.max[1], Math.floor(box.max[2] / edge) * edge];
  return min[0] < max[0] && min[2] < max[2] ? { min, max } : Nowhere;
}

export function createLevelTable(): LevelTable {
  const rows = Array.from({ length: LevelSlots * LevelRows }, (_, row) => row % 2 ? new Vector4(-1, -1, -1, 0) : new Vector4(1, 1, 1, 1));
  return { rows, node: uniformArray(rows, "vec4") };
}

// Places a level: its own box and the boxes it leaves to finer ground and finer water, in world metres, seen from
// a render origin. `finer` is the level that draws the ground it leaves.
export function setLevel(table: LevelTable, level: number, origin: Vector3, edge: number, band: number, own: Box, ground: Box, water: Box,
  finer: number) {
  const at = level * LevelRows;
  const put = (row: number, [x, y, z]: Point, w: number) => table.rows[at + row].set(x - origin.x, y - origin.y, z - origin.z, w);
  put(0, own.min, edge);
  put(1, own.max, band);
  put(2, ground.min, finer);
  put(3, ground.max, 0);
  put(4, water.min, 0);
  put(5, water.max, 0);
}
