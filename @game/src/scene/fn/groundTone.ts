import { Fn, If, clamp, dot, float, floor, fract, mix, smoothstep, vec3 } from "three/tsl";
import type { Node } from "three/webgpu";
import { GroundRepeat } from "../constant/grain";

type Vec3 = Node<"vec3">;
type Float = Node<"float">;

const wrap = (value: Vec3, repeat: Vec3) => value.sub(repeat.mul(floor(value.div(repeat))));

// A number 0…1 for a lattice cell.
export const groundHash = Fn(([at]: [Vec3]) => {
  const cell = fract(at.mul(0.1031)).toVar();
  cell.addAssign(dot(cell, cell.yzx.add(33.33)));
  return fract(cell.x.add(cell.y).mul(cell.z));
});

// Value noise in cells of one unit that repeats after `repeat` cells along each axis.
const groundNoise = Fn(([at, repeat]: [Vec3, Vec3]) => {
  const cell = floor(at);
  const part = fract(at).toVar();
  part.assign(part.mul(part).mul(part.mul(-2).add(3)));
  const corner = (x: number, y: number, z: number) => groundHash(wrap(cell.add(vec3(x, y, z)), repeat));
  const low = mix(mix(corner(0, 0, 0), corner(1, 0, 0), part.x), mix(corner(0, 1, 0), corner(1, 1, 0), part.x), part.y);
  const high = mix(mix(corner(0, 0, 1), corner(1, 0, 1), part.x), mix(corner(0, 1, 1), corner(1, 1, 1), part.x), part.y);
  return mix(low, high, part.z);
});

// What makes a stretch of one material look like ground instead of paint. None of it is simulated: the material
// and its colour come from the world, and this only varies that colour quietly in world space: stains over
// hundreds of metres, mottling over tens, grain over metres, and thin beds within one rock. Nothing here colours
// ground by its slope or its height.
//
// `at` is a position within one repeat of the pattern (render space plus the ground anchor), `loose` runs from
// solid rock at 0 to loose grains at 1, and `range` is the distance from the eye, past which variation finer than
// a pixel is left out. A cube takes the variation at its middle and stays one piece, so a pattern finer than
// `edge`, the cube it is drawn on, is left out as well: it would only be noise from cube to cube.
//
// The finer patterns are laid on axes turned off the cubes' by a 3-4-5 triangle, which keeps them from lining up
// with cube edges and still lets them survive the render origin's steps: the turn carries a step of GroundRepeat
// along x or z onto three and four fifths of it, so a turned octave repeats after a fifth of GroundRepeat.
//
// Call it inside a node function: an octave is only worked out where it shows.
export function groundColor(base: Vec3, at: Vec3, loose: Float, range: Float, edge: Float) {
  const turned = vec3(at.x.mul(3).add(at.z.mul(4)).div(5), at.y, at.z.mul(3).sub(at.x.mul(4)).div(5));
  // An octave in cells of `cell` metres, gone between `from` and `to` metres from the eye.
  const octave = (cell: number, offset: number, from: number, to: number) => {
    const value = float(0).toVar();
    const shown = smoothstep(from, to, range).oneMinus().mul(smoothstep(cell * 0.5, cell * 1.5, edge).oneMinus());
    If(shown.greaterThan(0), () => {
      value.assign(groundNoise(turned.div(cell).add(offset), vec3(GroundRepeat / 5 / cell, GroundRepeat / cell, GroundRepeat / 5 / cell))
        .sub(0.5).mul(shown));
    });
    return value;
  };
  const stain = groundNoise(at.div(256), vec3(GroundRepeat / 256)).sub(0.5).mul(smoothstep(128, 384, edge).oneMinus());
  const mottle = octave(25.6, 3.7, 5000, 16000);
  const patchy = octave(6.4, 9.1, 1200, 4000);
  const fine = octave(1.6, 5.3, 120, 400);
  // Noise drawn out sixteen metres along the ground and half a metre up; loose ground has none.
  const bedded = float(0).toVar();
  const beds = smoothstep(30, 250, range).oneMinus().mul(smoothstep(0.25, 0.75, edge).oneMinus()).mul(loose.oneMinus());
  If(beds.greaterThan(0), () => {
    bedded.assign(groundNoise(vec3(at.x.div(16), at.y.mul(2), at.z.div(16)), vec3(GroundRepeat / 16, GroundRepeat * 2, GroundRepeat / 16))
      .sub(0.5).mul(beds));
  });
  const tone = float(1).add(stain.mul(0.3)).add(mottle.mul(mix(0.26, 0.34, loose))).add(patchy.mul(0.3)).add(fine.mul(0.26))
    .add(bedded.mul(0.3));
  // Stains run warm where they darken and pale where they bleach, as weathered ground does.
  const hue = mix(vec3(1.05, 0.99, 0.92), vec3(0.97, 1, 1.04), clamp(stain.mul(1.6).add(mottle.mul(0.8)).add(0.5), 0, 1));
  return base.mul(tone).mul(hue);
}
