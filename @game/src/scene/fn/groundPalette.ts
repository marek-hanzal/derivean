import { DataTexture, NearestFilter, RGBAFormat, SRGBColorSpace, UnsignedByteType } from "three/webgpu";
import type { WorldInfo } from "../../../contract/GameBridge";

// The world's materials as a strip the far terrain looks its surface up in by id: the material's colour, and in
// alpha whether it is loose grains. It is the same colour the voxels of that material get.
export function groundPalette(world: WorldInfo) {
  const texels = new Uint8Array(world.materials.length * 4);
  world.materials.forEach(({ color, matter }, id) => {
    texels.set([color >> 16 & 255, color >> 8 & 255, color & 255, matter === "granular" ? 255 : 0], id * 4);
  });
  const palette = new DataTexture(texels, world.materials.length, 1, RGBAFormat, UnsignedByteType);
  palette.colorSpace = SRGBColorSpace;
  palette.minFilter = palette.magFilter = NearestFilter;
  palette.needsUpdate = true;
  return palette;
}
