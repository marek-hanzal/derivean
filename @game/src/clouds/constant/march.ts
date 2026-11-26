// The march's budget. Steps lengthen with distance, so a ray spends its samples where a pixel can show them.
export const MarchSteps = 56;
// Shortest step in metres through each band, low to high.
export const StepLengths = [70, 110, 160] as const;
// Share of the distance travelled a step is at least.
export const StepGrowth = 0.045;
// How much longer a step through clear air is.
export const ClearStride = 1.7;
// Metres within which wisps are drawn.
export const WispReach = 30000;
// First of the five growing steps towards the sun, per band.
export const SunSteps = [70, 110, 170] as const;
// Share of the screen's width and height the march runs at.
export const MarchScale = 0.5;
// Share of the accumulated picture a frame keeps where it can be found again.
export const Persistence = 0.94;
// The clouds' own fade with distance, squared-exponential: this over the view distance is its density.
export const HazeDepth = 1.8;
