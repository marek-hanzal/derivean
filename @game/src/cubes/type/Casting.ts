import type { Group, Material } from "three/webgpu";

// How a batch of faces casts shadows: a second mesh over the same faces, among the casters, drawn by the depth
// surface into the cascade whose layer it is on.
export interface Casting {
  group: Group;
  material: Material;
  layer: number;
}
