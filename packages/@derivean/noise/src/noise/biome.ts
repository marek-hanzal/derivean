/** @format */

import { PerlinNoise } from "@derivean/noise-wasm";
import { fpWeight, type XZ } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";

export const biome = (seed: string) => {
	const perlin = new PerlinNoise(seed);
	// perlin.set_frequency(1);
	// perlin.set_octaves(1);

	return flow(([x, z]: XZ) => perlin.get(x, z), fpWeight({ weight: 2 }));
};
