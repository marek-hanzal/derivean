import {
  DepthTexture, FloatType, Matrix4, MeshBasicMaterial, OrthographicCamera, type Scene, Vector3, type WebGLRenderer,
  WebGLRenderTarget,
} from "three";
import {
  TerrainLayer, TerrainShadowDrift, TerrainShadowMs, TerrainShadowSettleMs, TerrainShadowSize, TerrainShadowSpan,
  TerrainShadowTexel,
} from "../constant/shadow";
import type { TerrainShade } from "../type/TerrainShade";

// Metres of depth the sun's view holds either side of the map's middle: the tallest relief at the lowest sun.
const Depth = 160000;

// The shadows mountains cast, as a depth map of the far terrain seen from the sun over a square around the eye.
// It is drawn again when the sun has moved or the eye has drifted, never more often than a couple of times a
// second, and after the terrain has changed, once that has had a few seconds to arrive.
export function createTerrainShadow(scene: Scene, sun: { value: Vector3 }) {
  const target = new WebGLRenderTarget(TerrainShadowSize, TerrainShadowSize, {
    depthTexture: new DepthTexture(TerrainShadowSize, TerrainShadowSize, FloatType),
  });
  const camera = new OrthographicCamera(-TerrainShadowSpan / 2, TerrainShadowSpan / 2, TerrainShadowSpan / 2, -TerrainShadowSpan / 2, 0, Depth * 2);
  camera.layers.set(TerrainLayer);
  const silhouette = new MeshBasicMaterial({ colorWrite: false });
  const uniforms: TerrainShade = {
    terrainShadowMap: { value: target.depthTexture! },
    terrainShadowMatrix: { value: new Matrix4() },
    terrainShadowOn: { value: 0 },
  };
  const toMap = new Matrix4().set(0.5, 0, 0, 0.5, 0, 0.5, 0, 0.5, 0, 0, 0.5, 0.5, 0, 0, 0, 1);
  const centre = new Vector3();
  const drawnSun = new Vector3();
  let drawnAt = -Infinity;
  let drawn = false;
  let stale = true;

  // Poses the sun's view for a render origin; the map's middle stays where it was drawn in the world.
  function pose(origin: Vector3) {
    // The view's up must not lie along the sun's rays.
    camera.up.set(0, Math.abs(drawnSun.y) > 0.9 ? 0 : 1, Math.abs(drawnSun.y) > 0.9 ? 1 : 0);
    camera.position.copy(centre).sub(origin).addScaledVector(drawnSun, Depth);
    camera.lookAt(centre.x - origin.x, centre.y - origin.y, centre.z - origin.z);
    camera.updateMatrixWorld();
    uniforms.terrainShadowMatrix.value.multiplyMatrices(toMap, camera.projectionMatrix).multiply(camera.matrixWorldInverse);
  }

  return {
    uniforms,
    // The far terrain changed: its shadows are out of date.
    touch() { stale = true; },
    // `origin` and `eye` are world metres; `lit` is false at night and under plain shading.
    update(renderer: WebGLRenderer, origin: Vector3, eye: Vector3, lit: boolean, now: number) {
      uniforms.terrainShadowOn.value = Number(lit && drawn);
      if (!lit) return;
      const moved = !drawn || drawnSun.distanceToSquared(sun.value) > 1e-5 || centre.distanceToSquared(eye) > TerrainShadowDrift ** 2;
      if (moved ? now - drawnAt >= TerrainShadowMs : stale && now - drawnAt >= TerrainShadowSettleMs) {
        drawnAt = now;
        stale = false;
        drawn = true;
        drawnSun.copy(sun.value);
        centre.copy(eye).divideScalar(TerrainShadowTexel).round().multiplyScalar(TerrainShadowTexel);
        pose(origin);
        const previous = renderer.getRenderTarget();
        const overridden = scene.overrideMaterial;
        scene.overrideMaterial = silhouette;
        renderer.setRenderTarget(target);
        renderer.clear();
        renderer.render(scene, camera);
        scene.overrideMaterial = overridden;
        renderer.setRenderTarget(previous);
        uniforms.terrainShadowOn.value = 1;
      } else if (drawn) pose(origin);
    },
    dispose() {
      target.depthTexture?.dispose();
      target.dispose();
      silhouette.dispose();
    },
  };
}
