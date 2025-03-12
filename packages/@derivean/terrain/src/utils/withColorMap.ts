/** @format */

import type { Tile } from "@derivean/chunk";
import { HSLA, hslaToRgba, RGBA, type Color } from "@derivean/utils";

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

export namespace withColorMap {
	export interface Props {
		tile: Tile;
	}
}

/**
 * Maps a noise value to an RGBA color based on the provided color map
 * Currently uses heightmap as the primary source for color mapping
 */
export const withColorMap = ({ tile }: withColorMap.Props): Color.RGBA => {
	const color = HSLA([0, 0, 0, 1]);

	console.log("tile", tile.land.plain);

	return hslaToRgba(color);
};
