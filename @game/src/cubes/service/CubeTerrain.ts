import { type Group, type Material, type Vector3 } from "three/webgpu";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { BandShare, FreeLevel } from "../constant/levels";
import { type Box, Everywhere, Nowhere, column, setLevel } from "../fn/levelTable";
import type { LevelTable } from "../type/LevelTable";
import { createFaceBatch, type FaceBatch } from "./FaceBatch";

// The ground at every cube size: streamed voxel chunks, the far levels with the standing water they carry, and
// detached pieces, all face lists drawn by one surface. Each cube size is one batch, and so one draw; a piece is
// a batch of its own, posed as the body it is.
export function createCubeTerrain(content: Group, world: WorldInfo, levels: LevelTable, surface: Material, sheet: Material) {
  const voxels = createFaceBatch(content, surface, 0, world.voxelSize);
  const ground: (FaceBatch | undefined)[] = [];
  const water: (FaceBatch | undefined)[] = [];
  const far = new Map<number, { level: number; sheets: number }>();
  const pieces = new Map<number, FaceBatch>();
  let voxelBox: Frame["voxelBox"] = null;
  let boxes: NonNullable<Frame["cubes"]>["levels"] = [];

  function drop(id: number) {
    const chunk = far.get(id);
    if (!chunk) return;
    ground[chunk.level]?.remove(id);
    for (let index = 0; index < chunk.sheets; index++) water[chunk.level]?.remove(`${id}:${index}`);
    far.delete(id);
  }

  function apply(frame: Frame) {
    for (const key of frame.unloaded) voxels.remove(key.join(","));
    for (const { key, origin, quads } of frame.chunks) {
      if (quads) voxels.add(key.join(","), quads, origin);
    }
    for (const { id, faces } of frame.spawned) {
      const piece = createFaceBatch(content, surface, FreeLevel, world.voxelSize);
      piece.fill(id, faces, world.faceWords, [0, 0, 0]);
      pieces.set(id, piece);
    }
    for (const id of frame.retired) {
      pieces.get(id)?.dispose();
      pieces.delete(id);
    }
    for (const { id, position, rotation } of frame.debris) {
      const body = pieces.get(id)?.mesh;
      if (!body) continue;
      body.position.set(...position);
      body.quaternion.set(...rotation);
      body.updateMatrix();
    }
    voxelBox = frame.voxelBox;
    if (!frame.cubes) return;
    for (const id of frame.cubes.removed) drop(id);
    for (const { id, level, edge, origin, quads, water: sheets } of frame.cubes.added) {
      drop(id);
      (ground[level] ??= createFaceBatch(content, surface, level, edge)).add(id, quads, origin);
      // A sheet lies at its water's level, whatever height its chunk is at.
      sheets.forEach(({ level: height, quads: wet }, index) => {
        const batch = water[level] ??= createFaceBatch(content, sheet, level, edge);
        batch.add(`${id}:${index}`, wet, [origin[0], height, origin[2]]);
        if (batch.mesh) batch.mesh.renderOrder = 1;
      });
      far.set(id, { level, sheets: sheets.length });
    }
    boxes = frame.cubes.levels;
  }

  return {
    apply,
    // Says where each level is drawn this frame, seen from the render origin: the voxels inside the footprint of
    // their box, a far level inside its own box and outside the next finer one that has a box.
    place(origin: Vector3) {
      for (const batch of [voxels, ...ground, ...water]) batch?.follow(origin);
      const near: Box = voxelBox ? column(voxelBox) : Nowhere;
      setLevel(levels, 0, origin, world.voxelSize, 0, near, Nowhere, Nowhere);
      setLevel(levels, FreeLevel, origin, world.voxelSize, 0, Everywhere, Nowhere, Nowhere);
      let finerGround = near;
      let finerWater = Nowhere;
      for (const { level, edge, box } of boxes) {
        if (!box) {
          setLevel(levels, level, origin, edge, 0, Nowhere, Nowhere, Nowhere);
          continue;
        }
        setLevel(levels, level, origin, edge, (world.cubes?.band ?? 0) * edge * BandShare, box, finerGround, finerWater);
        finerGround = finerWater = box;
      }
    },
    // Far chunks held, by level.
    counts: () => Array.from(ground, (batch) => batch?.lists ?? 0),
    dispose() {
      for (const batch of [voxels, ...ground, ...water, ...pieces.values()]) batch?.dispose();
      far.clear();
      pieces.clear();
    },
  };
}
