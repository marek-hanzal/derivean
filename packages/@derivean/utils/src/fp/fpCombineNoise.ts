import { flow } from "fp-ts/lib/function";
import type { Noise } from "../noise/Noise";
import type { XZ } from "../type/XZ";
import { fpClamp } from "./fpClamp";

export namespace fpCombineNoise {
	export interface Props {
		noise1: {
			noise: Noise;
			weight: number;
		};
		noise2: {
			noise: Noise;
			weight: number;
		};
	}
}

export const fpCombineNoise = ({ noise1, noise2 }: fpCombineNoise.Props): Noise => {
	return flow((xz: XZ) => noise1.noise(xz) * noise1.weight + noise2.noise(xz) * noise2.weight, fpClamp({ min: -1, max: 1 }));
};
