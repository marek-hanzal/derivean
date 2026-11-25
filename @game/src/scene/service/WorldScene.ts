import {
  ACESFilmicToneMapping, BoxGeometry, Color, DirectionalLight, EdgesGeometry, FogExp2,
  Group, HemisphereLight, LineBasicMaterial, LineSegments, Mesh, MeshBasicMaterial,
  PCFShadowMap, PerspectiveCamera, Scene, SphereGeometry, Vector3, WebGLRenderer,
} from "three";
import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import { sky, sunlight } from "../fn/sky";
import { voxelGeometry } from "../fn/voxelGeometry";
import { voxelSurface } from "../fn/voxelSurface";
import { createParticleScene } from "../../particles/service/ParticleScene";
import { createWaterScene } from "../../water/service/WaterScene";

export function createWorldScene(canvas: HTMLCanvasElement, world: WorldInfo) {
  const renderer = new WebGLRenderer({ canvas, antialias: true, powerPreference: "high-performance" });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5));
  renderer.toneMapping = ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = PCFShadowMap;

  const scene = new Scene();
  const horizon = new Color("#a7b8aa");
  const fog = new FogExp2(horizon, 0.078);
  scene.fog = fog;
  const atmosphere = sky(horizon);
  scene.add(atmosphere);
  scene.add(new HemisphereLight("#ecf1e9", "#8b6952", 1.48));

  const sun = new DirectionalLight("#ffdda8", 2.25);
  sun.castShadow = true;
  sun.shadow.mapSize.set(1536, 1536);
  sun.shadow.normalBias = 0.023;
  sun.shadow.bias = -0.00025;
  sun.shadow.camera.left = -19;
  sun.shadow.camera.right = 19;
  sun.shadow.camera.top = 19;
  sun.shadow.camera.bottom = -19;
  sun.shadow.camera.far = 55;
  scene.add(sun, sun.target);

  const camera = new PerspectiveCamera(62, 1, 0.055, 135);
  camera.rotation.order = "YXZ";
  const palette = world.materials.map(({ color }) => new Color(color));
  const surface = voxelSurface(world.voxelSize);
  const water = createWaterScene(scene, world, palette);
  const particles = createParticleScene(scene);
  const terrain = new Map<string, Mesh>();
  const pieces = new Map<number, Mesh>();

  const cube = new BoxGeometry(1, 1, 1);
  const lineMaterial = new LineBasicMaterial({ color: "#ffe5a5", transparent: true, opacity: 0.8, depthWrite: false });
  const wire = new LineSegments(new EdgesGeometry(cube), lineMaterial);
  const brushMaterial = new MeshBasicMaterial({
    color: "#ffdb88", transparent: true, opacity: 0.16, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2,
  });
  const cubeBrush = new Mesh(cube, brushMaterial);
  const sphereBrush = new Mesh(new SphereGeometry(0.5, 28, 16), brushMaterial);
  const indicator = new Group();
  indicator.add(wire, cubeBrush, sphereBrush);
  wire.scale.setScalar(world.voxelSize + 0.003);
  indicator.visible = false;
  scene.add(indicator);

  const shadowCenter = new Vector3();
  const eyeTarget = new Vector3();
  const menuTarget = new Vector3(-1.8, 0.9, -4.9);
  let initializedEye = false;
  let dirtyShadow = true;
  let width = 0;
  let height = 0;

  function mesh(faces: Int16Array) {
    const part = new Mesh(voxelGeometry(faces, world, palette), surface);
    part.castShadow = true;
    part.receiveShadow = true;
    scene.add(part);
    dirtyShadow = true;
    return part;
  }

  function remove(key: string) {
    const part = terrain.get(key);
    if (!part) return;
    scene.remove(part);
    part.geometry.dispose();
    terrain.delete(key);
    dirtyShadow = true;
  }

  function apply(frame: Frame) {
    water.apply(frame);
    particles.apply(frame.particles);
    eyeTarget.set(...frame.eye);
    for (const key of frame.unloaded) remove(key.join(","));
    for (const { key, origin, faces } of frame.chunks) {
      const id = key.join(",");
      remove(id);
      if (!faces.length) continue;
      const part = mesh(faces);
      part.position.set(...origin);
      terrain.set(id, part);
    }
    for (const { id, faces } of frame.spawned) pieces.set(id, mesh(faces));
    for (const id of frame.retired) {
      const piece = pieces.get(id);
      if (!piece) continue;
      scene.remove(piece);
      piece.geometry.dispose();
      pieces.delete(id);
      dirtyShadow = true;
    }
    for (const { id, position, rotation } of frame.debris) {
      const part = pieces.get(id);
      if (!part) continue;
      if (part.position.distanceToSquared(shadowCenter.set(...position)) > 0.000001 ||
          1 - Math.abs(part.quaternion.x * rotation[0] + part.quaternion.y * rotation[1]
            + part.quaternion.z * rotation[2] + part.quaternion.w * rotation[3]) > 0.000001) dirtyShadow = true;
      part.position.set(...position);
      part.quaternion.set(...rotation);
    }
  }

  function target(point: Point | undefined, shape: BrushShape, radius: number) {
    indicator.visible = point !== undefined;
    if (!point) return;
    indicator.position.set(...point).addScalar(0.5).multiplyScalar(world.voxelSize);
    cubeBrush.visible = shape === "cube";
    sphereBrush.visible = shape === "sphere";
    cubeBrush.scale.setScalar((2 * radius - 1) * world.voxelSize + 0.004);
    sphereBrush.scale.setScalar(2 * radius * world.voxelSize);
  }

  function draw(playing: boolean, visited: boolean, yaw: number, pitch: number, dt: number, time: number) {
    if (canvas.clientWidth !== width || canvas.clientHeight !== height) {
      width = canvas.clientWidth;
      height = canvas.clientHeight;
      renderer.setSize(width, height, false);
      camera.aspect = width / Math.max(1, height);
      camera.updateProjectionMatrix();
    }
    fog.density = playing || visited ? 0.078 : 0.052;
    if (playing || visited) {
      if (!initializedEye) {
        camera.position.copy(eyeTarget);
        initializedEye = true;
      }
      camera.position.lerp(eyeTarget, 1 - Math.exp(-32 * dt));
      camera.rotation.set(pitch, yaw, 0);
      shadowCenter.set(Math.round(camera.position.x / 2) * 2, 0, Math.round(camera.position.z / 2) * 2);
    } else {
      camera.position.set(9 + Math.sin(time * 0.13) * 0.7, 7.1, 6.3);
      camera.lookAt(menuTarget);
      shadowCenter.set(0, 0, -6);
    }
    if (sun.target.position.distanceToSquared(shadowCenter) > 0) {
      sun.target.position.copy(shadowCenter);
      sun.position.copy(shadowCenter).add(sunlight);
      dirtyShadow = true;
    }
    atmosphere.position.copy(camera.position);
    renderer.shadowMap.autoUpdate = false;
    renderer.shadowMap.needsUpdate = dirtyShadow;
    renderer.render(scene, camera);
    dirtyShadow = false;
  }

  return {
    apply,
    target,
    draw,
    renderStats: renderer.info.render,
  };
}
