import { fpScaleXZ } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";

export const biome = (seed: string) =>
	flow(
		fpScaleXZ({ scale: 0.5 }),
		flow(
			createNoise({
				seed: `${seed}-biome`,
				frequency: 1,
				type: "Cellular",
			}),
		),
	);
