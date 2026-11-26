import { type Group, type Material, type Vector3 } from "three/webgpu";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { FreeLevel, Overlap } from "../constant/levels";
import { levelSquares } from "../fn/levelSquares";
import { type Box, Everywhere, Nowhere, setLevel } from "../fn/levelTable";
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
    // Says where each level is drawn this frame, seen from the render origin (levelSquares), and closes each
    // far level along the square it leaves to the finer ones. A level reaches a sliver into the coarser one
    // around it, so the two meet without a crack.
    place(origin: Vector3) {
      for (const batch of [voxels, ...ground, ...water]) batch?.follow(origin);
      setLevel(levels, FreeLevel, origin, world.voxelSize, 0, Everywhere, Nowhere, Nowhere);
      const all = [{ level: 0, edge: world.voxelSize, box: voxelBox }, ...boxes];
      const squares = levelSquares(all);
      let finerGround: Box = Nowhere;
      let finerWater: Box = Nowhere;
      for (const { level, edge } of all) {
        const at = squares.findIndex((square) => square.level === level);
        const own = at < 0 ? Nowhere : squares[at].own;
        const shown = own !== Nowhere;
        setLevel(levels, level, origin, edge, shown ? (squares[at + 1]?.edge ?? edge) * Overlap : 0, own, finerGround, finerWater);
        if (shown && finerGround !== Nowhere) ground[level]?.cap(finerGround.min, finerGround.max);
        else ground[level]?.cap();
        if (!shown) continue;
        finerGround = own;
        if (level) finerWater = own;
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
