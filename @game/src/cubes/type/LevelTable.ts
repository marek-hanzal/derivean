import type { UniformArrayNode, Vector4 } from "three/webgpu";

// Where each level of cubes may be drawn, as one uniform array every cube surface reads by the level its faces
// carry. A level owns LevelRows rows, all in render space:
//   0  xyz minimum of its own box, w its cube edge in metres
//   1  xyz maximum of its own box, w metres beyond the box that it is still drawn
//   2, 3  minimum and maximum of the box it leaves to finer ground; w of row 2 is the level that draws there
//   4, 5  the same for standing water, which the voxels do not take over
// A box whose minimum lies above its maximum holds nothing.
export interface LevelTable {
  rows: Vector4[];
  node: UniformArrayNode<"vec4">;
}
