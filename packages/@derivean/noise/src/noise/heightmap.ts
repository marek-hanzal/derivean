/** @format */

import { ValueNoise } from "@derivean/noise-wasm";
import { fpClamp, fpCombineNoise, fpScaleXZ, fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { blend } from "../utils/blend";
import { createNoiseCache } from "../utils/createNoiseCache";

export const heightmap = (seed: string) => {
	const sourceNoise = new ValueNoise(`${seed}-source`);
	const controlNoise = new ValueNoise(`${seed}-control`);
	const noise2 = new ValueNoise(`${seed}-heightmap-3`);

	return createNoiseCache({
		noise: flow(
			fpScaleXZ({ scale: 1 }),
			flow(
				fpCombineNoise({
					noise1: {
						noise: blend({
							scale: [2, 5],
							limit: [0.2, 0.8],
							sourceNoise: ([x, z]) => sourceNoise.get(x, z),
							controlNoise: ([x, z]) => controlNoise.get(x, z),
						}),
						weight: 1,
					},
					noise2: {
						noise: flow(([x, z]) => noise2.get(x, z), fpClamp({ min: -1, max: 0 })),
						weight: 1,
					},
				}),
			),
			fpWeight({ weight: 1.5 }),
			fpClamp({ min: -1, max: 1 }),
		),
	});
};
