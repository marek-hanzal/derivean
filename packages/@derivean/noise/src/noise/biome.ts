/** @format */

import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";
import { createNoiseCache } from "../utils/createNoiseCache";

export const biome = (seed: string) =>
	createNoiseCache({
		noise: flow(
			createNoise({
				seed: `${seed}-biome`,
				type: "Cellular",
				cellular: { distanceFunction: "EuclideanSq", returnType: "CellValue" },
				frequency: 0.025,
			}),
			fpWeight({ weight: 2 }),
		),
	});
