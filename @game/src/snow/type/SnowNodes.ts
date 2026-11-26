import type { Color, TextureNode, UniformArrayNode, UniformNode, Vector4 } from "three/webgpu";

// The simulation's snow as a surface reads it (CloudField.snowLadder and snowLine).
export interface SnowNodes {
  // One row of texels per row of weather cells; a cell takes `rungs + 1` texels along the row: the snow in
  // kg/m² on each rung of the ladder, then the cell's snow line in metres above the sea.
  field: TextureNode;
  rungs: number;
  // Metres above the sea of the rungs.
  heights: UniformArrayNode<"float">;
  // Where the render origin lies on the grid of cell centres: x and y the fraction of a cell along x and z, z
  // and w the whole cells. Then metres along a cell and cells along the grid.
  place: UniformNode<"vec4", Vector4>;
  cell: UniformNode<"float", number>;
  cells: UniformNode<"float", number>;
  // The colour of snow, linear.
  color: UniformNode<"color", Color>;
}
