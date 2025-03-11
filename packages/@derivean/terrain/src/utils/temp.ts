/** @format */

import { HSLA, hslaToRgba, type Color, type NoiseType } from "@derivean/utils";

export interface NoiseColorMapping {
	/**
	 * Range of noise values for this color mapping
	 */
	range: [number, number];
	/**
	 * Base HSLA color when noise is at the minimum of the range
	 */
	colorStart: Color.HSLA;
	/**
	 * Base HSLA color when noise is at the maximum of the range
	 */
	colorEnd: Color.HSLA;
}

/**
 * Configuration for noise to color mapping
 */
export interface NoiseToColorConfig {
	/**
	 * Mapping for each noise type
	 */
	heightMap?: NoiseColorMapping[];
	biomeMap?: NoiseColorMapping[];
	tempMap?: NoiseColorMapping[];
	moistureMap?: NoiseColorMapping[];
	shadeMap?: NoiseColorMapping[];
	/**
	 * Weights for each noise type (how much it contributes to the final color)
	 */
	weights?: {
		height?: number;
		biome?: number;
		temperature?: number;
		moisture?: number;
		shade?: number;
	};
}

/**
 * Default noise to color mapping configurations
 */
export const defaultNoiseToColorConfig: NoiseToColorConfig = {
	// Default Height Color Map - grayscale (black to white)
	heightMap: [
		{
			range: [-1, 1],
			colorStart: HSLA([0, 0, 0, 1]), // Black
			colorEnd: HSLA([0, 0, 100, 1]), // White
		},
	],
	// Default Biome Color Map - green to brown to gray
	biomeMap: [
		{
			range: [-1, 0],
			colorStart: HSLA([120, 70, 30, 1]), // Dark green
			colorEnd: HSLA([35, 60, 40, 1]), // Brown
		},
		{
			range: [0, 1],
			colorStart: HSLA([35, 60, 40, 1]), // Brown
			colorEnd: HSLA([0, 0, 70, 1]), // Gray
		},
	],
	// Default Temperature Color Map - cold to hot
	tempMap: [
		{
			range: [-1, 1],
			colorStart: HSLA([240, 70, 50, 1]), // Cold blue
			colorEnd: HSLA([0, 70, 50, 1]), // Hot red
		},
	],
	// Default Moisture Color Map - affects saturation
	moistureMap: [
		{
			range: [-1, 1],
			colorStart: HSLA([0, 0, 50, 1]), // Desaturated
			colorEnd: HSLA([0, 100, 50, 1]), // Fully saturated
		},
	],
	// Default Shade Color Map - grayscale adjustment
	shadeMap: [
		{
			range: [-1, 1],
			colorStart: HSLA([0, 0, 20, 1]), // Dark shade
			colorEnd: HSLA([0, 0, 80, 1]), // Light shade
		},
	],
	// Default weights for each noise type
	weights: { height: 0.35, biome: 0.35, temperature: 0.1, moisture: 0.1, shade: 0.1 },
};

/**
 * Interpolates between two HSLA colors based on a factor
 *
 * @param color1 First color
 * @param color2 Second color
 * @param factor Interpolation factor (0 to 1)
 * @returns Interpolated HSLA color
 */
function interpolateHSLA(color1: Color.HSLA, color2: Color.HSLA, factor: number): Color.HSLA {
	const [h1, s1, l1, a1] = color1.color;

	const [h2, s2, l2, a2] = color2.color;

	// Special handling for hue to ensure we go the shortest way around the circle
	let h;
	const hueDiff = h2 - h1;
	if (hueDiff > 180) {
		h = h1 + (hueDiff - 360) * factor;
	} else if (hueDiff < -180) {
		h = h1 + (hueDiff + 360) * factor;
	} else {
		h = h1 + hueDiff * factor;
	}

	// Make sure hue stays in 0-360 range
	h = (h + 360) % 360;

	const s = s1 + (s2 - s1) * factor;
	const l = l1 + (l2 - l1) * factor;
	const a = a1 + (a2 - a1) * factor;

	return HSLA([h, s, l, a]);
}

/**
 * Maps a noise value to a color using the provided mapping
 *
 * @param value Noise value (-1 to 1)
 * @param mapping Array of color mappings
 * @returns HSLA color
 */
