import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";

export const moisture = (seed: string) =>
	flow(
		createNoise({
			seed: `${seed}-moisture`,
			type: "Cellular",
			cellular: {
				distanceFunction: "EuclideanSq",
				returnType: "CellValue",
			},
			frequency: 0.05,
		}),
		fpWeight({ weight: 2 }),
	);
