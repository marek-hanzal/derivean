import { Discard, If, int, max } from "three/tsl";
import type { Node } from "three/webgpu";
import { Face } from "../constant/layout";
import { LevelRows } from "../constant/levels";
import type { LevelTable } from "../type/LevelTable";
import type { FaceNodes } from "./faceNodes";

// The rows of a fragment's level and the cube edge among them.
export function levelRows(table: LevelTable, face: FaceNodes) {
  const first = int(face.field(Face.level)).mul(LevelRows);
  const row = (index: number): Node<"vec4"> => table.node.element(first.add(index));
  return { row, edge: row(0).w };
}

// Leaves a fragment to the level that owns its place: a level is drawn inside its own box and outside the box of
// the next finer one, cut where the box's sides pass, also through a cube. `inside` is a point within the
// fragment's own cube, so a face lying in a box's side goes with its cube. `leaves` picks the rows of the finer
// box and `beyond` is how many metres past its own box the level is still drawn. With `finest`, a pass that
// draws only the levels from that one up, the finer box is left open only to a level that is drawn too: the
// finest level drawn fills its middle, where it is whole.
export function levelClip(row: (index: number) => Node<"vec4">, inside: Node<"vec3">, leaves: 2 | 4, beyond: Node<"float"> | number,
  finest?: Node<"float">) {
  const past = max(row(0).xyz.sub(inside), inside.sub(row(1).xyz));
  If(max(max(past.x, past.y), past.z).greaterThan(beyond), () => { Discard(); });
  const within = inside.sub(row(leaves).xyz).min(row(leaves + 1).xyz.sub(inside));
  const left = within.x.min(within.y).min(within.z).greaterThan(0);
  If(finest ? left.and(row(leaves).w.greaterThanEqual(finest)) : left, () => { Discard(); });
}