function mapNoiseToColor(value: number, mapping: NoiseColorMapping[]): Color.HSLA {
	// Default to neutral color if no mapping is provided
	if (!mapping || mapping.length === 0) {
		return HSLA([0, 0, 50, 1]);
	}

	// Find the mapping that contains the value
	for (const map of mapping) {
		const [min, max] = map.range;
		if (value >= min && value <= max) {
			const factor = (value - min) / (max - min);
			return interpolateHSLA(map.colorStart, map.colorEnd, factor);
		}
	}

	// Fallback to the last mapping's end color if value is out of range
	if (value > mapping[mapping.length - 1]!.range[1]) {
		return mapping[mapping.length - 1]!.colorEnd;
	}

	// Fallback to the first mapping's start color if value is out of range
	return mapping[0]!.colorStart;
}

/**
 * Blends multiple HSLA colors together based on weights
 *
 * @param colors Array of HSLA colors
 * @param weights Array of weights for each color
 * @returns Blended HSLA color
 */
function blendHSLAColors(colors: Color.HSLA[], weights: number[]): Color.HSLA {
	if (colors.length !== weights.length || colors.length === 0) {
		return HSLA([0, 0, 50, 1]); // Neutral gray
	}

	// Normalize weights
	const totalWeight = weights.reduce((sum, weight) => sum + weight, 0);
	const normalizedWeights = weights.map((weight) => weight / totalWeight);

	// Calculate weighted averages for S, L, A
	let h = 0;
	let s = 0;
	let l = 0;
	let a = 0;
	let hSin = 0;
	let hCos = 0;

	// Special handling for hue to avoid issues with the circular nature of hue
	for (let i = 0; i < colors.length; i++) {
		const hue = colors[i]!.color[0]!;
		const hueRad = (hue * Math.PI) / 180;
		hSin += Math.sin(hueRad) * normalizedWeights[i]!;
		hCos += Math.cos(hueRad) * normalizedWeights[i]!;

		s += colors[i]!.color[1] * normalizedWeights[i]!;
		l += colors[i]!.color[2] * normalizedWeights[i]!;
		a += colors[i]!.color[3] * normalizedWeights[i]!;
	}

	// Convert back to degrees
	h = (Math.atan2(hSin, hCos) * 180) / Math.PI;
	if (h < 0) {
		h += 360;
	} // Ensure positive hue value

	return HSLA([h, s, l, a]);
}

/**
 * Converts noise values to a color based on provided or default mappings
 *
 * @param noiseValues Object containing noise values (-1 to 1)
 * @param config Configuration for noise to color mapping
 * @returns RGBA color
 */
export function noiseToColor(
	noiseValues: Record<NoiseType, number>,
	config: NoiseToColorConfig = defaultNoiseToColorConfig,
): Color.RGBA {
	const {
		heightMap = defaultNoiseToColorConfig.heightMap,
		biomeMap = defaultNoiseToColorConfig.biomeMap,
		tempMap = defaultNoiseToColorConfig.tempMap,
		moistureMap = defaultNoiseToColorConfig.moistureMap,
		shadeMap = defaultNoiseToColorConfig.shadeMap,
		weights = defaultNoiseToColorConfig.weights,
	} = config;

	// Map each noise value to a color
	const heightColor = mapNoiseToColor(noiseValues.height, heightMap || []);
	const biomeColor = mapNoiseToColor(noiseValues.biome, biomeMap || []);
	const tempColor = mapNoiseToColor(noiseValues.temp, tempMap || []);
	const moistureColor = mapNoiseToColor(noiseValues.moisture, moistureMap || []);
	const shadeColor = mapNoiseToColor(noiseValues.shade, shadeMap || []);

	// Blend colors based on weights
	const blendedColor = blendHSLAColors(
		[heightColor, biomeColor, tempColor, moistureColor, shadeColor],
		[
			weights!.height || 0,
			weights!.biome || 0,
			weights!.temperature || 0,
			weights!.moisture || 0,
			weights!.shade || 0,
		],
	);

	biomeColor.color[3] = 1;

	// Convert to RGBA
	return hslaToRgba(blendedColor);
}
