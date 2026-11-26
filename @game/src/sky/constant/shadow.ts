// Half extent in metres and resolution of the sun shadow frustum that follows the camera.
export const ShadowReach = 32;
export const ShadowMap = 2048;
// Share of the shadow reach over which shadows thin out towards its edge.
export const ShadowFade = 0.3;
// View depths in metres between which voxel faces turn from their own normals to the mean slope normal. The
// range ends where the shadows end, so voxels beyond are lit exactly like the far terrain.
export const MeanShading: [number, number] = [ShadowReach / 2, ShadowReach];
