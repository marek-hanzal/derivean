/** @format */

import { fpClamp, HSLA, type Biome, type Color } from "@derivean/utils";

// const BIOME_DEEP_OCEAN = [-1, -0.5] as const;
const BIOME_GRASSLAND = [-1, 1] as const;
// const BIOME_MOUNTAINS = [0.75, 1] as const;

export const ComplexBiome: Biome = {
	type: "Comprehensive",
	resolve({ source, color }) {
		let output: Color.HSLA = color;

		switch (true) {
			// case source.biome >= BIOME_DEEP_OCEAN[0] && source.biome <= BIOME_DEEP_OCEAN[1]:
			// 	output = HSLA([240, 100, 10, 1]);
			// 	break;
			case source.biome >= BIOME_GRASSLAND[0] && source.biome <= BIOME_GRASSLAND[1]:
				output = HSLA([120, 100, 50, 1]);
				// output.color[0] += Math.abs(source.biome) * 30;
				output.color[2] += fpClamp({ min: -25, max: 30 })(source.heightmap * 50);
				break;
			// case source.biome >= BIOME_MOUNTAINS[0] && source.biome <= BIOME_MOUNTAINS[1]:
			// 	output = HSLA([0, 100, 50, 1]);
			// 	// output.color[0] += Math.abs(source.biome) * 30;
			// 	output.color[2] += fpClamp({ min: -25, max: 30 })(source.heightmap * 50);
			// 	break;
		}

		return {
			color: output,
			/**
			 * This biome should rule them all.
			 */
			exclusive: true,
		};
	},
};
