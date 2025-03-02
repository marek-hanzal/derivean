/** @format */

import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";

export const moisture = (seed: string) =>
	flow(
		createNoise({ seed: `${seed}-moisture`, type: "ValueCubic", frequency: 0.015 }),
		fpWeight({ weight: 2 }),
	);
