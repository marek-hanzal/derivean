import { Fn, clamp, float, floor, int, ivec2, max, mix, select } from "three/tsl";
import type { Node } from "three/webgpu";
import type { SnowNodes } from "../type/SnowNodes";

// kg/m² of snow on ground at a point in render space, `sea` being the sea's height there, read as the simulation
// reads it (packAt and snowAt in @simulation/src/atmosphere/fn/Snow.cpp): within a weather cell nothing below
// its snow line, linear from the line to the first rung above it and between the rungs further up, the highest
// rung held beyond the ladder; then bilinear between the four cells around the point. Nothing below the sea.
export function snowMass(snow: SnowNodes, position: Node<"vec3">, sea: Node<"float">): Node<"float"> {
  return Fn(() => {
    const last = snow.rungs - 1;
    const top = snow.heights.element(int(last));
    const above = clamp(position.y.sub(sea), 0, top);
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
      const line = read(int(snow.rungs));
      const upperPack = read(upper);
      const fromLine = select(high.greaterThan(line), upperPack.mul(above.sub(line)).div(max(high.sub(line), 1e-3)), upperPack);
      const between = mix(read(upper.sub(1)), upperPack, above.sub(low).div(high.sub(low)));
      return select(above.lessThan(line).or(line.greaterThanEqual(top)), select(above.greaterThanEqual(top), upperPack, float(0)),
        select(line.greaterThan(low), fromLine, between));
    };
    const alongX = x.sub(floor(x));
    const mass = mix(mix(pack(0, 0), pack(1, 0), alongX), mix(pack(0, 1), pack(1, 1), alongX), z.sub(floor(z)));
    return select(position.y.lessThan(sea), float(0), mass);
  })();
}
