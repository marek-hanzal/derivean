import { DepthTexture, DoubleSide, FloatType, MeshBasicNodeMaterial, ViewportDepthTextureNode } from "three/webgpu";
import {
  Fn, abs, cameraPosition, cameraProjectionMatrixInverse, cameraWorldMatrix, dot, exp, float, getViewPosition, length, max, min, mix,
  normalize, positionWorld, pow, screenUV, vec3, vec4,
} from "three/tsl";
import { cloudShadow } from "../../clouds/fn/cloudShadow";
import type { CloudShade } from "../../clouds/type/CloudShade";
import { faceNodes } from "../../cubes/fn/faceNodes";
import { levelClip, levelRows } from "../../cubes/fn/levelClip";
import type { LevelTable } from "../../cubes/type/LevelTable";
import { airFade, airLight } from "../../sky/fn/aerial";
import type { SkyNodes } from "../../sky/type/SkyNodes";

// Seas and lakes: the level sheets the engine sends with a level of cubes, drawn at the water's level whatever
// plane their faces carry. The depth of the bed under a pixel is read from the scene's depth, so tint and opacity
// grow with depth and the sheet dissolves into the shore where the cubes rise through it. Looked into it shows
// its depth, looked along it mirrors the sky, and far away the air takes it as it takes the ground. A sheet is
// drawn where its level of cubes is, also over the voxels, which do not hold this water.
export function sheetSurface(sky: SkyNodes, levels: LevelTable, clouds: CloudShade) {
  const material = new MeshBasicNodeMaterial({ transparent: true, depthWrite: false, side: DoubleSide, fog: false });
  const face = faceNodes();
  // The copy of the scene's depth the sheets read; a float like the depth buffer it is copied from.
  const sceneDepth = new DepthTexture(1, 1, FloatType);
  material.addEventListener("dispose", () => sceneDepth.dispose());
  material.positionNode = face.corner;
  material.colorNode = Fn(() => {
    // Sheets of two levels would blend twice where they overlapped, so a sheet ends at its square.
    levelClip(levelRows(levels, face).row, positionWorld, 4, 0);
    const fromEye = positionWorld.sub(cameraPosition);
    const range = length(fromEye);
    const bed = cameraWorldMatrix.mul(vec4(getViewPosition(screenUV, new ViewportDepthTextureNode(screenUV, null, sceneDepth), cameraProjectionMatrixInverse), 1));
    const depth = max(positionWorld.y.sub(bed.y), 0);
    const deepening = exp(depth.div(-14)).oneMinus();
    const toEye = normalize(fromEye.negate());
    const facing = abs(toEye.y);
    // Schlick's reflectance of water: a mirror of the sky at grazing angles, clear when looked into.
    const fresnel = min(pow(facing.oneMinus(), 5).mul(0.98).add(0.02), 0.85);
    const mirror = vec3(toEye.x.negate(), facing, toEye.z.negate());
    // The sun reaches the water as far as the clouds let it.
    const sun = sky.sunLight.mul(cloudShadow(clouds, positionWorld));
    const body = mix(vec3(0.07, 0.2, 0.2), vec3(0.008, 0.04, 0.075), deepening)
      .mul(sky.skyLight.mul(0.55).add(sun.mul(max(sky.skySun.y, 0)).mul(0.4)));
    const glint = sun.mul(pow(max(dot(mirror, sky.skySun), 0), 500)).mul(0.5);
    const natural = mix(body, airLight(sky, mirror, range), fresnel).add(glint);
    const relief = mix(vec3(0.2, 0.36, 0.52), vec3(0.07, 0.15, 0.3), deepening);
    // The shore needs no line drawn: the bed is cubes, and the sheet ends at the riser that rises through it.
    // Shallow water lets its bed show; tens of metres of it let nothing through.
    const cover = exp(depth.div(-0.9)).oneMinus().mul(mix(float(0.8), 1, max(max(fresnel, sky.plain), exp(depth.div(-25)).oneMinus())));
    return vec4(airFade(sky, mix(natural, relief, sky.plain), fromEye), cover);
  })();
  return material;
}
