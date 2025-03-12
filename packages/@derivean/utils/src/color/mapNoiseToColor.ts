/** @format */

import { HSLA, type Color } from "@derivean/utils";

/**
 * Maps a noise value (-1 to 1) to an HSLA color within the specified range
 *
 * @param noise The input noise value, should be between -1 and 1
 * @param colorRange An object containing min and max HSLA colors
 * @returns An HSLA color interpolated based on the noise value
 */
export interface ColorRange {
	min: Color.HSLA;
	max: Color.HSLA;
}

export function mapNoiseToColor(noise: number, colorRange: ColorRange): Color.HSLA {
	// Ensure the noise value is clamped between -1 and 1
	const clampedNoise = Math.max(-1, Math.min(1, noise));

	// Convert noise from [-1, 1] to [0, 1] for interpolation
	const t = (clampedNoise + 1) / 2;

	// Extract color components
	const [minH, minS, minL, minA] = colorRange.min.color;
	const [maxH, maxS, maxL, maxA] = colorRange.max.color;

	// Handle hue interpolation specially (since it's circular)
	let hueDiff = maxH - minH;

	// Take the shortest path around the color wheel
	if (Math.abs(hueDiff) > 180) {
		if (hueDiff > 0) {
			hueDiff -= 360;
		} else {
			hueDiff += 360;
		}
	}

	const h = (minH + hueDiff * t + 360) % 360;
	const s = minS + (maxS - minS) * t;
	const l = minL + (maxL - minL) * t;
	const a = minA + (maxA - minA) * t;

	return HSLA([h, s, l, a]);
}
