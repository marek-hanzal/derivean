/** @format */

import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";

export const biome = (seed: string) =>
	flow(
		createNoise({ seed: `${seed}-biome`, type: "ValueCubic", frequency: 0.025 }),
		fpWeight({ weight: 2 }),
	);
