import {
  DepthTexture, FloatType, GreaterEqualCompare, Group, LinearFilter, Matrix4, OrthographicCamera, RenderTarget, Scene, Vector3, Vector4,
  type WebGPURenderer,
} from "three/webgpu";
import { uniform, uniformArray } from "three/tsl";
import type { WorldInfo } from "../../../contract/GameBridge";
import type { Box } from "../../cubes/fn/levelTable";
import type { LevelTable } from "../../cubes/type/LevelTable";
import {
  Cascades, DrawEvery, LevelsPerCascade, MapDepth, MapRoom, MapSizes, MoveShare, SunTurn, TexelBias, cascadeOf,
} from "../constant/cascades";
import { casterSurface } from "../fn/casterSurface";
import type { ShadowNodes } from "../type/ShadowNodes";
import type { CloudShade } from "../../clouds/type/CloudShade";

// What a cascade's map was last drawn for, in world metres.
interface Drawn { centre: Vector3; sun: Vector3; square: Box; stamp: number; frame: number; clip: Matrix4 }

export interface ShadowView {
  // The render origin, the eye and the ground under it (null if unknown), in world metres.
  origin: Vector3;
  eye: Vector3;
  ground: number | null;
  // Unit vector towards the light.
  sun: Vector3;
  // The square of ground each level is drawn in this frame.
  squares: readonly { level: number; own: Box }[];
  // A number that changes whenever the faces cast into a cascade have.
  stamp(cascade: number): number;
}

