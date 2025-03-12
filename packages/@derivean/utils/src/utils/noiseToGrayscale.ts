/** @format */

import { RGBA, type Color } from "../color/Color";

/**
 * Converts a noise value (-1 to 1) to a grayscale RGBA color
 *
 * @param noise Noise value from -1 to 1
 * @returns RGBA color with full opacity (alpha = 255)
 */
export function noiseToGrayscale(noise: number): Color.RGBA {
	// Clamp the noise value to ensure it's within -1 to 1
	const clampedNoise = Math.max(-1, Math.min(1, noise));

	// Map the noise from [-1, 1] to [0, 1]
	const normalizedNoise = (clampedNoise + 1) / 2;

	// Map the normalized noise to 0-255 range
	const colorValue = Math.round(normalizedNoise * 255);

	// Return grayscale RGBA (all RGB channels have the same value)
	return RGBA([colorValue, colorValue, colorValue, 255]);
}
