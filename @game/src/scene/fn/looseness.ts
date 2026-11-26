import type { WorldInfo } from "../../../contract/GameBridge";

// For each face corner of a voxel face list, whether its material is loose grains (255) or solid (0): what the
// ground shader needs to tell sand from rock.
export function looseness(faces: Int16Array, world: WorldInfo) {
  const loose = new Uint8Array(faces.length / 2);
  for (let quad = 0; quad < faces.length / 8; quad++) {
    loose.fill(world.materials[faces[quad * 8 + 7]].matter === "granular" ? 255 : 0, quad * 4, quad * 4 + 4);
  }
  return loose;
}
