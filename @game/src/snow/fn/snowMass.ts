import { Fn, If, clamp, float, floor, int, ivec2, max, mix, select } from "three/tsl";
import type { Node } from "three/webgpu";
import type { SnowNodes } from "../type/SnowNodes";

// kg/m² of snow on ground at a point in render space, `sea` being the sea's height there, read as the simulation
// reads it (packAt and snowAt in @simulation/src/atmosphere/fn/Snow.cpp): within a weather cell nothing below
// its snow line, linear from the line to the first rung above it and between the rungs further up, the end
// rungs held beyond the ladder; then bilinear between the four cells around the point. Nothing below the sea:
// the renderer's choice, as ground there lies under water.
export function snowMass(snow: SnowNodes, position: Node<"vec3">, sea: Node<"float">): Node<"float"> {
  return Fn(() => {
    const mass = float(0).toVar();
    // Nothing is read below the sea, and of a cell only what the point's height needs: its snow line, and the rungs
    // round the point where snow lies there.
    If(position.y.greaterThanEqual(sea), () => {
      const last = snow.rungs - 1;
      const top = snow.heights.element(int(last));
      const above = clamp(position.y.sub(sea), snow.heights.element(int(0)), top);
      // The first rung at or above the point, from the second rung on: the rungs' heights are the same in every cell.
      const upper = int(1).toVar();
      for (let rung = 1; rung < last; rung++) upper.addAssign(int(snow.heights.element(int(rung)).lessThan(above)));
      const low = snow.heights.element(upper.sub(1));
      const high = snow.heights.element(upper);
      const x = snow.place.x.add(position.x.div(snow.cell));
      const z = snow.place.y.add(position.z.div(snow.cell));
      const cells = int(snow.cells);
      const wrapped = (cell: Node<"int">) => cell.mod(cells).add(cells).mod(cells);
      const pack = (east: number, south: number) => {
        const column = wrapped(int(floor(x)).add(int(snow.place.z)).add(east)).mul(snow.rungs + 1);
        const row = wrapped(int(floor(z)).add(int(snow.place.w)).add(south));
        const read = (rung: Node<"int">) => snow.field.load(ivec2(column.add(rung), row)).r;
        const line = read(int(snow.rungs)).toVar();
        const lying = float(0).toVar();
        // Below the line, or a line over the ladder: bare, except at and over the top rung.
        const bare = above.lessThan(line).or(line.greaterThanEqual(top));
        If(bare.not().or(above.greaterThanEqual(top)), () => {
          const upperPack = read(upper).toVar();
          If(bare, () => {
            lying.assign(upperPack);
          }).ElseIf(line.greaterThan(low), () => {
            lying.assign(select(high.greaterThan(line), upperPack.mul(above.sub(line)).div(max(high.sub(line), 1e-3)), upperPack));
          }).Else(() => {
            lying.assign(mix(read(upper.sub(1)), upperPack, above.sub(low).div(high.sub(low))));
          });
        });
        return lying;
      };
      const alongX = x.sub(floor(x));
      mass.assign(mix(mix(pack(0, 0), pack(1, 0), alongX), mix(pack(0, 1), pack(1, 1), alongX), z.sub(floor(z))));
    });
    return mass;
  })();
}
