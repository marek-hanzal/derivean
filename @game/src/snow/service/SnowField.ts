import { Color, DataTexture, FloatType, NearestFilter, RedFormat, SRGBColorSpace, Vector4, type Vector3 } from "three/webgpu";
import { texture, uniform, uniformArray } from "three/tsl";
import type { CloudField, WorldInfo } from "../../../contract/GameBridge";
import { SnowMaterial } from "../constant/cover";
import type { SnowNodes } from "../type/SnowNodes";

// The snow the simulation keeps, held for the surfaces that show it: the latest cloud snapshot's ladder and snow
// lines as a texture. Nothing is drawn as snow until a snapshot has come, nor in a world without a snow
// material, where `nodes` is undefined.
export function createSnowField(world: WorldInfo) {
  const rungs = world.snowLadder.length;
  const material = world.materials.find(({ name }) => name === SnowMaterial);
  let field: DataTexture | undefined;
  let grid: { origin: [number, number]; cell: number } | undefined;
  const place = uniform(new Vector4());
  const cell = uniform(1);
  const cells = uniform(1);
  const blank = new DataTexture(new Float32Array(rungs + 1), rungs + 1, 1, RedFormat, FloatType);
  blank.needsUpdate = true;
  const read = texture(blank);
  const nodes: SnowNodes | undefined = material && rungs > 1 ? {
    field: read, rungs, heights: uniformArray(world.snowLadder, "float"), place, cell, cells,
    color: uniform(new Color().setHex(material.color, SRGBColorSpace)),
  } : undefined;

  return {
    nodes,
    // Takes a snapshot.
    apply(clouds: CloudField) {
      const width = clouds.cells * (rungs + 1);
      if (!field || field.image.width !== width || field.image.height !== clouds.cells) {
        field?.dispose();
        field = new DataTexture(new Float32Array(width * clouds.cells), width, clouds.cells, RedFormat, FloatType);
        field.minFilter = field.magFilter = NearestFilter;
        read.value = field;
      }
      const texels = field.image.data as Float32Array;
      for (let at = 0; at < clouds.cells * clouds.cells; at++) {
        texels.set(clouds.snowLadder.subarray(at * rungs, (at + 1) * rungs), at * (rungs + 1));
        texels[at * (rungs + 1) + rungs] = clouds.snowLine[at];
      }
      field.needsUpdate = true;
      grid = { origin: clouds.origin, cell: clouds.cell };
      cell.value = clouds.cell;
      cells.value = clouds.cells;
    },
    // Call when the render origin moves, and after a snapshot.
    rebase(origin: Vector3) {
      if (!grid) return;
      const x = (origin.x - grid.origin[0]) / grid.cell - 0.5;
      const z = (origin.z - grid.origin[1]) / grid.cell - 0.5;
      place.value.set(x - Math.floor(x), z - Math.floor(z), Math.floor(x), Math.floor(z));
    },
    dispose() {
      field?.dispose();
      blank.dispose();
    },
  };
}
