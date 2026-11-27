import { MeshBasicNodeMaterial, type Node, type Texture, type UniformNode, type Vector3 } from "three/webgpu";
import {
  Fn, If, PI, cameraPosition, clamp, float, floor, fract, fwidth, int, ivec2, length, luminance, max, min, mix, modelWorldMatrix, positionWorld, smoothstep,
  select, step, textureLoad, uint, varying, vec3, vec4,
} from "three/tsl";
import { GrainCells, GroundRepeat } from "../../scene/constant/grain";
import { groundColor, groundHash } from "../../scene/fn/groundTone";
import { sunlit } from "../../shadow/fn/sunlit";
import type { ShadowNodes } from "../../shadow/type/ShadowNodes";
import { GroundBounce } from "../../sky/constant/air";
import { airFade } from "../../sky/fn/aerial";
import { faceLight } from "../../sky/fn/faceLight";
import type { SkyNodes } from "../../sky/type/SkyNodes";
import { CoverMass } from "../../snow/constant/cover";
import { snowMass } from "../../snow/fn/snowMass";
import type { SnowNodes } from "../../snow/type/SnowNodes";
import { SnowHolds } from "../constant/land";
import { Face } from "../constant/layout";
import type { LevelHeights } from "../service/LevelHeights";
import type { LevelTable } from "../type/LevelTable";
import { faceNodes } from "./faceNodes";
import { landNormal } from "./landNormal";
import { levelClip, levelRows } from "./levelClip";

export interface CubeLook {
  sky: SkyNodes;
  levels: LevelTable;
  // The world's materials by id: colour, and in alpha whether the material is loose grains (groundPalette).
  palette: Texture;
  // The render origin within one repeat of the ground's variation.
  anchor: UniformNode<"vec3", Vector3>;
  shadow: ShadowNodes;
  // Absent in a world that has no snow to draw.
  snow?: SnowNodes;
  // The far levels' ground heights (LevelHeights). Without them faces take only their own light and risers no snow.
  land?: LevelHeights["nodes"];
}

// How much of the sky's light a corner keeps with one, two and three of its neighbours solid.
const Open = 0.82;
// Metres of a cube's edge up to which thin snow covers a cube whole or not at all, and from which its top takes
// the snow's share of its colour.
const SnowWhole = 0.5;
const SnowMean = 1;

