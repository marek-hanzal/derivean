import { Vector3 } from "three/webgpu";
import type { HazeAir } from "../type/HazeAir";

// Clean air is the simulation's (WorldInfo.air, AirOptics); the haze is the weather's (HazeAir), coloured by
// what it is made of.
// The veil of ash and sulphate above the haze thins by e over this height from the sea.
export const VeilHeight = 8000;
// Metres over which the haze fades out at its top.
export const HazeEdge = 100;
// The air until the first weather snapshot arrives: a clean maritime day's, grey sea salt.
export const StillAir: Readonly<HazeAir> = { haze: 0.05, top: 2000, thickness: 2000, veil: 0, spectrum: [1, 1, 1], albedo: [1, 1, 1] };

// Metres between which the cloud over the eye stops colouring the air: the weather there is another cell's.
export const VeilReach = [30000, 120000] as const;

// Share of the view distance at which the far terrain starts to be given to the sky outright.
export const AirEnd = 0.8;

// The exposure is set for this much light on level ground (a clear sky with the sun about 45° up) and follows
// the light by this power: 1 would make an overcast noon as bright as a clear one, 0 would leave it as dull as it
// is. The range ends where the light on level ground is a sun 10° up: below it the sky would burn out, and the
// ground is left dark as a camera would leave it.
export const ExposedLight = 1.7;
export const Exposure = 1.2;
export const Adaptation = 0.6;
export const ExposureRange = [0.9, 3.2] as const;

// Share of the light falling on level ground that the ground sends back up at the faces above it, and its hue.
export const GroundBounce = 0.28;
export const GroundTint = "#8b6952";

// Where the light of plain relief shading comes from, whatever the hour.
export const ReliefSun = new Vector3(-13, 12, 7).normalize();

// Sky colours at the three moments the rest are blended from, as sRGB, for StillAir: its haze makes ReferenceHaze
// of the light lost along the ground at the sea and holds ReferenceDepth overhead. More haze whitens the day's
// horizon towards its own grey and pales the day's zenith towards the horizon by ZenithPaling per unit of depth.
export const ReferenceHaze = 0.65;
export const ReferenceDepth = 0.05;
export const ZenithPaling = 1.5;
// Sky colours at the three moments the rest are blended from, as sRGB.
export const DaySky = { zenith: "#3d6d9c", horizon: "#b3c4c8", glow: "#fff1d6" };
export const DuskSky = { zenith: "#2a3b63", horizon: "#b79a8c", glow: "#ff8b3d" };
export const NightSky = { zenith: "#04070d", horizon: "#0a111c", glow: "#000000" };
