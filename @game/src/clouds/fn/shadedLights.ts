import { ShaderChunk } from "three";

const lit = "getDirectionalLightInfo( directionalLight, directLight );";

// three.js's light loop with the sun dimmed by \`sunShare\`, a float the shader using it must have set by then.
// Only direct sunlight is dimmed: the sky still lights the ground under a cloud.
export function shadedLights() {
  const chunk = ShaderChunk.lights_fragment_begin;
  if (!chunk.includes(lit)) throw new Error("three.js light loop changed; revisit the cloud shadows");
  return chunk.replace(lit, `${lit}\n directLight.color *= sunShare;`);
}
