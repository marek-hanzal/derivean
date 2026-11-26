import { BackSide, MeshBasicNodeMaterial, type UniformNode } from "three/webgpu";
import { Fn, positionWorld, vec4 } from "three/tsl";
import { faceNodes } from "../../cubes/fn/faceNodes";
import { levelClip, levelRows } from "../../cubes/fn/levelClip";
import type { LevelTable } from "../../cubes/type/LevelTable";

// What the cubes are drawn with into a shadow map: depth alone, of the faces that look away from the sun. A ray
// of sunlight leaves solid ground only through such a face, so the nearest of them along a ray is where shadow
// begins, and the faces the sun lights are not in the map to shadow themselves. Each level is drawn where it
// owns the ground, as on screen, except that the finest level of the map fills its middle (levelClip).
export function casterSurface(levels: LevelTable, finest: UniformNode<"float", number>) {
  const material = new MeshBasicNodeMaterial({ side: BackSide, fog: false });
  material.colorWrite = false;
  const face = faceNodes();
  material.positionNode = face.corner;
  material.colorNode = Fn(() => {
    const { row, edge } = levelRows(levels, face);
    levelClip(row, positionWorld.sub(face.normal.normalize().mul(edge.mul(0.01))), 2, row(1).w, finest);
    return vec4(0, 0, 0, 1);
  })();
  return material;
}
