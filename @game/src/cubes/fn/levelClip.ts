import { Discard, If, float, int, interleavedGradientNoise, max, screenCoordinate, uint } from "three/tsl";
import { LevelRows } from "../constant/levels";
import type { FaceNodes } from "./faceNodes";
import type { Node } from "three/webgpu";
import type { LevelTable } from "../type/LevelTable";

// The rows of a fragment's level and the cube edge among them.
export function levelRows(table: LevelTable, face: FaceNodes) {
  const first = int(face.words.y.shiftRight(uint(3)).bitAnd(uint(15))).mul(LevelRows);
  const row = (index: number): Node<"vec4"> => table.node.element(first.add(index));
  return { row, edge: row(0).w };
}

// Leaves a fragment to the level that owns its place: a level is drawn inside its own box and outside the box of
// the next finer one. `inside` is a point within the fragment's own cube, so a face lying in a box's side goes
// with its cube. The finer level thins out over a band beyond its box and the coarser one starts at the box, so
// the two overlap there and nothing opens between them. `leaves` picks the rows of the finer box.
export function levelClip(row: (index: number) => Node<"vec4">, inside: Node<"vec3">, leaves: 2 | 4, thins: boolean) {
  const beyond = max(row(0).xyz.sub(inside), inside.sub(row(1).xyz));
  const outside = max(max(beyond.x, beyond.y), beyond.z);
  const band = thins ? row(1).w.mul(interleavedGradientNoise(screenCoordinate.xy)) : float(0);
  If(outside.greaterThan(band), () => { Discard(); });
  const within = inside.sub(row(leaves).xyz).min(row(leaves + 1).xyz.sub(inside));
  If(within.x.min(within.y).min(within.z).greaterThan(0), () => { Discard(); });
}
