import { createNoise } from "../utils/createNoise";

export const shade = (seed: string) =>
	createNoise({
		seed: `${seed}-shade`,
	});