// The sun's shadows over the whole view (constant/cascades). The casters are a scene of their own, the cube
// batches' second meshes. The clouds' shadows, where the world has them, are read with these; a cascade is drawn again only when the eye, the sun or its casters have moved, the
// coarse ones seldom, so a still view costs nothing. The depth buffer is reversed, as the renderer's is.
export function createSunShadow(world: WorldInfo, levels: LevelTable, clouds?: CloudShade) {
  const casters = new Scene();
  casters.matrixAutoUpdate = false;
  const group = new Group();
  group.matrixAutoUpdate = false;
  casters.add(group);
  const finest = uniform(0);
  const material = casterSurface(levels, finest);

  const matrices = Array.from({ length: Cascades }, () => new Matrix4());
  const squares = Array.from({ length: Cascades }, () => new Vector4(1, 1, -1, -1));
  const rays = Array.from({ length: Cascades }, () => new Vector4(0, 1, 0, 0));
  const view = world.cubes?.viewDistance ?? world.far.viewDistance;
  const reach = world.cubes?.reach ?? 128;
  const cascades = MapSizes.map((size, index) => {
    const depth = new DepthTexture(size, size, FloatType);
    depth.compareFunction = GreaterEqualCompare;
    depth.minFilter = depth.magFilter = LinearFilter;
    // Depth alone: a cascade has no colour to write.
    const target = new RenderTarget(size, size, { depthTexture: depth, count: 0, generateMipmaps: false });
    // Half the width of the box of the coarsest level cast into the cascade; the last one's is the view.
    const half = index === Cascades - 1 ? view : reach * world.voxelSize * 2 ** ((index + 1) * LevelsPerCascade - 1);
    const camera = new OrthographicCamera(-half * MapRoom[index], half * MapRoom[index], half * MapRoom[index], -half * MapRoom[index], 0, 2 * half * MapDepth);
    camera.layers.set(index);
    // `kept` is written over at every draw; `drawn` is it once the map holds something.
    const kept: Drawn = { centre: new Vector3(), sun: new Vector3(), square: { min: [0, 0, 0], max: [0, 0, 0] }, stamp: 0, frame: 0, clip: new Matrix4() };
    return { size, depth, target, camera, half, kept, drawn: undefined as Drawn | undefined };
  });
  const nodes: ShadowNodes = {
    maps: cascades.map(({ depth }) => depth),
    matrices: uniformArray(matrices, "mat4"), squares: uniformArray(squares, "vec4"), rays: uniformArray(rays, "vec4"), finest, clouds,
  };

  const scratch = new Matrix4();
  const across = new Vector3();
  const up = new Vector3();
  const centre = new Vector3();
  let frame = 0;

  // The square of ground a cascade's own levels fill: that of the coarsest of them that is drawn.
  function squareOf(index: number, drawn: ShadowView["squares"]) {
    let found: Box | undefined;
    for (const { level, own } of drawn) if (cascadeOf(level) === index && own.min[0] < own.max[0]) found = own;
    return found;
  }

  function draw(renderer: WebGPURenderer, index: number, { origin, eye, ground, sun, stamp }: ShadowView, square: Box) {
    const cascade = cascades[index];
    const { camera, half, size } = cascade;
    const depth = half * MapDepth;
    camera.up.set(0, Math.abs(sun.y) > 0.99 ? 0 : 1, Math.abs(sun.y) > 0.99 ? 1 : 0);
    camera.position.copy(sun);
    camera.lookAt(0, 0, 0);
    camera.updateMatrixWorld();
    across.setFromMatrixColumn(camera.matrixWorld, 0);
    up.setFromMatrixColumn(camera.matrixWorld, 1);
    // The centre moves in whole texels across the map, so a still caster keeps the texels it was drawn into.
    const texel = 2 * half * MapRoom[index] / size;
    centre.set(eye.x, ground ?? eye.y, eye.z);
    const snap = (axis: Vector3) => centre.addScaledVector(axis, Math.round(centre.dot(axis) / texel) * texel - centre.dot(axis));
    snap(across);
    snap(up);
    camera.position.copy(centre).sub(origin).addScaledVector(sun, depth);
    camera.updateMatrixWorld();
    finest.value = index * LevelsPerCascade;
    renderer.setRenderTarget(cascade.target);
    renderer.render(casters, camera);
    renderer.setRenderTarget(null);
    // The renderer has given the camera its reversed projection by now.
    const { kept } = cascade;
    kept.clip.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse).multiply(scratch.makeTranslation(-origin.x, -origin.y, -origin.z));
    kept.centre.copy(centre);
    kept.sun.copy(sun);
    kept.square = square;
    kept.stamp = stamp(index);
    kept.frame = frame;
    cascade.drawn = kept;
  }

  return {
    nodes,
    // Where the cube batches put their second meshes, and what draws them.
    casting: (level: number) => ({ group, material, layer: cascadeOf(level) }),
    // Draws the cascades that are due and places every map for this frame's render origin. Call it before the
    // view is drawn, once the levels are placed.
    cast(renderer: WebGPURenderer, state: ShadowView) {
      frame++;
      group.position.copy(state.origin).negate();
      group.updateMatrix();
      let next = -1;
      let urgency = 1;
      cascades.forEach((cascade, index) => {
        const square = squareOf(index, state.squares);
        if (!square) {
          cascade.drawn = undefined;
          return;
        }
        const { drawn } = cascade;
        if (!drawn) return draw(renderer, index, state, square);
        const moved = Math.hypot(state.eye.x - drawn.centre.x, state.eye.z - drawn.centre.z) > cascade.half * MoveShare;
        if (!moved && drawn.sun.angleTo(state.sun) <= SunTurn && state.stamp(index) === drawn.stamp) return;
        // The cascade that has waited longest for its turn goes first; of two that have, the coarser.
        const waited = (frame - drawn.frame) / DrawEvery[index];
        if (waited >= urgency) {
          urgency = waited;
          next = index;
        }
      });
      if (next >= 0) draw(renderer, next, state, squareOf(next, state.squares)!);
      cascades.forEach(({ drawn, half, size }, index) => {
        if (!drawn) return squares[index].set(1, 1, -1, -1);
        const { square, sun, clip } = drawn;
        const { origin } = state;
        squares[index].set(square.min[0] - origin.x, square.min[2] - origin.z, square.max[0] - origin.x, square.max[2] - origin.z);
        const placed = matrices[index].copy(clip).multiply(scratch.makeTranslation(origin.x, origin.y, origin.z));
        // Depth gained by a metre towards the sun: the matrix's third row applied to the sun's direction.
        const [, , x, , , , y, , , , z] = placed.elements;
        const rate = x * sun.x + y * sun.y + z * sun.z;
        placed.elements[14] += rate * TexelBias * 2 * half * MapRoom[index] / size;
        rays[index].set(sun.x, sun.y, sun.z, rate);
      });
    },
    dispose() {
      for (const { target, depth } of cascades) {
        target.dispose();
        depth.dispose();
      }
      material.dispose();
    },
  };
}

export type SunShadow = ReturnType<typeof createSunShadow>;
