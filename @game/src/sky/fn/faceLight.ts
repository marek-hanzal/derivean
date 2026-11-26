import { dot, max, mix } from "three/tsl";
import type { Node } from "three/webgpu";
import type { SkyNodes } from "../type/SkyNodes";

type Float = Node<"float"> | number;

// The light a flat face takes: between the sky above and the ground below by how far it faces up, less what its
// corners hide of them (`open`, 1 for none), and the sun where the face looks at it, by the share of it that
// reaches the face (`sunlit`, 0 in shadow). A face in shadow is still lit by sky and ground. `ground` is the light
// the ground below sends back, if not that of bare level ground (`sky.groundLight`).
export function faceLight(sky: SkyNodes, normal: Node<"vec3">, open: Float, sunlit: Float, ground?: Node<"vec3">) {
  return mix(ground ?? sky.groundLight, sky.skyLight, normal.y.mul(0.5).add(0.5)).mul(open)
    .add(sky.sunLight.mul(max(dot(normal, sky.sunDirection), 0)).mul(sunlit).mul(mix(1, open, 0.5)));
}
