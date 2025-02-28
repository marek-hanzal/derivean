import type { NoiseSource } from "@derivean/utils";
import { biome } from "./noise/biome";
import { heightmap } from "./noise/heightmap";
import { moisture } from "./noise/moisture";
import { shade } from "./noise/shade";
import { temperature } from "./noise/temperature";

export const DefaultNoiseSource: NoiseSource = ({ seed }) => {
	return {
		heightmap: heightmap(seed),
		biome: biome(seed),
		temperature: temperature(seed),
		moisture: moisture(seed),
		shade: shade(seed),
	};
};
