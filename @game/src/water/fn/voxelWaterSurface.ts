import { type Color, DoubleSide, MeshBasicNodeMaterial } from "three/webgpu";
import { Fn, PI, cameraPosition, dot, max, mix, positionWorld, vec3, vec4 } from "three/tsl";
import { faceNodes } from "../../cubes/fn/faceNodes";
import { airFade } from "../../sky/fn/aerial";
import type { SkyNodes } from "../../sky/type/SkyNodes";

// Voxel water: the finite water of the played world, meshed by the engine in its own units. One translucent
// colour, lit as the cubes are.
export function voxelWaterSurface(sky: SkyNodes, color: Color) {
  const material = new MeshBasicNodeMaterial({ transparent: true, depthWrite: false, side: DoubleSide, fog: false });
  const face = faceNodes();
  material.positionNode = face.corner;
  material.colorNode = Fn(() => {
    const normal = face.normal.normalize();
    const light = mix(sky.groundLight, sky.skyLight, normal.y.mul(0.5).add(0.5)).add(sky.sunLight.mul(max(dot(normal, sky.sunDirection), 0)));
    const lit = vec3(color.r, color.g, color.b).mul(light).div(PI).add(vec3(0.003, 0.022, 0.04));
    return vec4(airFade(sky, lit, positionWorld.sub(cameraPosition)), 0.76);
  })();
  return material;
}
