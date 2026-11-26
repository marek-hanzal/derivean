import { MeshBasicNodeMaterial, type Texture, type UniformNode, type Vector3 } from "three/webgpu";
import {
  Fn, If, PI, cameraPosition, clamp, dot, float, floor, fract, fwidth, int, ivec2, length, max, min, mix, positionWorld,
  smoothstep, step, textureLoad, uint, vec3, vec4,
} from "three/tsl";
import { GrainCells, GroundRepeat } from "../../scene/constant/grain";
import { groundColor, groundHash } from "../../scene/fn/groundTone";
import { airFade } from "../../sky/fn/aerial";
import type { SkyNodes } from "../../sky/type/SkyNodes";
import type { LevelTable } from "../type/LevelTable";
import { faceNodes } from "./faceNodes";
import { levelClip, levelRows } from "./levelClip";

export interface CubeLook {
  sky: SkyNodes;
  levels: LevelTable;
  // The world's materials by id: colour, and in alpha whether the material is loose grains (groundPalette).
  palette: Texture;
  // The render origin within one repeat of the ground's variation.
  anchor: UniformNode<"vec3", Vector3>;
}

// How much of the sky's light a corner keeps with one, two and three of its neighbours solid.
const Open = 0.82;

// The one look of ground at every cube size, voxels and far levels alike: the material's colour varied as ground
// is, each cube a shade of its own with a seam round it and specks within, lit flat by sky and sun with its
// corners darkened by the neighbours the engine counted there, and faded by the air. Nothing is smoothed: a face
// is lit by its own normal.
export function cubeSurface({ sky, levels, palette, anchor }: CubeLook) {
  const material = new MeshBasicNodeMaterial({ fog: false });
  const face = faceNodes();
  material.positionNode = face.corner;
  material.colorNode = Fn(() => {
    const { row, edge } = levelRows(levels, face);
    const normal = face.normal.normalize();
    const inside = positionWorld.sub(normal.mul(edge.mul(0.01)));
    levelClip(row, inside, 2, row(1).w);

    const fromEye = positionWorld.sub(cameraPosition);
    const range = length(fromEye);
    const texel = textureLoad(palette, ivec2(int(face.words.x.shiftRight(uint(16)).bitAnd(uint(255))), 0));
    const loose = texel.a;
    // Cubes of every level lie on one lattice from the world's origin, which the anchor carries into render space.
    // A cube's grain repeats after `repeat` cubes, so only the anchor's place within one repeat is added before
    // the division: the whole anchor would cost a voxel's cell its last bits.
    const repeat = min(float(GrainCells), float(GroundRepeat).div(edge));
    const near = anchor.sub(repeat.mul(edge).mul(floor(anchor.div(repeat.mul(edge)))));
    const cell = inside.add(near).div(edge);
    const pixel = max(max(fwidth(cell.x), fwidth(cell.y)), fwidth(cell.z));
    const cube = floor(cell);
    const wrapped = (cells: typeof cube, scale: number) => cells.sub(repeat.mul(scale).mul(floor(cells.div(repeat.mul(scale)))));
    // A cube is one piece of ground: it takes the ground's variation at its middle, a tone of its own, and a
    // faint seam round its faces. What it is made of shows within it as specks a quarter and a twelfth of it
    // across, with the odd pale stone and dark pit among them: plain in loose grains, faint on a face of rock.
    // Each of these is gone before it is smaller than a pixel.
    const tone = float(1).toVar();
    If(pixel.lessThan(0.57), () => {
      const within = min(fract(cell), fract(cell).oneMinus()).add(normal.abs());
      const seam = smoothstep(0.015, 0.09, min(min(within.x, within.y), within.z));
      tone.addAssign(smoothstep(0.14, 0.57, pixel).oneMinus()
        .mul(groundHash(wrapped(cube, 1)).sub(0.5).mul(mix(0.08, 0.14, loose)).sub(seam.oneMinus().mul(0.04))));
    });
    If(pixel.lessThan(0.12), () => {
      const coarse = groundHash(wrapped(floor(cell.mul(4)), 4));
      const fines = groundHash(wrapped(floor(cell.mul(12)).add(7), 12));
      tone.addAssign(coarse.sub(0.5).mul(0.6).add(step(0.94, coarse).mul(0.5)).sub(step(coarse, 0.07).mul(0.5))
        .mul(smoothstep(0.04, 0.12, pixel).oneMinus()).add(fines.sub(0.5).mul(smoothstep(0.015, 0.04, pixel).oneMinus()))
        .mul(mix(0.07, 0.2, loose)));
    });
    const albedo = mix(groundColor(texel.rgb, cube.add(0.5).mul(edge).add(anchor.sub(near)), loose, range, edge).mul(tone), vec3(0.6), sky.plain);

    // Corner occlusion, two bits a corner, blended across the quad: exact, because a quad is only stretched
    // along an axis its occlusion does not change on.
    const corner = (index: number) => float(face.words.x.shiftRight(uint(24 + index * 2)).bitAnd(uint(3)));
    const across = clamp(face.across, 0, 1);
    const open = float(Open).pow(mix(mix(corner(0), corner(1), across.x), mix(corner(2), corner(3), across.x), across.y));
    const light = mix(sky.groundLight, sky.skyLight, normal.y.mul(0.5).add(0.5)).mul(open)
      .add(sky.sunLight.mul(max(dot(normal, sky.sunDirection), 0)).mul(mix(1, open, 0.5)));
    return vec4(airFade(sky, albedo.mul(light).div(PI), fromEye), 1);
  })();
  return material;
}
