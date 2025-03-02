/** @format */

import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";

export const shade = (seed: string) =>
	flow(
		createNoise({ seed: `${seed}-shade`, type: "ValueCubic", frequency: 0.005 }),
		fpWeight({ weight: 2 }),
	);
