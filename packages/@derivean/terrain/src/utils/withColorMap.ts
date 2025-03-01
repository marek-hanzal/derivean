/** @format */

import {
	hslaToRgba,
	RGBA,
	type Biome,
	type Color,
	type NoiseColorMap,
	type NoiseType,
} from "@derivean/utils";

export namespace withColorMap {
	export interface Props {
		/**
		 * Color map used to convert noise values to colors
		 */
		colorMap: NoiseColorMap;
		biomes?: Biome[];
		/**
		 * Various noise sources that can be used for coloring
		 */
		source: Record<NoiseType, number>;
	}
}

/**
 * Maps a noise value to an RGBA color based on the provided color map
 * Currently uses heightmap as the primary source for color mapping
 */
export const withColorMap = ({ colorMap, biomes = [], source }: withColorMap.Props): Color.RGBA => {
	const base = colorMap.find(({ level: [min, max] }) => {
		return source.heightmap! >= min && source.heightmap! <= max;
	});

	/**
	 * No base color, nothing to do, sorry.
	 */
	if (!base) {
		return RGBA([128, 128, 128, 255]);
	}

	const { color } = base;
	// const type: string[] = ["heightmap"];

	// for (const biome of biomes) {
	// 	const resolved = biome.resolve({
	// 		type,
	// 		base,
	// 		color,
	// 		source,
	// 	});

	// 	if (resolved) {
	// 		({ color } = resolved);
	// 		type.push(biome.type);
	// 		if (resolved.exclusive) {
	// 			break;
	// 		}
	// 	}
	// }

	return hslaToRgba(color);
};
