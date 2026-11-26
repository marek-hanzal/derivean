import { ForwardShare, ForwardThinning, LeastThrough } from "../constant/shadow";

// Share of direct sunlight that comes out below cloud of optical depth `depth` along the beam: the straight beam dies
// off as Beer and Lambert say; light scattered forward keeps thin cloud bright, and some always comes out of the
// base of the thickest.
export const sunThrough = (depth: number) =>
  LeastThrough + (1 - LeastThrough) * ((1 - ForwardShare) * Math.exp(-depth) + ForwardShare * Math.exp(-depth * ForwardThinning));
