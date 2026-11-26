import { type Camera, InstancedBufferAttribute, InstancedBufferGeometry, Mesh, PlaneGeometry, type Scene, type Vector3 } from "three/webgpu";
import type { Frame } from "../../../contract/GameBridge";
import type { SkyNodes } from "../../sky/type/SkyNodes";
import { FallBox, Rain, Shelter, Snow, type Fall } from "../constant/fall";
import { fallMaterial } from "../fn/fallMaterial";

const fract = (value: number) => value - Math.floor(value);

// Rain and snow around the eye, as many drops as the simulated rates ask for, slanted by the simulated wind. The
// scene's depth hides drops behind ground, but nothing here knows a roof: under an overhang it still falls.
export function createPrecipitation(scene: Scene, camera: Camera, sky: SkyNodes) {
  const quad = new PlaneGeometry(1, 1).translate(0, 0.5, 0);
  const most = Math.max(Rain.drops, Snow.drops);
  const seeds = new InstancedBufferAttribute(Float32Array.from({ length: most * 3 }, () => Math.random()), 3);

  function falling(fall: Fall) {
    const geometry = new InstancedBufferGeometry();
    geometry.index = quad.index;
    geometry.setAttribute("position", quad.getAttribute("position"));
    const look = fallMaterial(fall, sky, seeds);
    const mesh = new Mesh(geometry, look.material);
    mesh.frustumCulled = false;
    mesh.renderOrder = 2;
    mesh.visible = false;
    scene.add(mesh);
    return { fall, mesh, geometry, look, travelled: [0, 0, 0] };
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
      for (const { fall, mesh, geometry, look, travelled } of kinds) {
        const kind = fall === Rain ? 0 : 1;
        const count = shown && !sheltered ? Math.round(fall.drops * Math.min(1, Math.sqrt(rates[kind] / fall.heaviest))) : 0;
        mesh.visible = count > 0;
        geometry.instanceCount = count;
        mesh.position.copy(camera.position);
        mesh.updateMatrix();
        const velocity = look.velocity.value.set(wind[0], -fall.speed, wind[1]);
        travelled[0] = fract(travelled[0] + velocity.x * dt / FallBox);
        travelled[1] = fract(travelled[1] + velocity.y * dt / FallBox);
        travelled[2] = fract(travelled[2] + velocity.z * dt / FallBox);
        look.fallen.value.set(travelled[0], travelled[1], travelled[2]);
        look.eyeInBox.value.set(fract(eye.x / FallBox), fract(eye.y / FallBox), fract(eye.z / FallBox));
        look.seconds.value = seconds % 3600;
      }
    },
    dispose() {
      for (const { mesh, geometry, look } of kinds) {
        scene.remove(mesh);
        geometry.dispose();
        look.material.dispose();
      }
      quad.dispose();
    },
  };
}
