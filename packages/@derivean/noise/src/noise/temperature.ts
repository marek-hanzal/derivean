import { fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { createNoise } from "../utils/createNoise";

export const temperature = (seed: string) =>
	flow(
		createNoise({
			seed: `${seed}-temperature`,
			type: "Cellular",
			cellular: {
				distanceFunction: "EuclideanSq",
				returnType: "CellValue",
			},
			frequency: 0.05,
		}),
		fpWeight({ weight: 2 }),
	);
