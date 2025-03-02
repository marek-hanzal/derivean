/** @format */

import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";
import { createNoiseCache } from "../utils/createNoiseCache";

export const temperature = (seed: string) =>
	createNoiseCache({
		noise: flow(
			createNoise({ seed: `${seed}-temperature`, type: "Value", frequency: 0.005 }),
			fpWeight({ weight: 2 }),
		),
	});
