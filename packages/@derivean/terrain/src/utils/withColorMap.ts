/** @format */

import type { Tile } from "@derivean/chunk";
import { HSLA, hslaToRgba, mapNoiseToColor, type Color } from "@derivean/utils";

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

	return hslaToRgba(
		mapNoiseToColor(tile.biome.deep_ocean, {
			min: HSLA([220, 100, 30, 1]),
			max: HSLA([200, 80, 60, 1]),
		}),
	);

	// return hslaToRgba(color);
};
