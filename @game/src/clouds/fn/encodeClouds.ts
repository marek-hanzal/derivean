import type { CloudField } from "../../../contract/GameBridge";
import { CloudForms, DensestCloud, FillShare, ThinnestCover } from "../constant/forms";

// Texels per cell: two RGBA layers for each of the three bands.
export const BandLayers = 2;
export const Bands = 3;

// A snapshot as the layers the shaders read, band by band: [cover, base, top, extinction per metre] and
// [ice share, billow, breadth, streak]. Every value is one that blends meaningfully between neighbouring cells
// and between snapshots, which a genus code is not.
export function encodeClouds(field: CloudField) {
  const cells = field.cells * field.cells;
  const layers = new Float32Array(cells * 4 * BandLayers * Bands);
  for (let band = 0; band < Bands; band++) {
    const source = field.bands[band];
    const shape = band * BandLayers * cells * 4;
    const form = shape + cells * 4;
    for (let i = 0; i < cells; i++) {
      const cover = Math.min(1, Math.max(0, source.cover[i]));
      const thickness = Math.max(source.top[i] - source.base[i], 30);
      const within = source.depth[i] / Math.max(cover, ThinnestCover);
      const { billow, breadth, streak } = CloudForms[source.genus[i]] ?? CloudForms[0];
      layers[shape + i * 4] = cover;
      layers[shape + i * 4 + 1] = source.base[i];
      layers[shape + i * 4 + 2] = source.base[i] + thickness;
      layers[shape + i * 4 + 3] = Math.min(DensestCloud, within / thickness / FillShare);
      layers[form + i * 4] = source.ice[i];
      // Heaped low cloud is as heaped as the column's updrafts make it; the genus alone would switch, not grow.
      layers[form + i * 4 + 1] = band === 0 && billow >= 0.5 ? Math.max(0.5, billow * (0.55 + 0.45 * field.heaping[i])) : billow;
      layers[form + i * 4 + 2] = breadth;
      layers[form + i * 4 + 3] = streak * source.ice[i];
    }
  }
  return layers;
}