// The one look of ground at every cube size, voxels and far levels alike: the material's colour varied as ground
// is, each cube a shade of its own with a seam round it and specks within, lit flat by sky and sun with its
// corners darkened by the neighbours the engine counted there, shadowed by the cubes between it and the sun, and
// faded by the air. Nothing is smoothed: a face is lit by its own normal and, on a coarse cube, partly by the slope
// of the land the cube stands for (landNormal), one light for the whole face. The simulation's snow lies on the tops
// of the cubes, and on the risers of ground gentle enough for it to lie on (landNormal). Where it is thin, a small
// cube is whole or bare, and the share of such cubes it covers is its share of the ground; the top of a cube a
// metre or more across stands for that much ground, so it takes the mean of it, the snow's share of its colour.
// That is what the small cubes average to, and it does not scatter the far view with white flecks.
export function cubeSurface({ sky, levels, palette, anchor, shadow, snow, land }: CubeLook) {
  const material = new MeshBasicNodeMaterial({ fog: false });
  const face = faceNodes();
  material.positionNode = face.corner;
  // Snow changes little across a quad, so it is read at the corners of the tops.
  const snowed = snow && varying(Fn(() => {
    const mass = float(0).toVar();
    If(face.top, () => { mass.assign(snowMass(snow, positionWorld, sky.airSea)); });
    return mass;
  })(), "vSnow");
  // The snow round a face, one value for the whole face, read at its middle: what the ground in front of a riser
  // sends back.
  const around = snow && varying(snowMass(snow, modelWorldMatrix.mul(vec4(face.middle, 1)).xyz, sky.airSea), "vSnowAround");
  material.colorNode = Fn(() => {
    const { row, edge } = levelRows(levels, face);
    const normal = face.normal.normalize();
    const inside = positionWorld.sub(normal.mul(edge.mul(0.01)));
    levelClip(row, inside, 2, row(1).w);

    const fromEye = positionWorld.sub(cameraPosition);
    const range = length(fromEye);
    const texel = textureLoad(palette, ivec2(int(face.field(Face.material)), 0));
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
    const own = groundHash(wrapped(cube, 1));
    // A cube is one piece of ground: it takes the ground's variation at its middle, a tone of its own, and a
    // faint seam round its faces. What it is made of shows within it as specks a quarter and a twelfth of it
    // across, with the odd pale stone and dark pit among them: plain in loose grains, faint on a face of rock.
    // Each of these is gone before it is smaller than a pixel.
    const tone = float(1).toVar();
    If(pixel.lessThan(0.57), () => {
      const within = min(fract(cell), fract(cell).oneMinus()).add(normal.abs());
      const seam = smoothstep(0.015, 0.09, min(min(within.x, within.y), within.z));
      tone.addAssign(smoothstep(0.14, 0.57, pixel).oneMinus().mul(own.sub(0.5).mul(mix(0.08, 0.14, loose)).sub(seam.oneMinus().mul(0.04))));
    });
    If(pixel.lessThan(0.12), () => {
      const coarse = groundHash(wrapped(floor(cell.mul(4)), 4));
      const fines = groundHash(wrapped(floor(cell.mul(12)).add(7), 12));
      tone.addAssign(coarse.sub(0.5).mul(0.6).add(step(0.94, coarse).mul(0.5)).sub(step(coarse, 0.07).mul(0.5))
        .mul(smoothstep(0.04, 0.12, pixel).oneMinus()).add(fines.sub(0.5).mul(smoothstep(0.015, 0.04, pixel).oneMinus()))
        .mul(mix(0.07, 0.2, loose)));
    });
    const bare = groundColor(texel.rgb, cube.add(0.5).mul(edge).add(anchor.sub(near)), loose, range, edge).mul(tone);
    const lit = sunlit(shadow, positionWorld.add(normal.mul(edge.mul(0.02))));
    // A riser on ground the snow lies on is the stair the lattice cuts into a covered slope, so it takes the snow
    // round it as a top does; on ground steeper than snow holds on it stays bare rock.
    const riser = normal.y.abs().lessThan(0.5);
    const stands = land && landNormal(land, levels, face.field(Face.level), edge, inside,
      (point) => sunlit(shadow, point), around && riser.and(around.greaterThan(0)));
    const onRiser = stands && around ? select(riser, clamp(around.div(CoverMass), 0, 1), float(0))
      .mul(smoothstep(SnowHolds[0], SnowHolds[1], stands.steep).oneMinus()) : float(0);
    const share = snowed ? max(clamp(snowed.div(CoverMass), 0, 1), onRiser) : float(0);
    // The cube's own number, turned so that it is not the one its tone was drawn from.
    const white = snow && snowed ? mix(step(fract(own.mul(7.31).add(0.37)), share.sub(1e-4)), share, smoothstep(SnowWhole, SnowMean, edge)) : float(0);
    const ground = snow ? mix(bare, vec3(tone.mul(0.25).add(0.75)).mul(snow.color), white) : bare;
    const albedo = mix(ground, vec3(0.6), sky.plain);

    // Corner occlusion, two bits a corner, blended across the quad: exact, because a quad is only stretched
    // along an axis its occlusion does not change on.
    const corner = (index: number) => float(face.field(Face.occlusion).shiftRight(uint(index * 2)).bitAnd(uint(3)));
    const across = clamp(face.across, 0, 1);
    const open = float(Open).pow(mix(mix(corner(0), corner(1), across.x), mix(corner(2), corner(3), across.x), across.y));
    // Snow sends back far more of the light than bare ground: the bounce takes its share of the snow's colour.
    // Plain shading keeps its neutral bounce.
    const fallen = luminance(sky.groundLight).div(GroundBounce);
    const below = snow && around ? mix(sky.groundLight, vec3(fallen).mul(snow.color), clamp(around.div(CoverMass), 0, 1).mul(sky.plain.oneMinus())) : undefined;
    const itself = faceLight(sky, normal, open, lit, below);
    const light = stands ? mix(itself, faceLight(sky, stands.normal, open, stands.lit, below), stands.share) : itself;
    return vec4(airFade(sky, albedo.mul(light).div(PI), fromEye), 1);
  })();
  return material;
}
