/** @format */

import { HSLA, hslaToRgba, type Biome, type Color, type NoiseType } from "@derivean/utils";

export namespace withColorMap {
	export interface Props {
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
export const withColorMap = ({ biomes = [], source }: withColorMap.Props): Color.RGBA => {
	let color = HSLA([0, 0, 0, 1]);
	const type: string[] = [];

	for (const biome of biomes) {
		const resolved = biome.resolve({ type, color, source });

		if (resolved) {
			({ color } = resolved);
			type.push(biome.type);
			if (resolved.exclusive) {
				break;
			}
		}
	}

	return hslaToRgba(color);
};
