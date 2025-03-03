/** @format */

import { ValueNoise } from "@derivean/noise-wasm";
import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoiseCache } from "../utils/createNoiseCache";

export const shade = (seed: string) => {
	const noise = new ValueNoise(`${seed}-shade`);

	return createNoiseCache({ noise: flow(([x, z]) => noise.get(x, z), fpWeight({ weight: 2 })) });
};
