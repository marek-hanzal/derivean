import {
  DepthTexture, FloatType, HalfFloatType, LinearFilter, Matrix4, type Node, NodeMaterial, type PerspectiveCamera, QuadMesh, RenderTarget,
  type Scene, Vector2, Vector3, type WebGPURenderer,
} from "three/webgpu";
import {
  Fn, abs, float, floor, fract, hash, log2, max, min, mix, perspectiveDepthToViewZ, sampler, screenCoordinate, select, texture, uint,
  uniform, uv, vec2, vec3, vec4,
} from "three/tsl";
import type { SkyNodes } from "../../sky/type/SkyNodes";
import { HazeDepth, MarchScale, Persistence } from "../constant/march";
import { cloudArgs } from "../fn/cloudArgs";
import { marchNode } from "../fn/marchNode";
import { resolveNode } from "../fn/resolveNode";
import type { CloudNodes } from "../type/CloudNodes";

function picture() {
  const made = new RenderTarget(1, 1, { type: HalfFloatType, depthBuffer: false });
  made.texture.minFilter = made.texture.magFilter = LinearFilter;
  return made;
}

// The renderer's output chain. The scene is drawn into a multisampled half-float target whose colour and depth the
// clouds read; the clouds are marched at reduced size, built up over frames, and laid over the scene by depth on
// the way to the screen, where tone mapping happens once. Clouds draw after the whole scene, so no far plane can
// clip one.
export function createCloudPass(clouds: CloudNodes, sky: SkyNodes, viewDistance: number) {
  const depth = new DepthTexture(1, 1, FloatType);
  const scene = new RenderTarget(1, 1, { type: HalfFloatType, samples: 4, depthTexture: depth });
  const marched = picture();
  const memory = [picture(), picture()];
  const near = uniform(0.05);
  const far = uniform(1000);
  const turn = uniform(new Matrix4());
  const lens = uniform(new Vector2(1, 1));
  const jitter = uniform(0);
  const pastClip = uniform(new Matrix4());
  const eyeMoved = uniform(new Vector3());
  const texel = uniform(new Vector2(1, 1));
  const persistence = uniform(0);
  const marchSize = uniform(new Vector2(1, 1));
  const shown = uniform(0);
  const marchedNode = texture(marched.texture);
  const rememberedNode = texture(memory[0].texture);
  const shownNode = texture(memory[1].texture);

  // Texture coordinates run down the screen; a ray's direction in render space.
  const viewRay = (at: Node<"vec2">) => vec3(at.x.mul(2).sub(1).mul(lens.x), at.y.mul(-2).add(1).mul(lens.y), -1);
  const direction = (at: Node<"vec2">) => turn.mul(vec4(viewRay(at).normalize(), 0)).xyz;
  // Metres from the eye to the scene along the view axis; the far plane where nothing was drawn.
  const ahead = (at: Node<"vec2">) => {
    const stored = texture(depth, at).x;
    return select(stored.lessThanEqual(0), far, perspectiveDepthToViewZ(stored, near, far).negate());
  };

  const march = new NodeMaterial();
  march.fragmentNode = Fn(() => {
    const at = uv();
    const ray = viewRay(at);
    const limit = min(float(viewDistance * 1.6), ahead(at).mul(ray.length()));
    // Each pixel starts the frames' golden-ratio sequence at a number of its own, white across the screen: a frame
    // that is not averaged away (while turning) then shows grain with no direction. A gradient pattern over the
    // screen drew a slanted hatch there.
    const dither = fract(hash(uint(screenCoordinate.x).add(uint(screenCoordinate.y).mul(uint(65521)))).add(jitter));
    return marchNode({
      direction: direction(at), limit, dither, eye: clouds.eye, sun: sky.skySun, sunLight: sky.sunDisc, skyAmbient: sky.clearSky,
      groundAmbient: sky.groundLight, horizon: sky.skyHorizon, haze: float(HazeDepth / viewDistance), ...cloudArgs(clouds),
    });
  })();
  const resolve = new NodeMaterial();
  resolve.fragmentNode = resolveNode({
    at: uv(), direction: direction(uv()), marched: marchedNode, remembered: rememberedNode, linear: sampler(marchedNode), pastClip,
    eyeMoved, eye: clouds.eye, datum: clouds.datum, range0: clouds.ranges[0], range1: clouds.ranges[1], range2: clouds.ranges[2],
    texel, persistence,
  });
  // Each pixel weighs the four cloud texels around it by how close their scene depth is to its own, so a ridge in
  // front of a cloud keeps a clean edge.
  const compose = new NodeMaterial();
  compose.fragmentNode = Fn(() => {
    const at = uv();
    const color = texture(scene.texture, at);
    const spread = log2(far);
    const own = log2(ahead(at)).div(spread);
    const place = at.mul(marchSize).sub(0.5);
    const corner = floor(place);
    const part = place.sub(corner);
    const cloud = vec4(0).toVar();
    const total = float(0).toVar();
    for (const [x, y] of [[0, 0], [1, 0], [0, 1], [1, 1]]) {
      const offset = vec2(x, y);
      const there = corner.add(offset).add(0.5).div(marchSize);
      const share = mix(part.oneMinus(), part, offset);
      const weight = share.x.mul(share.y).div(abs(log2(ahead(there)).div(spread).sub(own)).add(0.0005));
      cloud.addAssign(shownNode.sample(there).level(float(0)).mul(weight));
      total.addAssign(weight);
    }
    const over = mix(vec4(0, 0, 0, 1), cloud.div(max(total, 1e-6)), shown);
    return vec4(color.rgb.mul(over.a).add(over.rgb), 1);
  })();
  const quad = new QuadMesh(march);
  const size = new Vector2();
  const turned = new Matrix4();
  const clip = new Matrix4();
  const lastEye = new Vector3();
  let frames = 0;
  let current = 0;

  function draw(renderer: WebGPURenderer, material: NodeMaterial, target: RenderTarget | null) {
    quad.material = material;
    renderer.setRenderTarget(target);
    quad.render(renderer);
  }

  return {
    // Draws the scene into the pass's own target; the caller reads the renderer's counts straight after.
    scene(renderer: WebGPURenderer, view: Scene, camera: PerspectiveCamera) {
      renderer.getDrawingBufferSize(size);
      if (scene.width !== size.x || scene.height !== size.y) {
        scene.setSize(size.x, size.y);
        const w = Math.max(1, Math.round(size.x * MarchScale));
        const h = Math.max(1, Math.round(size.y * MarchScale));
        for (const each of [marched, ...memory]) each.setSize(w, h);
        texel.value.set(1 / w, 1 / h);
        marchSize.value.set(w, h);
        frames = 0;
      }
      renderer.setRenderTarget(scene);
      renderer.render(view, camera);
    },
    // Marches, resolves and lays the clouds over the scene onto the screen; with `cloudy` false only the scene
    // goes there. `eye` is the camera's world position.
    finish(renderer: WebGPURenderer, camera: PerspectiveCamera, eye: Vector3, cloudy: boolean) {
      near.value = camera.near;
      far.value = camera.far;
      shown.value = Number(cloudy);
      if (cloudy) {
        const tangent = Math.tan(camera.fov * Math.PI / 360);
        camera.updateMatrixWorld();
        turn.value.copy(camera.matrixWorld);
        lens.value.set(tangent * camera.aspect, tangent);
        eyeMoved.value.copy(eye).sub(lastEye);
        lastEye.copy(eye);
        jitter.value = (frames * 0.61803398875) % 1;
        persistence.value = frames === 0 ? 0 : Persistence;
        // This frame looks the last frame's picture up through the last frame's lens, turned but not moved.
        turned.copy(camera.matrixWorldInverse).setPosition(0, 0, 0);
        const lensNow = turned.premultiply(camera.projectionMatrix);
        pastClip.value.copy(frames === 0 ? lensNow : clip);
        clip.copy(lensNow);
        draw(renderer, march, marched);
        rememberedNode.value = memory[current].texture;
        current = 1 - current;
        draw(renderer, resolve, memory[current]);
        shownNode.value = memory[current].texture;
        frames++;
      } else {
        frames = 0;
      }
      draw(renderer, compose, null);
    },
    // Forgets the picture built so far, for a cut such as a teleport.
    forget() { frames = 0; },
    dispose() {
      for (const each of [scene, marched, ...memory, march, resolve, compose, depth]) each.dispose();
    },
  };
}

export type CloudPass = ReturnType<typeof createCloudPass>;
