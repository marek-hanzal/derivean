/** @format */

import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";
import { createNoiseCache } from "../utils/createNoiseCache";

export const shade = (seed: string) =>
	createNoiseCache({
		noise: flow(
			createNoise({ seed: `${seed}-shade`, type: "ValueCubic", frequency: 0.005 }),
			fpWeight({ weight: 2 }),
		),
	});
