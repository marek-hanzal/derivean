/** @format */

import type { Tile } from "@derivean/chunk";
import { HSLA, hslaToRgba, type Color } from "@derivean/utils";

/**
 * Color definitions for different land types
 * Using HSLA as it's easier to make color transitions
 */
const COLOR_MAP = {
	mountain: HSLA([0, 0, 35, 1]), // Gray mountains
	platau: HSLA([30, 60, 70, 1]), // Brownish plateaus
	valley: HSLA([115, 50, 32, 1]), // Dark green valleys
	hill: HSLA([85, 65, 50, 1]), // Lighter green hills
	plain: HSLA([60, 70, 75, 1]), // Yellowish plains
};

export namespace withColorMap {
	export interface Props {
		tile: Tile;
	}
}

/**
 * Maps noise values to HSLA/RGBA colors based on the strength of each land type
 * Blends colors based on the relative strength of each land type
 */
export const withColorMap = ({ tile }: withColorMap.Props): Color.RGBA => {
	// Extract land properties
	const { mountain, platau, valley, hill, plain } = tile.land;

	// Map each noise value to a normalized weight (0 to 1)
	// First, convert from -1,1 range to 0,1 range
	const mountainWeight = (mountain + 1) / 2;
	const platauWeight = (platau + 1) / 2;
	const valleyWeight = (valley + 1) / 2;
	const hillWeight = (hill + 1) / 2;
	const plainWeight = (plain + 1) / 2;

	// Calculate total weight to normalize
	const totalWeight = mountainWeight + platauWeight + valleyWeight + hillWeight + plainWeight;

	// Guard against division by zero
	if (totalWeight === 0) {
		return hslaToRgba(HSLA([0, 0, 50, 1])); // Default gray if all weights are zero
	}

	// Initialize color components for weighted average
	let h = 0;
	let s = 0;
	let l = 0;

	// Add weighted contribution from each land type
	h += COLOR_MAP.mountain.color[0] * mountainWeight;
	h += COLOR_MAP.platau.color[0] * platauWeight;
	h += COLOR_MAP.valley.color[0] * valleyWeight;
	h += COLOR_MAP.hill.color[0] * hillWeight;
	h += COLOR_MAP.plain.color[0] * plainWeight;

	s += COLOR_MAP.mountain.color[1] * mountainWeight;
	s += COLOR_MAP.platau.color[1] * platauWeight;
	s += COLOR_MAP.valley.color[1] * valleyWeight;
	s += COLOR_MAP.hill.color[1] * hillWeight;
	s += COLOR_MAP.plain.color[1] * plainWeight;

	l += COLOR_MAP.mountain.color[2] * mountainWeight;
	l += COLOR_MAP.platau.color[2] * platauWeight;
	l += COLOR_MAP.valley.color[2] * valleyWeight;
	l += COLOR_MAP.hill.color[2] * hillWeight;
	l += COLOR_MAP.plain.color[2] * plainWeight;

	// Normalize by dividing by total weight
	h /= totalWeight;
	s /= totalWeight;
	l /= totalWeight;

	// Create the final blended HSLA color
	const blendedColor = HSLA([h, s, l, 1]);

	// Convert to RGBA and return
	return hslaToRgba(blendedColor);
};
