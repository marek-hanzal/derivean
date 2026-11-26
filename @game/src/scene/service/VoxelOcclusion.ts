import {
  Data3DTexture, LinearFilter, RedFormat, RepeatWrapping, UnsignedByteType, Vector2, Vector3, Vector4, type WebGLRenderer,
} from "three";
import type { Point, WorldInfo } from "../../../contract/GameBridge";
import type { Occlusion } from "../type/Occlusion";

// Voxels along a side of the volume, and chunks either side of the player's that it holds.
const Side = 256;
const Around = 3;
// Metres a second of the eye between which the darkening fades out: beyond, chunks pass faster than they are worth
// writing.
const Hurry = [15, 30] as const;

// Which voxels round the player are solid surface, kept in a small volume that wraps: a chunk lies at its world
// position modulo the volume's size. The voxel surface samples it half a voxel off each face, where a solid
// neighbour shows up as a crease to darken. A trick: it stands for the sky light a corner loses. Only the chunks
// nearest the player fit, so creases are shaded near and fade out a few chunks away.
export function createVoxelOcclusion(renderer: WebGLRenderer, world: WorldInfo) {
  const chunk = Math.round(world.chunkMeters / world.voxelSize);
  const slots = Side / chunk;
  if (!Number.isInteger(slots) || slots < Around * 2 + 1) throw new Error("Chunks do not tile the occlusion volume");
  const volume = new Data3DTexture(new Uint8Array(Side ** 3), Side, Side, Side);
  volume.format = RedFormat;
  volume.type = UnsignedByteType;
  volume.minFilter = volume.magFilter = LinearFilter;
  volume.wrapS = volume.wrapT = volume.wrapR = RepeatWrapping;
  volume.unpackAlignment = 1;
  volume.needsUpdate = true;
  const uniforms: Occlusion = {
    voxelOcclusion: { value: volume },
    occlusionPlace: { value: new Vector4(0, 0, 0, 1 / (Side * world.voxelSize)) },
    occlusionReach: { value: new Vector2((Around - 0.5) * world.chunkMeters, 0) },
  };
  const faces = new Map<string, Int16Array>();
  const held = new Map<number, string>();
  const at = new Vector3();
  let centre = "";
  const wrap = (value: number) => ((value % slots) + slots) % slots;

  // The solid cell behind every face of a chunk, as one byte a voxel.
  function shell(quads: Int16Array | undefined) {
    const block = new Uint8Array(chunk ** 3);
    for (let i = 0; quads && i < quads.length; i += 8) {
      const axis = quads[i];
      const u = (axis + 1) % 3;
      const v = (axis + 2) % 3;
      const cell = [0, 0, 0];
      cell[axis] = quads[i + 2] - (quads[i + 1] > 0 ? 1 : 0);
      if (cell[axis] < 0 || cell[axis] >= chunk) continue;
      for (cell[u] = quads[i + 3]; cell[u] < quads[i + 3] + quads[i + 5]; cell[u]++) {
        for (cell[v] = quads[i + 4]; cell[v] < quads[i + 4] + quads[i + 6]; cell[v]++) {
          block[(cell[2] * chunk + cell[1]) * chunk + cell[0]] = 255;
        }
      }
    }
    return block;
  }

  function fill(slot: number, key: string, x: number, y: number, z: number) {
    if (held.get(slot) === key) return;
    held.set(slot, key);
    // A texture the GPU has not seen is copied from memory, which is the cheap path wanted here.
    const block = new Data3DTexture(shell(faces.get(key)), chunk, chunk, chunk);
    block.format = RedFormat;
    block.type = UnsignedByteType;
    block.unpackAlignment = 1;
    renderer.copyTextureToTexture(block, volume, null, at.set(wrap(x) * chunk, wrap(y) * chunk, wrap(z) * chunk));
  }

  // Brings the volume in line with the chunks round the player's chunk.
  function gather(middle: Point) {
    for (let dz = -Around; dz <= Around; dz++) {
      for (let dy = -Around; dy <= Around; dy++) {
        for (let dx = -Around; dx <= Around; dx++) {
          const [x, y, z] = [middle[0] + dx, middle[1] + dy, middle[2] + dz];
          fill((wrap(z) * slots + wrap(y)) * slots + wrap(x), `${x},${y},${z}`, x, y, z);
        }
      }
    }
  }

  return {
    uniforms,
    // A chunk's faces as they arrive; none when it is gone.
    keep(key: Point, quads: Int16Array | undefined) {
      const name = key.join(",");
      if (quads?.length) faces.set(name, quads);
      else faces.delete(name);
      // What the volume holds of it is out of date.
      for (const [slot, owner] of held) if (owner === name) held.delete(slot);
      centre = "";
    },
    // `eye` and `origin` are world metres, `speed` the eye's in metres a second. In a hurry the creases fade
    // out and the volume is left as it is.
    update(origin: Vector3, eye: Vector3, speed: number, dt: number) {
      const size = Side * world.voxelSize;
      uniforms.occlusionPlace.value.set(origin.x / size % 1, origin.y / size % 1, origin.z / size % 1, 1 / size);
      const wanted = 1 - Math.min(Math.max((speed - Hurry[0]) / (Hurry[1] - Hurry[0]), 0), 1);
      const strength = uniforms.occlusionReach.value;
      strength.y += (wanted - strength.y) * Math.min(1, dt * 4);
      if (wanted === 0) return;
      const [x, y, z] = [Math.floor(eye.x / world.chunkMeters), Math.floor(eye.y / world.chunkMeters), Math.floor(eye.z / world.chunkMeters)];
      const name = `${x},${y},${z}`;
      if (name === centre) return;
      centre = name;
      gather([x, y, z]);
    },
    dispose() { volume.dispose(); },
  };
}
