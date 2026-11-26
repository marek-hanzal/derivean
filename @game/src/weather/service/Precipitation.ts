import {
  type Camera, InstancedBufferAttribute, InstancedBufferGeometry, Mesh, PlaneGeometry, type Scene, type ShaderMaterial,
  type Vector3,
} from "three";
import type { Frame } from "../../../contract/GameBridge";
import type { SkyLight } from "../../sky/type/SkyLight";
import { FallBox, Rain, Shelter, Snow, type Fall } from "../constant/fall";
import { fallMaterial } from "../fn/fallMaterial";

const fract = (value: number) => value - Math.floor(value);

// Rain and snow around the eye, as many drops as the simulated rates ask for, slanted by the simulated wind. The
// scene's depth hides drops behind ground, but nothing here knows a roof: under an overhang it still falls.
export function createPrecipitation(scene: Scene, camera: Camera, light: SkyLight) {
  const quad = new PlaneGeometry(1, 1).translate(0, 0.5, 0);
  const most = Math.max(Rain.drops, Snow.drops);
  const seeds = new Float32Array(most * 3);
  for (let i = 0; i < seeds.length; i++) seeds[i] = Math.random();
  const geometry = new InstancedBufferGeometry();
  geometry.index = quad.index;
  geometry.setAttribute("position", quad.getAttribute("position"));
  geometry.setAttribute("seed", new InstancedBufferAttribute(seeds, 3));

  function falling(fall: Fall) {
    const mesh = new Mesh(geometry.clone(), fallMaterial(fall, light));
    mesh.frustumCulled = false;
    mesh.renderOrder = 2;
    mesh.visible = false;
    scene.add(mesh);
    return { fall, mesh, material: mesh.material as ShaderMaterial, travelled: [0, 0, 0] };
  }
  const kinds = [falling(Rain), falling(Snow)];
  let rates = [0, 0];
  let wind: [number, number] = [0, 0];
  let sheltered = false;
  let last: number | undefined;

  return {
    apply(frame: Frame) {
      rates = [frame.weather.rain, frame.weather.snow];
      wind = frame.weather.wind;
      sheltered = frame.ground !== null && frame.eye[1] < frame.ground - Shelter;
    },
    // `eye` is the eye in world metres, `seconds` real time: drops fall at their own speed whatever the clock.
    update(eye: Vector3, seconds: number, shown: boolean) {
      const dt = last === undefined ? 0 : Math.min(0.1, seconds - last);
      last = seconds;
      kinds.forEach(({ fall, mesh, material, travelled }, kind) => {
        const count = shown && !sheltered ? Math.round(fall.drops * Math.min(1, Math.sqrt(rates[kind] / fall.heaviest))) : 0;
        mesh.visible = count > 0;
        (mesh.geometry as InstancedBufferGeometry).instanceCount = count;
        mesh.position.copy(camera.position);
        const velocity = material.uniforms.velocity.value.set(wind[0], -fall.speed, wind[1]);
        travelled[0] = fract(travelled[0] + velocity.x * dt / FallBox);
        travelled[1] = fract(travelled[1] + velocity.y * dt / FallBox);
        travelled[2] = fract(travelled[2] + velocity.z * dt / FallBox);
        material.uniforms.fallen.value.set(travelled[0], travelled[1], travelled[2]);
        material.uniforms.eyeInBox.value.set(fract(eye.x / FallBox), fract(eye.y / FallBox), fract(eye.z / FallBox));
        material.uniforms.seconds.value = seconds % 3600;
      });
    },
    dispose() {
      for (const { mesh, material } of kinds) {
        scene.remove(mesh);
        mesh.geometry.dispose();
        material.dispose();
      }
      geometry.dispose();
      quad.dispose();
    },
  };
}
