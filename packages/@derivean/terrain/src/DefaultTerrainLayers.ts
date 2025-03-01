/** @format */

import { HSLA, type TerrainLayer } from "@derivean/utils";

/**
 * Non-linear terrain layers where noise values don't strictly correlate with visual elevation
 * This creates more interesting terrain with "pockets" of different terrain types
 *
 * The grayscale values loosely represent visual elevation (darker = lower, lighter = higher)
 * but the noise value ranges are deliberately non-linear to create varied terrain
 */
export const DefaultTerrainLayers = {
	// DEEP WATER ZONES (noise: -1.0 to -0.60)
	DeepOcean: {
		name: "deep-ocean",
		color: HSLA([0, 0, 10, 1.0]),
		length: 0.2,
		steps: 6,
		transition: 4,
	} satisfies TerrainLayer,

	Abyss: {
		name: "abyss",
		color: HSLA([0, 0, 8, 1.0]),
		length: 0.1,
		steps: 4,
		transition: 3,
	} satisfies TerrainLayer,

	TrenchZone: {
		name: "trench",
		color: HSLA([0, 0, 5, 1.0]),
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	// SHALLOW WATER ZONES (noise: -0.60 to -0.38)
	Ocean: {
		name: "ocean",
		color: HSLA([0, 0, 18, 1.0]),
		length: 0.12,
		steps: 5,
		transition: 4,
	} satisfies TerrainLayer,

	ShallowWater: {
		name: "shallow-water",
		color: HSLA([0, 0, 24, 1.0]),
		length: 0.12,
		steps: 5,
		transition: 5,
	} satisfies TerrainLayer,

	// TRANSITIONAL ZONES (noise: -0.38 to -0.22)
	Reef: {
		name: "reef",
		color: HSLA([0, 0, 30, 1.0]),
		length: 0.06,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	Coast: {
		name: "coast",
		color: HSLA([0, 0, 35, 1.0]),
		length: 0.06,
		steps: 4,
		transition: 4,
	} satisfies TerrainLayer,

	Beach: {
		name: "beach",
		color: HSLA([0, 0, 42, 1.0]),
		length: 0.04,
		steps: 3,
		transition: 4,
	} satisfies TerrainLayer,

	// LOWLAND ZONES (noise: -0.22 to 0.00)
	// These appear at lower noise values but represent moderate elevations
	Lowland: {
		name: "lowland",
		color: HSLA([0, 0, 48, 1.0]),
		length: 0.08,
		steps: 4,
		transition: 4,
	} satisfies TerrainLayer,

	Grassland: {
		name: "grassland",
		color: HSLA([0, 0, 52, 1.0]),
		length: 0.07,
		steps: 4,
		transition: 4,
	} satisfies TerrainLayer,

	Savanna: {
		name: "savanna",
		color: HSLA([0, 0, 55, 1.0]),
		length: 0.07,
		steps: 4,
		transition: 3,
	} satisfies TerrainLayer,

	// DEPRESSION ZONES (noise: 0.00 to 0.15)
	// These appear at higher noise values but represent lower elevations (depressions)
	Valley: {
		name: "valley",
		color: HSLA([0, 0, 40, 1.0]), // Darker than grasslands, representing lower elevation
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	Basin: {
		name: "basin",
		color: HSLA([0, 0, 38, 1.0]), // Even darker, lower elevation
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	Wetland: {
		name: "wetland",
		color: HSLA([0, 0, 36, 1.0]), // Darker, representing wet depressions
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	// HIGHLAND ZONES (noise: 0.15 to 0.35)
	// Back to higher elevations
	Midland: {
		name: "midland",
		color: HSLA([0, 0, 58, 1.0]),
		length: 0.07,
		steps: 4,
		transition: 3,
	} satisfies TerrainLayer,

	Highland: {
		name: "highland",
		color: HSLA([0, 0, 63, 1.0]),
		length: 0.07,
		steps: 4,
		transition: 3,
	} satisfies TerrainLayer,

	Plateau: {
		name: "plateau",
		color: HSLA([0, 0, 67, 1.0]),
		length: 0.06,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	// CANYON ZONES (noise: 0.35 to 0.45)
	// Another depression, surrounded by higher terrain
	Canyon: {
		name: "canyon",
		color: HSLA([0, 0, 45, 1.0]), // Darker than highlands, representing lower elevation
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	Ravine: {
		name: "ravine",
		color: HSLA([0, 0, 43, 1.0]), // Even darker
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	// MOUNTAIN ZONES (noise: 0.45 to 0.85)
	Foothills: {
		name: "foothills",
		color: HSLA([0, 0, 70, 1.0]),
		length: 0.08,
		steps: 4,
		transition: 4,
	} satisfies TerrainLayer,

	Mountain: {
		name: "mountain",
		color: HSLA([0, 0, 75, 1.0]),
		length: 0.16,
		steps: 6,
		transition: 4,
	} satisfies TerrainLayer,

	AlpineSlope: {
		name: "alpine-slope",
		color: HSLA([0, 0, 78, 1.0]),
		length: 0.14,
		steps: 5,
		transition: 4,
	} satisfies TerrainLayer,

	// CRATER ZONE (noise: 0.85 to 0.90)
	// Another depression at high noise values
	Crater: {
		name: "crater",
		color: HSLA([0, 0, 50, 1.0]), // Much darker than surrounding mountains
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	// HIGHEST ZONES (noise: 0.90 to 1.00)
	HighMountain: {
		name: "high-mountain",
		color: HSLA([0, 0, 82, 1.0]),
		length: 0.05,
		steps: 3,
		transition: 3,
	} satisfies TerrainLayer,

	MountainPeak: {
		name: "mountain-peak",
		color: HSLA([0, 0, 85, 1.0]), // Not quite white to allow for coloring
		length: 0.1,
		steps: 3,
	} satisfies TerrainLayer,
} as const;
