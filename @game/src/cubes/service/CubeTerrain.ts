import { type Group, type Material, type Vector3 } from "three/webgpu";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { FreeLevel, LevelSlots, Overlap } from "../constant/levels";
import { levelSquares } from "../fn/levelSquares";
import { type Box, Everywhere, Nowhere, setLevel } from "../fn/levelTable";
import type { Casting } from "../type/Casting";
import type { LevelTable } from "../type/LevelTable";
import { createFaceBatch, type FaceBatch } from "./FaceBatch";
import type { LevelHeights } from "./LevelHeights";

// The ground at every cube size: streamed voxel chunks, the far levels with the standing water they carry, and
// detached pieces, all face lists drawn by one surface. Each cube size is one batch, and so one draw; a piece is
// a batch of its own, posed as the body it is. The ground casts shadows, each level into the cascade `casting`
// names for it; water does not. `heights`, where given, keeps the far levels' ground heights for the surface.
export function createCubeTerrain(content: Group, world: WorldInfo, levels: LevelTable, surface: Material, sheet: Material,
  casting: (level: number) => Casting, heights?: LevelHeights) {
  const side = world.cubes?.side ?? world.chunkMeters / world.voxelSize;
  const voxels = createFaceBatch(content, surface, 0, world.voxelSize, side, casting(0));
  const ground: (FaceBatch | undefined)[] = [];
  const water: (FaceBatch | undefined)[] = [];
  const far = new Map<number, { level: number; sheets: number }>();
  const pieces = new Map<number, FaceBatch>();
  // Every level drawn, the voxels first, and the edge of each far level by level: kept from frame to frame and
  // changed when the far levels are.
  const all: { level: number; edge: number; box: Frame["voxelBox"] }[] = [{ level: 0, edge: world.voxelSize, box: null }];
  const edges: (number | undefined)[] = [];
  // Where each level is among the squares of a frame, -1 for none.
  const squareAt = new Array<number>(LevelSlots).fill(-1);
  // Pieces that came, went or moved.
  let stirred = 0;

  function drop(id: number) {
    const chunk = far.get(id);
    if (!chunk) return;
    ground[chunk.level]?.remove(id);
    heights?.remove(id);
    for (let index = 0; index < chunk.sheets; index++) water[chunk.level]?.remove(`${id}:${index}`);
    far.delete(id);
  }

  function apply(frame: Frame) {
    for (const key of frame.unloaded) voxels.remove(key.join(","));
    for (const { key, origin, quads } of frame.chunks) {
      if (quads) voxels.add(key.join(","), quads, origin);
    }
    for (const { id, faces } of frame.spawned) {
      const piece = createFaceBatch(content, surface, FreeLevel, world.voxelSize, side, casting(0));
      piece.fill(id, faces, world.faceWords, [0, 0, 0]);
      pieces.set(id, piece);
    }
    for (const id of frame.retired) {
      pieces.get(id)?.dispose();
      pieces.delete(id);
    }
    for (const { id, position, rotation } of frame.debris) pieces.get(id)?.pose(position, rotation);
    stirred += frame.spawned.length + frame.retired.length + frame.debris.length;
    all[0].box = frame.voxelBox;
    if (!frame.cubes) return;
    for (const id of frame.cubes.removed) drop(id);
    for (const { id, level, edge, origin, quads, water: sheets } of frame.cubes.added) {
      drop(id);
      (ground[level] ??= createFaceBatch(content, surface, level, edge, side, casting(level))).add(id, quads, origin);
      heights?.add(id, level, edge, origin, quads);
      // A sheet lies at its water's level, whatever height its chunk is at.
      sheets.forEach(({ level: height, quads: wet }, index) => {
        const batch = water[level] ??= createFaceBatch(content, sheet, level, edge, side);
        batch.add(`${id}:${index}`, wet, [origin[0], height, origin[2]]);
        if (batch.mesh) batch.mesh.renderOrder = 1;
      });
      far.set(id, { level, sheets: sheets.length });
    }
    all.length = 1;
    edges.length = 0;
    for (const box of frame.cubes.levels) {
      all.push(box);
      edges[box.level] = box.edge;
    }
  }

  return {
    apply,
    // Says where each level is drawn this frame, seen from the render origin (levelSquares), and closes each
    // far level along the square it leaves to the finer ones. A level reaches a sliver into the coarser one
    // around it, so the two meet without a crack. Returns the squares, from the finest level drawn.
    place(origin: Vector3) {
      voxels.follow(origin);
      for (const batch of ground) batch?.follow(origin);
      for (const batch of water) batch?.follow(origin);
      setLevel(levels, FreeLevel, origin, world.voxelSize, 0, Everywhere, Nowhere, Nowhere, -1);
      heights?.place(origin, edges);
      const squares = levelSquares(all);
      squareAt.fill(-1);
      squares.forEach((square, index) => { squareAt[square.level] = index; });
      let finerGround: Box = Nowhere;
      let finerWater: Box = Nowhere;
      let finer = -1;
      for (const { level, edge } of all) {
        const at = squareAt[level];
        const own = at < 0 ? Nowhere : squares[at].own;
        const shown = own !== Nowhere;
        setLevel(levels, level, origin, edge, shown ? (squares[at + 1]?.edge ?? edge) * Overlap : 0, own, finerGround, finerWater, finer);
        if (shown && finerGround !== Nowhere) ground[level]?.cap(finerGround.min, finerGround.max);
        else ground[level]?.cap();
        if (!shown) continue;
        finerGround = own;
        finer = level;
        if (level) finerWater = own;
      }
      return squares;
    },
    // A number that changes whenever the faces of the levels `chosen` picks have.
    stamp(chosen: (level: number) => boolean) {
      let sum = chosen(0) ? voxels.revision + stirred : 0;
      ground.forEach((batch, level) => {
        if (batch && chosen(level)) sum += batch.revision;
      });
      return sum;
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
