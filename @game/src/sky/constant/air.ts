import { Vector3 } from "three/webgpu";

// What the air takes from light per metre at sea level: clean air by colour (Rayleigh scattering), and the dust
// and droplets in it, which take all colours alike. Together they let a dark ridge be told from the sky about
// 250 km away. Each thins by e over its height in metres.
export const CleanAir = [5.8e-6, 13.5e-6, 33.1e-6] as const;
export const CleanAirHeight = 8000;
export const HazeAir = 3e-6;
export const HazeAirHeight = 1200;
// The rest of the dust and droplets, spread higher: its optical depth straight up from the sea, and the height
// over which it thins by e. With the haze below it the air holds about 0.05 of it, a clean day's on Earth, and that
// is what lights the shadows (clearSky): at a sun 10° up, snow in shadow takes about 0.39 of the light of
// snow in the sun at the sea, 0.33 at 1200 m. It costs the view: a dark ridge is told from the sky about 180 km
// away at the sea, 220 km at 1200 m.
export const HighHaze = 0.045;
export const HighHazeHeight = 8000;

// Metres between which the cloud over the eye stops colouring the air: the weather there is another cell's.
export const VeilReach = [30000, 120000] as const;

// Share of the view distance at which the far terrain starts to be given to the sky outright.
export const AirEnd = 0.8;

// The exposure is set for this much light on level ground (the clear noon of a low sun) and follows the light
// by this power: 1 would make an overcast noon as bright as a clear one, 0 would leave it as dull as it is. The
// range keeps the sky from burning out at dusk, where the ground is left dark as a camera would leave it.
export const ExposedLight = 1.7;
export const Exposure = 1.2;
export const Adaptation = 0.6;
export const ExposureRange = [0.9, 2.2] as const;

// Share of the light falling on level ground that the ground sends back up at the faces above it, and its hue.
export const GroundBounce = 0.28;
export const GroundTint = "#8b6952";

// Where the light of plain relief shading comes from, whatever the hour.
export const ReliefSun = new Vector3(-13, 12, 7).normalize();

// Sky colours at the three moments the rest are blended from, as sRGB.
export const DaySky = { zenith: "#3d6d9c", horizon: "#b3c4c8", glow: "#fff1d6" };
export const DuskSky = { zenith: "#2a3b63", horizon: "#b79a8c", glow: "#ff8b3d" };
export const NightSky = { zenith: "#04070d", horizon: "#0a111c", glow: "#000000" };
