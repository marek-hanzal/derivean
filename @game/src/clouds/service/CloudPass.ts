import {
  type DepthTexture, HalfFloatType, LinearFilter, Matrix4, type PerspectiveCamera, RGBAFormat, Vector2, Vector3,
  type WebGLRenderer, WebGLRenderTarget,
} from "three";
import type { GpuTimer } from "../../diagnostics/service/GpuTimer";
import type { SkyLight } from "../../sky/type/SkyLight";
import { MarchScale, Persistence } from "../constant/march";
import { composeMaterial } from "../fn/composeMaterial";
import { fullscreen } from "../fn/fullscreen";
import { marchMaterial } from "../fn/marchMaterial";
import { resolveMaterial } from "../fn/resolveMaterial";
import type { CloudUniforms } from "../type/CloudUniforms";

function target() {
  const made = new WebGLRenderTarget(1, 1, { format: RGBAFormat, type: HalfFloatType, depthBuffer: false });
  made.texture.minFilter = made.texture.magFilter = LinearFilter;
  return made;
}

// The cloud pass of the renderer's output chain: march at reduced size, build the picture up over frames, and lay
// it over the scene by depth. It draws after the whole scene, so no far plane or dome can clip a cloud.
export function createCloudPass(
  clouds: CloudUniforms, light: SkyLight, timer: GpuTimer, far: number, farthest: number, haze: number,
) {
  const view = {
    sceneDepth: { value: null as DepthTexture | null },
    cloudCamera: { value: new Matrix4() },
    cloudLens: { value: new Vector2() },
    depthBase: { value: Math.log2(far + 1) },
    farthest: { value: farthest },
    hazeDensity: { value: haze },
    jitter: { value: 0 },
  };
  const marched = target();
  const memory = [target(), target()];
  const picture = { value: memory[0].texture };
  const past = {
    marched: { value: marched.texture },
    remembered: picture,
    pastClip: { value: new Matrix4() },
    eyeMoved: { value: new Vector3() },
    marchTexel: { value: new Vector2() },
    persistence: { value: 0 },
  };
  const march = marchMaterial({ ...clouds, ...light, ...view });
  const resolve = resolveMaterial({ ...past, cloudCamera: view.cloudCamera, cloudLens: view.cloudLens, cloudEye: clouds.cloudEye, cloudDatum: clouds.cloudDatum, cloudRange: clouds.cloudRange });
  const compose = composeMaterial({ sceneColor: { value: null }, sceneDepth: view.sceneDepth, remembered: picture, marchSize: { value: new Vector2() } });
  const turned = new Matrix4();
  const clip = new Matrix4();
  const lastEye = new Vector3();
  let frames = 0;
  let current = 0;

  return {
    enabled: false,
    needsDepthTexture: true,
    setDepthTexture(texture: DepthTexture) { view.sceneDepth.value = texture; },
    setSize(width: number, height: number) {
      const w = Math.max(1, Math.round(width * MarchScale));
      const h = Math.max(1, Math.round(height * MarchScale));
      for (const each of [marched, ...memory]) each.setSize(w, h);
      past.marchTexel.value.set(1 / w, 1 / h);
      compose.uniforms.marchSize.value.set(w, h);
      frames = 0;
    },
    // Takes the camera's pose for this frame; `eye` is its world position.
    aim(camera: PerspectiveCamera, eye: Vector3) {
      const tangent = Math.tan(camera.fov * Math.PI / 360);
      camera.updateMatrixWorld();
      view.cloudCamera.value.copy(camera.matrixWorld);
      view.cloudLens.value.set(tangent * camera.aspect, tangent);
      past.eyeMoved.value.copy(eye).sub(lastEye);
      lastEye.copy(eye);
      // Next frame looks this frame's picture up through this frame's lens, turned but not moved.
      past.pastClip.value.copy(clip);
      turned.copy(camera.matrixWorldInverse).setPosition(0, 0, 0);
      clip.multiplyMatrices(camera.projectionMatrix, turned);
    },
    render(renderer: WebGLRenderer, writeBuffer: WebGLRenderTarget, readBuffer: WebGLRenderTarget) {
      view.jitter.value = (frames * 0.61803398875) % 1;
      past.persistence.value = frames === 0 ? 0 : Persistence;
      timer.begin();
      fullscreen(renderer, march, marched);
      picture.value = memory[current].texture;
      current = 1 - current;
      fullscreen(renderer, resolve, memory[current]);
      picture.value = memory[current].texture;
      compose.uniforms.sceneColor.value = readBuffer.texture;
      fullscreen(renderer, compose, writeBuffer);
      timer.end();
      frames++;
    },
    // Forgets the picture built so far, for a cut such as a teleport.
    forget() { frames = 0; },
    dispose() {
      for (const each of [marched, ...memory, march, resolve, compose]) each.dispose();
    },
  };
}

export type CloudPass = ReturnType<typeof createCloudPass>;
