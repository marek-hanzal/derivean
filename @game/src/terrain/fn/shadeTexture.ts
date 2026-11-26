import { DataTexture, NearestFilter, RGBAFormat, UnsignedByteType } from "three";
import type { FarPatch } from "../../../contract/GameBridge";

// A patch's shading raster as a texture: per texel the surface normal's x and z, the surface material, and how
// much sky the place sees. The shader reads single texels and blends them itself, since a material id cannot be
// filtered.
export function shadeTexture(patch: FarPatch) {
  const side = patch.cells * patch.shadeDetail + 1;
  const texture = new DataTexture(patch.shade, side, side, RGBAFormat, UnsignedByteType);
  texture.minFilter = texture.magFilter = NearestFilter;
  texture.needsUpdate = true;
  return texture;
}
